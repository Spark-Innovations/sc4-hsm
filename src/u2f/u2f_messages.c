#include "mbedtls/config.h"
#if 0 // defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <sys/types.h>
#include <stdarg.h>
#endif

#include <assert.h>
#include <string.h>

#include "endian.h"
#include "keys.h"
#include "mbedtls/aes.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/entropy.h"
#include "u2f.h"
#include "hardware.h"

unsigned int g_counter = 1;
#define IMPL_U2F_KEYHANDLE_SIZE 64 /* Expected key handle size */

#define mbedtls_printf lcd_printf

uint16_t u2f_register(U2F_REGISTER_REQ *req, U2F_REGISTER_RESP *resp,
		      int flags, uint16_t *olen) {
  int ret, i;
  size_t len;
  mbedtls_ecdsa_context ctx_new_ec;
  mbedtls_ecdsa_context ctx_attestation;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_aes_context aes;
  mbedtls_md_context_t ctx_sha256;
  const mbedtls_md_info_t *md_info;
  const char *pers = "ecdsa";
  unsigned char buf[64];
  unsigned char *ptr;
  uint16_t status = U2F_SW_INS_NOT_SUPPORTED;

  memset(resp, 0, sizeof(*resp));
  *olen = 0;
  resp->registerId = U2F_REGISTER_ID;

  mbedtls_ecdsa_init(&ctx_new_ec);
  mbedtls_ecdsa_init(&ctx_attestation);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_aes_init(&aes);
  mbedtls_md_init(&ctx_sha256);
  mbedtls_entropy_init(&entropy);

  /*
   * Generate a key pair for signing
   */

  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char *)pers,
                                   strlen(pers))) != 0) {
    mbedtls_printf("error: mbedtls_ctr_drbg_seed returned %d\n", ret);
    goto cleanup;
  }

  if ((ret = mbedtls_ecdsa_genkey(&ctx_new_ec, MBEDTLS_ECP_DP_SECP256R1,
                                  mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
    mbedtls_printf("error: mbedtls_ecdsa_genkey returned %d\n", ret);
    goto cleanup;
  }

  /* Export EC public key */
  ret = mbedtls_ecp_point_write_binary(
      &ctx_new_ec.grp, &ctx_new_ec.Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &len,
      (unsigned char *)&resp->pubKey, sizeof(resp->pubKey));
  if (ret != 0) {
    mbedtls_printf("error: mbedtls_ecp_point_write_binary returned %d\n", ret);
    goto cleanup;
  }

  /* Convert EC private key to a key handle -> encrypt it and the appId using
   * an AES private key */
  MBEDTLS_MPI_CHK(mbedtls_aes_setkey_enc(&aes, aes_key, 128));

  assert(mbedtls_mpi_size(&ctx_new_ec.d) == 32);

  /* load EC private key to start of buf */
  mbedtls_mpi_write_binary(&ctx_new_ec.d, buf,
			   mbedtls_mpi_size(&ctx_new_ec.d));

  /* Copy appId to the buffer, after the EC private key */
  memcpy(buf + 32, req->appId, U2F_APPID_SIZE);

  /* AES encrypt and store into response */
  for (i = 0, ptr = resp->keyHandleCertSig; i < IMPL_U2F_KEYHANDLE_SIZE / 16;
       i++, ptr += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, &buf[i * 16], ptr);
  }
  resp->keyHandleLen = IMPL_U2F_KEYHANDLE_SIZE;
  assert(ptr - resp->keyHandleCertSig == IMPL_U2F_KEYHANDLE_SIZE);

  /* Copy x509 attestation public key certificate */
  memcpy(ptr, attestation_cert, sizeof(attestation_cert));
  ptr += sizeof(attestation_cert);

  /* Compute SHA256 hash of the following items */

  md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  MBEDTLS_MPI_CHK(mbedtls_md_setup(&ctx_sha256, md_info, 0));
  memset(buf, 0, sizeof(buf));
  MBEDTLS_MPI_CHK(mbedtls_md_starts(&ctx_sha256));

  /* A byte reserved for future use [1 byte] with the value 0x00 */
  mbedtls_md_update(&ctx_sha256, buf, 1);
  /* The application parameter [32 bytes] from the registration request message.
   */
  mbedtls_md_update(&ctx_sha256, req->appId, U2F_APPID_SIZE);
  /* The challenge parameter [32 bytes] from the registration request message.
   */
  mbedtls_md_update(&ctx_sha256, req->chal, U2F_CHAL_SIZE);
  /* The key handle [variable length] */
  mbedtls_md_update(&ctx_sha256, resp->keyHandleCertSig,
                    IMPL_U2F_KEYHANDLE_SIZE);
  /* The user public key [65 bytes]. */
  mbedtls_md_update(&ctx_sha256, (unsigned char *)&resp->pubKey,
                    sizeof(U2F_EC_POINT));

  mbedtls_md_finish(&ctx_sha256, buf);

  /* Sign the SHA256 hash using the attestation key */
  mbedtls_ecp_group_load(&ctx_attestation.grp, MBEDTLS_ECP_DP_SECP256R1);
  mbedtls_mpi_read_binary(&ctx_attestation.d, attestation_private_key,
                          sizeof(attestation_private_key));

  if ((ret = mbedtls_ecdsa_write_signature(&ctx_attestation, MBEDTLS_MD_SHA256,
                                           buf, mbedtls_md_get_size(md_info),
                                           ptr, &len, mbedtls_ctr_drbg_random,
                                           &ctr_drbg)) != 0) {
    mbedtls_printf("error: mbedtls_ecdsa_genkey returned %d\n", ret);
    goto cleanup;
  }
  ptr += len;

  *olen = ptr - (unsigned char *)resp;
  status = U2F_SW_NO_ERROR;

cleanup:
  mbedtls_ecdsa_free(&ctx_attestation);
  mbedtls_ecdsa_free(&ctx_new_ec);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_aes_free(&aes);
  mbedtls_entropy_free(&entropy);
  mbedtls_md_free(&ctx_sha256);
  return status;
}

uint16_t u2f_authenticate(U2F_AUTHENTICATE_REQ *req,
                          U2F_AUTHENTICATE_RESP *resp, int flags) {
  int ret, i;
  size_t len;
  mbedtls_ecdsa_context ctx_ec;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_aes_context aes;
  mbedtls_md_context_t ctx_sha256;
  const mbedtls_md_info_t *md_info;
  const char *pers = "ecdsa";
  unsigned char buf[64];
  unsigned char *ptr;
  uint16_t status = U2F_SW_INS_NOT_SUPPORTED;

  memset(resp, 0, sizeof(*resp));

  mbedtls_ecdsa_init(&ctx_ec);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_aes_init(&aes);
  mbedtls_md_init(&ctx_sha256);
  mbedtls_entropy_init(&entropy);

  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char *)pers,
                                   strlen(pers))) != 0) {
    mbedtls_printf("error: mbedtls_ctr_drbg_seed returned %d\n", ret);
    goto cleanup;
  }

  /* Convert key handle to EC private key -> decrypt it using AES private key */
  MBEDTLS_MPI_CHK(mbedtls_aes_setkey_dec(&aes, aes_key, 128));

  if (req->keyHandleLen != IMPL_U2F_KEYHANDLE_SIZE) {
    mbedtls_printf("wrong key handle len %d\n", req->keyHandleLen);
    status = U2F_SW_WRONG_DATA;
    goto cleanup;
  }

  for (i = 0; i < req->keyHandleLen / 16; i++) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, &req->keyHandle[i * 16],
                          &buf[i * 16]);
  }

  /* compare request appid with appid extracted from key handle */
  if (memcmp(&buf[32], req->appId, U2F_APPID_SIZE) != 0) {
    mbedtls_printf("error: appid mismatch\n");
    status = U2F_SW_WRONG_DATA;
    goto cleanup;
  }

  mbedtls_ecp_group_load(&ctx_ec.grp, MBEDTLS_ECP_DP_SECP256R1);
  mbedtls_mpi_read_binary(&ctx_ec.d, buf, 32);

  *((uint32_t *)&resp->ctr) = cpu_to_be32(g_counter);
  g_counter++;
  resp->flags = U2F_AUTH_FLAG_TUP;

  /* Create signature */
  md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

  MBEDTLS_MPI_CHK(mbedtls_md_setup(&ctx_sha256, md_info, 0));
  MBEDTLS_MPI_CHK(mbedtls_md_starts(&ctx_sha256));

  /* The application parameter [32 bytes] from the registration request message.
   */
  mbedtls_md_update(&ctx_sha256, req->appId, U2F_APPID_SIZE);
  /* The user presence byte [1 byte]. */
  mbedtls_md_update(&ctx_sha256, &resp->flags, 1);
  /* The counter [4 bytes]. */
  mbedtls_md_update(&ctx_sha256, resp->ctr, sizeof(resp->ctr));
  /* The challenge parameter [32 bytes] from the registration request message.
   */
  mbedtls_md_update(&ctx_sha256, req->chal, U2F_CHAL_SIZE);
  mbedtls_md_finish(&ctx_sha256, buf);

  if ((ret = mbedtls_ecdsa_write_signature(
           &ctx_ec, MBEDTLS_MD_SHA256, buf, mbedtls_md_get_size(md_info),
           resp->sig, &len, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
    mbedtls_printf("error: mbedtls_ecdsa_genkey returned %d\n", ret);
    goto cleanup;
  }

  status = U2F_SW_NO_ERROR;

cleanup:
  mbedtls_ecdsa_free(&ctx_ec);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_aes_free(&aes);
  mbedtls_entropy_free(&entropy);
  mbedtls_md_free(&ctx_sha256);
  return status;
}

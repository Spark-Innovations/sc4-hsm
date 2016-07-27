#define FOR(i, n) for (i = 0; (int)i < (int)n; ++i)
#define sv static void

typedef unsigned char u8;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef long long i64;
typedef i64 gf[16];

typedef const unsigned char * string;

typedef struct {
  u8 h[64];
  u64 msglen;
  u64 a[8], z[8];
  u8 x[256];
} hash_state;

int crypto_hash_stream(u8 *out, hash_state *hs);
void spk2epk(u8 *epk, u8 *spk);

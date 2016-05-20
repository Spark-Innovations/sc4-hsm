
#include "hardware.h"
#include "tweetnacl.h"
#include "utils.h"

void printc(char c) { print("%c", c); }

void newline() { printc('\n'); }

/* Print bytes in hex */
const char *hexchars = "0123456789ABCDEF";

void printh(const u8 *n, int cnt) {
  for (int i = 0; i < cnt; i++) {
    printc(hexchars[n[i] >> 4]);
    printc(hexchars[n[i] & 15]);
  }
}

void sprinth(char *s, const u8* n, int cnt) {
  for (int i = 0; i < cnt; i++) {
    s[i*2] = hexchars[n[i] >> 4];
    s[i*2+1] = hexchars[n[i] & 15];
  }
  s[cnt*2+1]=0;
}

u8 readc() {
  while (!serial_available()) delay(1);
  return serial_read();
}

u32 readuint() {
  u32 n = 0;
  u8 c;
  do {
    c = readc();
    if (c == '\n') break;
    n = n * 10 + c - '0';
  } while (c != '\n');
  return n;
}

int hexchar2byte(u8 c) {
  if ('0'<=c && c<='9') return c-'0';
  if ('A'<=c && c<='F') return c-'A'+10;
  if ('a'<=c && c<='f') return c-'a'+10;
  return -1;
}

void hex2binary(u8* out, char* in, int bytecnt) {
  for (int i=0; i<bytecnt; i++) {
    out[i] = (hexchar2byte(in[i*2]) << 4) | hexchar2byte(in[i*2 + 1]);
  }
}

int readln(u8 *s, int maxlen) {
  int cnt = 0;
  u8 c;
  bzero(s, maxlen);
  while (1) {
    if (!serial_available()) {
      delay(1);
    } else {
      c = serial_read();
      if ((c == '\n') || (c == '\r')) break;
      if (cnt < maxlen) {
        s[cnt++] = c; // FIXME: long messages get truncated
      } else {
        break;
      }
    }
  }
  return cnt;
}

/* RANDOMBYTES - utility function for tweetnacl
 *
 * This is serious overkill because randombytes is only ever called with
 * a count argument value of 32, but better safe than sorry.  The strategy
 * here is to fill an entropy buffer with output from the HWRNG and hash
 * that a block at a time.  This code currently uses one word of raw entropy
 * per byte of output, so there is a safety margine of ~4x built in.
 *
 */

#define CHB crypto_hash_BYTES

void randomblock(u8 p[CHB]) {
  uint32_t r[CHB];
  for (int i=0; i<CHB; i++) r[i] = read_rng();
  crypto_hash(p, (u8*)r, CHB*sizeof(uint32_t));
}

void randombytes(u8 *p, uint32_t n) {
  if (n>0x1000) {
    bzero(p, n);
    return;
  }
  while (n>CHB) {
    randomblock(p);
    p += CHB;
    n -= CHB;
  }
  u8 h[CHB];
  randomblock(h);
  bcopy(h, p, n);
}

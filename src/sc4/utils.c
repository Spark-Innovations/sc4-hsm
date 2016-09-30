
#include "hardware.h"
#include "tweetnacl.h"
#include "utils.h"

// Regualar delay doesn't work in an interrupt handler
void delay1(int n) {
  volatile int i;
  for (i=0; i<n*100000; i++);
}

/* Output utilities */

void printc(char c) { serial_write(&c, 1); }

void serial_vprintf(const char* format, va_list args) {
  char buffer[1024];
  int n = vsnprintf(buffer, 1024, format, args);
  serial_write(buffer, n);
}

void serial_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  serial_vprintf(format, args);
  va_end(args);  
}

void serial_print(char* s) {
  while(*s) printc(*s++);
}

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


#define BPRINTF_BUFFER_SIZE 4096
char bprintf_buffer[BPRINTF_BUFFER_SIZE] = {0};
char *bprintf_ptr = bprintf_buffer;
int bprintf_size = BPRINTF_BUFFER_SIZE;

void bprintf_reset() {
  printf("bprint_size = %d %d\n", bprintf_size, bprintf_ptr-bprintf_buffer);
  serial_write(bprintf_buffer, bprintf_ptr-bprintf_buffer);
  bprintf_ptr = bprintf_buffer;
  bprintf_size = BPRINTF_BUFFER_SIZE;
  print("\nPush a button...");
  while(!user_buttons());
  for (int i=0; i<10; i++) while(user_buttons());
  print("\nOK\n");
}

void bprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vbprintf(format, args);
  va_end(args);
}

void vbprintf(const char *format, va_list args) {
  int n = vsnprintf(bprintf_ptr, bprintf_size, format, args);
  bprintf_size -= n;
  bprintf_ptr += n;
  *bprintf_ptr++ = '\n';
  *bprintf_ptr = '\0';
}

void bprinth(unsigned char* buf, int n) {
  sprinth(bprintf_ptr, buf, n);
  bprintf_size -= n;
  bprintf_ptr += n*2;
  *bprintf_ptr++ = '\n';
  *bprintf_ptr = '\0';
}

void vlcd_printf(const char *format, va_list args) {
  char buffer[256];
  vsnprintf(buffer, 256, format, args);
  lcd_print(buffer);
}

void lcd_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vlcd_printf(format, args);
  va_end(args);
}

/* Input utilities */

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

#ifndef __utils__
#define __utils__

#include <string.h>

#define print serial_print
#define printf serial_print

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char* string;

#ifdef __cplusplus
extern "C" {
#endif

  void randombytes(u8*, uint32_t);
  int readln(u8*, int);
  void printh(const u8*, int);
  void hex2binary(u8 *, char *, int);
  void sprinth(char *, const u8*, int);
  void newline(void);
  void lcd_print(string);

  void lcd_noise();
  void moire();
  void show_banner();
  void scheme_main();

#ifdef __cplusplus
}
#endif

#endif

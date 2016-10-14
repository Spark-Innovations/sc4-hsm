#ifndef __utils__
#define __utils__

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "hardware.h"
#include "tweetnacl-aux.h"

typedef char * string;

#define print serial_print
#define printf serial_printf

#ifdef __cplusplus
extern "C" {
#endif

  void randombytes(u8*, uint32_t);
  int readln(u8*, int);
  void printh(const u8*, int);
  void hex2binary(u8 *, char *, int);
  void sprinth(char *, const u8*, int);
  void newline(void);

  void system_reset();

  void serial_print(string);
  void serial_printf(const char*, ...);
  void serial_vprintf(const char*, va_list);

  void bprintf_reset();
  void bprintf(const char *, ...);
  void vbprintf(const char *, va_list);
  void bprinth(unsigned char* buf, int n);

  void lcd_print(string);
  void lcd_printf(const char *, ...);
  void vlcd_printf(const char *, va_list);

  void lcd_noise();
  void moire();
  void show_banner();
  void scheme_main();

  void delay1(int);

#ifdef __cplusplus
}
#endif

#endif

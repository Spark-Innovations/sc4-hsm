#ifndef LIBBASE58_H
#define LIBBASE58_H

#include <stdbool.h>
#include <stddef.h>

typedef unsigned char u8;

#ifdef __cplusplus
extern "C" {
#endif
  
  extern bool b58tobin(u8 *bin, size_t *binsz, const char *b58, size_t b58sz);
  extern bool b58enc(char *b58, size_t *b58sz, const u8 *bin, size_t binsz);

#ifdef __cplusplus
}
#endif

#endif

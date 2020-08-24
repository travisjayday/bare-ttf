#ifndef TTF_LIBC
#define TTF_LIBC

#define USE_GNUEFI
//#define USE_STDLIB


#ifdef USE_STDLIB
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#endif

#ifdef USE_GNUEFI
#include <efi.h>
#include <efistdarg.h>
#include <efibind.h>
#include <efilib.h>
#endif

void* ttf_malloc(uint32_t size);
void ttf_memcpy(void* dest, void* src, uint32_t n);
void ttf_free(void* ptr);

#endif

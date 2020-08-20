#ifndef TTF_UTILS_H
#define TTF_UTILS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

//#define VERBOSE

int16_t read_int16(uint8_t* base, uint32_t* cursor);

uint64_t read_uint64(uint8_t* base, uint32_t* cursor);
uint32_t read_uint32(uint8_t* base, uint32_t* cursor);
uint16_t read_uint16(uint8_t* base, uint32_t* cursor);
uint8_t read_uint8(uint8_t* base, uint32_t* cursor);

uint32_t calc_checksum(uint8_t *table, uint32_t len);

void ttf_log(const char *format, ...);
void ttf_log_r(const char *format, ...);

#endif

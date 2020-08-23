#ifndef TTF_UTILS_H
#define TTF_UTILS

#include "ttf_libc.h"

#define VERBOSE_PARSE
//#define VERBOSE_RASTR
//#define SILENCE_ERROR

int16_t read_int16(uint8_t* base, uint32_t* cursor);
uint64_t read_uint64(uint8_t* base, uint32_t* cursor);
uint32_t read_uint32(uint8_t* base, uint32_t* cursor);
uint16_t read_uint16(uint8_t* base, uint32_t* cursor);
uint8_t read_uint8(uint8_t* base, uint32_t* cursor);

uint32_t calc_checksum(uint8_t *table, uint32_t len);

void ttf_log(const char *format, ...);
void ttf_log_r(const char *format, ...);
void ttf_error(const char* format, ...);

#endif

#include "ttf_utils.h"

uint64_t read_uint64(uint8_t* base, uint32_t* cursor) {
    uint64_t res = 
          (uint64_t) base[*cursor + 0] << 56
        | (uint64_t) base[*cursor + 1] << 48
        | (uint64_t) base[*cursor + 2] << 40
        | (uint64_t) base[*cursor + 3] << 32
        | (uint64_t) base[*cursor + 4] << 24
        | (uint64_t) base[*cursor + 5] << 16 
        | (uint64_t) base[*cursor + 6] << 8 
        | (uint64_t) base[*cursor + 7];
    *cursor += 8;
    return res;
}

uint32_t read_uint32(uint8_t* base, uint32_t* cursor) {
    uint32_t res = 
          (uint32_t) base[*cursor + 0] << 24
        | (uint32_t) base[*cursor + 1] << 16 
        | (uint32_t) base[*cursor + 2] << 8 
        | (uint32_t) base[*cursor + 3];
    *cursor += 4;
    return res;
}

uint16_t read_uint16(uint8_t* base, uint32_t* cursor) {
    uint16_t res = 
          (uint16_t) base[*cursor + 0] << 8 
        | (uint16_t) base[*cursor + 1];
    *cursor += 2;
    return res;
}

uint8_t read_uint8(uint8_t* base, uint32_t* cursor) {
    uint8_t res = base[*cursor + 0];
    *cursor += 1;
    return res;
}

int16_t read_int16(uint8_t* base, uint32_t* cursor) {
    int16_t res = 
          base[*cursor + 0] << 8 
        | base[*cursor + 1];
    *cursor += 2;
    return res;
}

uint32_t calc_checksum(uint8_t *table, uint32_t len)
{
    uint32_t sum = 0;
    uint32_t cursor = 0;
    uint32_t padlen = ((len + 3) & ~3);
    while (cursor < padlen)
        sum += read_uint32(table, &cursor);
    return sum;
}

void ttf_log(const char *format, ...)
{
    va_list args;
    va_start(args, format);

#ifdef VERBOSE_PARSE

#ifdef USE_GNUEFI
    uint32_t p = 0;
    while (*(format + p++) != '\0');
    CHAR16 * _format = (CHAR16*) ttf_malloc(p * sizeof(CHAR16));
    p = 0; 
    do _format[p] = format[p];
    while (format[p++] != '\0');
    //ttf_free(_format);
    VPrint(_format, args);
#endif

#ifdef USE_STDLIB
    vprintf(format, args);
#endif

#endif
    va_end(args);
}

void ttf_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);

#ifdef VERBOSE_PARSE

#ifdef USE_GNUEFI
    uint32_t p = 0;
    while (*(format + p++) != '\0');
    CHAR16 * _format = (CHAR16*) ttf_malloc(p * sizeof(CHAR16));
    p = 0; 
    do _format[p] = format[p];
    while (format[p++] != '\0');
    //ttf_free(_format);
    VPrint(_format, args);
#endif

#ifdef USE_STDLIB
    vprintf(format, args);
#endif

#endif
    va_end(args);
}

void ttf_log_r(const char *format, ...)
{
    va_list args;
    va_start(args, format);

#ifdef VERBOSE_RASTR

#ifdef USE_GNUEFI
    uint32_t p = 0;
    while (*(format + p++) != '\0');
    CHAR16 * _format = (CHAR16*) ttf_malloc(p * sizeof(CHAR16));
    p = 0; 
    do _format[p] = format[p];
    while (format[p++] != '\0');
    //ttf_free(_format);
    VPrint(_format, args);
#endif

#ifdef USE_STDLIB
    vprintf(format, args);
#endif

#endif
    va_end(args);
}

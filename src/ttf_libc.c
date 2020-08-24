#include "ttf_libc.h"

void* ttf_malloc(uint32_t size) {
#ifdef USE_STDLIB
    return malloc(size);
#endif
#ifdef USE_GNUEFI
    return _malloc(size);
#endif
}

void ttf_memcpy(void* dest, void* src, uint32_t n) {
    uint8_t* d = dest;
    uint8_t* s = src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];  
}

void ttf_free(void* ptr) {
#ifdef USe_STDLIB
    free(ptr);
#endif
#ifdef USE_GNUEFI
    // TODO: Add free implementation
#endif
}

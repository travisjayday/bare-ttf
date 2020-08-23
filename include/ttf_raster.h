#ifndef TTF_RASTER
#define TTF_RASTER

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ttf_tables.h"
#include "ttf_utils.h"

typedef struct GLYF_PIXBUF {
    uint8_t* buf;
    uint16_t w;
    uint16_t h;
    int16_t shift_y;
    void (*free)(struct GLYF_PIXBUF*);
} GLYF_PIXBUF;

GLYF_PIXBUF* rasterize_glyf(TTF_GLYF* glyf, float scale);

#endif

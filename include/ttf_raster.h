#ifndef TTF_RASTER
#define TTF_RASTER

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ttf_tables.h"

typedef struct {
    float x0, y0, x1, y1;
    float m;
    float b;
    uint8_t hor;
    uint8_t ver; 
} _LINE;

typedef struct {
    uint8_t* buf;
    uint16_t w;
    uint16_t h;
} GLYF_PIXBUF;

void set_pixel(GLYF_PIXBUF* buf, float x, float y);

void q_bezier_curve(
        GLYF_PIXBUF* buffer, 
        float x0, float y0, /* the initial point */
        float x1, float y1, /* the control point */
        float x2, float y2); /* the final   point */

void line_to(
        GLYF_PIXBUF* buffer, 
        float x0, float y0, 
        float x1, float y1);

GLYF_PIXBUF* rasterize_glyf(TTF_GLYF* glyf, float scale);

#endif

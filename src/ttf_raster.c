#include "ttf_raster.h"

/* private structs used for rasterizing */
typedef struct {
    float x0, y0, x1, y1;
    float m;
    float b;
    uint8_t hor;
    uint8_t ver; 
} _LINE;

typedef struct {
    float x, y;
} _POINT;

#define add_line(_x0, _y0, _x1, _y1) \
        _LINE* l = (_LINE*) ttf_malloc(sizeof(_LINE)); \
        l->x0 = _x0; \
        l->y0 = _y0; \
        l->x1 = _x1; \
        l->y1 = _y1; \
        if (_x1 - _x0 == 0) l->ver = 1; \
        else l->ver = 0; \
        if (absdiff(_y1, _y0) <= 0.2) l->hor = 1; \
        else l->hor = 0; \
        l->m = (_y1 - _y0) / (_x1 - _x0); \
        lines[line_n++] = l;

#define add_curve(_x0, _y0, _x1, _y1, _x2, _y2) \
    float Bx = 0.0; \
    float By = 0.0;\
    float _ctx_x = 0.0;\
    float _ctx_y = 0.0; \
    _ctx_x = _x0; \
    _ctx_y = _y0; \
/*    add_line(_x0, _y0, _x2, _y2); */\
    for (float t = 0.2f; t <= 1; t += 0.2f) { \
        Bx = _x1 + (1.f - t) * (1.f - t) * (_x0 - _x1) + t * t * (_x2 - _x1); \
        By = _y1 + (1.f - t) * (1.f - t) * (_y0 - _y1) + t * t * (_y2 - _y1); \
        add_line(_ctx_x, _ctx_y, Bx, By); \
        _ctx_x = Bx; \
        _ctx_y = By; \
    }

// ret absolute difference 
float absdiff(float f1, float f2) {
    if (f1 > f2) return f1 - f2;
    else return f2 - f1;
}


// sort ascending
void sort(float* arr, uint16_t n)
{
    uint16_t i, j;
    float tmp;
    for (i = 0; i < n - 1; i++)
        for (j = 0; j < n - i - 1; j++)
            if (arr[j] > arr[j+1]) {
                tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
}


void free_pixbuf(GLYF_PIXBUF* px) {
    ttf_free(px->buf);
    ttf_free(px);
}


void printbuf(GLYF_PIXBUF* px) {
     for (uint16_t r = 0; r < px->h; r++) {
        for (uint16_t c = 0; c < px->w; c++) {
            uint8_t bri = px->buf[r * px->w + c];
            if (bri == 0xff) ttf_log_r("%c",'#');
            else if (bri > 0) ttf_log_r("%c",'%');
            else ttf_log_r(" ");
        }
        ttf_log_r("|\n");
    } 
}

void plot(GLYF_PIXBUF* px, uint32_t x, uint32_t y, float c) {
    if (y * px->w + x > px->w * px->h - 1) return;
    uint16_t bri = px->buf[y * px->w + x] + (uint8_t) (c * 255); 
    if (bri > 255) px->buf[y * px->w + x] = 255;
    else px->buf[y * px->w + x] = bri;
}

/* The following functions are used in Wu's Anti-Aliased line 
   drawing algorithm */ 

// integer part of x
uint32_t ipart(float x) {
    return (uint32_t) x;
}

uint32_t _round(float x) {
    return ipart(x + 0.5);
}

// fractional part of x
float fpart(float x) {
    return x - (uint32_t) (x);
}

float rfpart(float x) {
    return 1 - fpart(x);
}

void draw_line(GLYF_PIXBUF* px, float x0, float y0, float x1, float y1) {
    uint8_t steep = absdiff(y1, y0) > absdiff(x1, x0)? 1 : 0;

    float t = x0;
    if (steep) {
        x0 = y0;
        y0 = t;
        t = x1;
        x1 = y1;
        y1 = t;
    }
    if (x0 > x1) {
        t = x0;
        x0 = x1;
        x1 = t;
        t = y0;
        y0 = y1;
        y1 = t;
    }

    float dx = x1 - x0;
    float dy = y1 - y0;
    float gradient = dy / dx;
    if (dx == 0.0)
        gradient = 1.0;

    // handle first endpoint
    float xend = _round(x0);
    float yend = y0 + gradient * (xend - x0);
    float xgap = rfpart(x0 + 0.5);
    float xpxl1 = xend; // this will be used in the main loop
    float ypxl1 = ipart(yend);
    if (steep) { 
        plot(px, ypxl1,   xpxl1, rfpart(yend) * xgap);
        plot(px, ypxl1+1, xpxl1,  fpart(yend) * xgap);
    }
    else {
        plot(px, xpxl1, ypxl1  , rfpart(yend) * xgap);
        plot(px, xpxl1, ypxl1+1,  fpart(yend) * xgap);
    }
    float intery = yend + gradient; // first y-intersection for the main loop

    // handle second endpoint
    xend = _round(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fpart(x1 + 0.5);
    float xpxl2 = xend; //this will be used in the main loop
    float ypxl2 = ipart(yend);
    if (steep) {
        plot(px, ypxl2  , xpxl2, rfpart(yend) * xgap);
        plot(px, ypxl2+1, xpxl2,  fpart(yend) * xgap);
    }
    else {
        plot(px, xpxl2, ypxl2,  rfpart(yend) * xgap);
        plot(px, xpxl2, ypxl2+1, fpart(yend) * xgap);
    }

    // main loop
    if (steep) {
        for (uint16_t x = xpxl1 + 1; x <= xpxl2 - 1; x++) {
            plot(px, ipart(intery)  , x, rfpart(intery));
            plot(px, ipart(intery)+1, x,  fpart(intery));
            intery = intery + gradient;
        }
    }
    else {
        for (uint16_t x = xpxl1 + 1; x <= xpxl2 - 1; x++) {
            plot(px, x, ipart(intery),  rfpart(intery));
            plot(px, x, ipart(intery)+1, fpart(intery));
            intery = intery + gradient;
        }
    }
}
/* **** */ 

float min(float f1, float f2) {
    if (f1 < f2) return f1;
    return f2;
}

float max(float f1, float f2) {
    if (f1 > f2) return f1;
    return f2;
}

void _qswapl(_LINE** a, _LINE** b)
{
    _LINE* t = *a;
    *a = *b;
    *b = t;
}

void _qsortl(_LINE** arr, int16_t l, int16_t h) {
    if (l < h) {
        int16_t pi = l - 1;

        float pivot = min(arr[h]->y0, arr[h]->y1); 
        for (uint16_t j = l; j <= h - 1; j++) {
            float val = min(arr[j]->y0, arr[j]->y1);
            if (val < pivot) _qswapl(&arr[j], &arr[++pi]); 
        }
        _qswapl(&arr[h], &arr[++pi]);

        _qsortl(arr, l, pi - 1);
        _qsortl(arr, pi + 1, h);
    }
}

void raster_v2(GLYF_PIXBUF* pixbuf, _LINE** lines, uint16_t line_n, float scale_f) {
    uint16_t* active_elist = ttf_malloc(line_n * sizeof(uint16_t)); 
    float* x_inter = ttf_malloc(line_n * sizeof(float)); 
    uint16_t x_inter_n = 0;
    uint16_t active_elist_n = 0;
    uint16_t line_ptr = 0;

    _qsortl(lines, 0, line_n - 1);

    for (uint16_t i = 0; i < line_n; i++) {
        _LINE* l = lines[i];
        ttf_log_r("Segment((%f, %f), (%f, %f))\n", 
                    l->x0, l->y0, l->x1, l->y1);
    }
    ttf_log_r("\n");
    for (uint16_t i = 0; i < line_n; i++) ttf_log_r("Line((%f, %f) --> (%f, %f))\n", 
            lines[i]->x0, lines[i]->y0, lines[i]->x1, lines[i]->y1);


    for (uint16_t y = 0; y < pixbuf->h; y++) {
        ttf_log_r("before alloc\n");
 
        // add edges to active edgelist
        while (1) {
            ttf_log_r("line ptr: %d/%d\n", line_ptr, line_n);
            if (line_ptr >= line_n) break;
            _LINE* l = lines[line_ptr];

            if (min(l->y0, l->y1) < y) {
                
                // draw horizontal lines
                if (l->hor)
                    for (uint16_t x = min(l->x0, l->x1) + 2; x < max(l->x0, l->x1); x++) 
                        pixbuf->buf[y * pixbuf->w + x] = 0xff;            

                active_elist[active_elist_n++] = line_ptr;
                if (!l->ver)
                    x_inter[line_ptr] = l->x0 + (y - l->y0) / l->m;
                else
                    x_inter[line_ptr] = l->x0;
                line_ptr++;
            }
            else break;
        }
        ttf_log_r("after alloc\n");
 
        // remove edges from active edgelist
        for (uint16_t i = 0; i < active_elist_n; i++) {
            _LINE* l = lines[active_elist[i]];
            float lowest = max(l->y0, l->y1);
            if (lowest < y) {
                for (uint16_t j = i + 1; j < active_elist_n; j++)
                    active_elist[j - 1] = active_elist[j];
                active_elist_n--;
                i--;
            }
        }

        ttf_log_r("y = %d/%d\n", y, pixbuf->h);
        ttf_log_r("Active Edges: ");
        for (uint16_t i = 0; i < active_elist_n; i++) ttf_log_r("%d, ", active_elist[i]);
        ttf_log_r("\n");

        // bake sortex x intercepts
        float* x_ints = ttf_malloc(active_elist_n * sizeof(float));
        uint16_t m = 0;
        for (uint16_t i = 0; i < active_elist_n; i++) x_ints[m++] =  x_inter[active_elist[i]];
        sort(x_ints, m);


        ttf_log_r("X_intercepts: ");
        for (uint16_t i = 0; i < m; i++) ttf_log_r("%f, ", x_ints[i]);
        ttf_log_r("\n");

        // color intercept pairs
        for (uint16_t i = 0; i < m; i += 2) {
            if (i + 1 < m) {
                uint16_t x_start = x_ints[i];
                uint16_t x_end = x_ints[i + 1];
                for (uint16_t k = x_start + 1; k < x_end + 1; k++) {
                    pixbuf->buf[y * pixbuf->w + k] = 0xff;
                }
            }
        }

        
        for (uint16_t i = 0; i < active_elist_n; i++) {
            _LINE* l = lines[active_elist[i]];
            if (!l->ver) {
                x_inter[active_elist[i]] += 1 / l->m;
            } 
        }  

    }
}

void raster_v1(GLYF_PIXBUF* pixbuf, _LINE** lines, uint16_t line_n, float scale_f) {
    // points in blacklist are not allowed to be valid x intercepts for 
    // scanline rasterization
    _POINT** blacklist = (_POINT**) ttf_malloc((line_n + 1) * sizeof(_POINT*));
    uint16_t blacklist_n = 0;
    for (uint16_t i = 0; i < line_n; i++) {
        _LINE* li = lines[i];
        _LINE* lo = lines[(i + 1) % line_n];
        
        if (li->hor || lo->hor) continue;

        // add singular vertices at ^ or v
        if ((li->y0 <= li->y1 && lo->y1 <= li->y1) 
            || (li->y0 >= li->y1 && lo->y1 >= li->y1)) {
            _POINT* p = ttf_malloc(sizeof(_POINT));
            p->x = lines[i]->x1;
            p->y = lines[i]->y1;
            blacklist[blacklist_n++] = p;
        }
    }


    // Find x-intercepts between lines
    ttf_log_r("colected %d lines\n", line_n);
    for (uint16_t y = 0; y < pixbuf->h; y++) {
        float* x_ints = (float*) ttf_malloc((pixbuf->w) * sizeof(float));
        uint16_t x_ints_n = 0;
        ttf_log_r("Considering y=%d\n\n", y);
        for (uint16_t i = 0; i < line_n; i++) {
            _LINE* l = lines[i];
            ttf_log_r("Segment((%f, %f), (%f, %f))\n", 
                    l->x0, l->y0, l->x1, l->y1);
            if (!l->hor && !l->ver) {
                float x_start = l->x0 < l->x1? l->x0 : l->x1;
                float x_end = l->x0 < l->x1? l->x1 : l->x0;
                l->m = (l->y1 - l->y0) / (l->x1 - l->x0);
                ttf_log_r("simuating over [%f, %f] with m=%f\n", x_start, x_end, l->m);
                if (x_end - x_start < 0.005) continue;
                for (float x = x_start - 0.01; x < x_end; x += 0.005f / absdiff(l->m, 0)) {
                    float y_calc = l->m * (x - l->x0) + l->y0; 
                    //ttf_log_r("%.1f/%.1f ", y_calc, absdiff(y_calc, y) );
                    if (absdiff(y_calc, y) <= 0.0075f) {
                        x_ints[x_ints_n++] = x;
                        ttf_log_r("x_int: %f", x);
                        break;
                    }
                }
                ttf_log_r("\n");
            }
            if (l->ver) {
                // alternaing < vs <= to prevent dropout lines at 'I' for example
                if ((l->y1 > l->y0 && y < l->y1 && y >= l->y0) ||
                        (l->y1 < l->y0 && y < l->y0 && y >= l->y1)) {
                //ttf_log_r("vline at x=%f\n", l->x0);
                    x_ints[x_ints_n++] = l->x0;
                    ttf_log_r("xint: %f", l->x0);
                //ttf_log_r("vline at x=%f\n", x_ints[0]);
                }
            }
            if (l->hor && absdiff(l->y0, y) < 0.5f) {
                ttf_log_r("hline at y=%d from [%f, %f]\n", y, l->x0, l->x1);
                float x_start = l->x0 < l->x1? l->x0 : l->x1;
                float x_end = l->x0 < l->x1? l->x1 : l->x0;
 
                for (float i = x_start; i < x_end; i += 0.5f) {
                    pixbuf->buf[y * pixbuf->w + (uint16_t) (i + 0.5f)] = 0xff;
                }
                continue;
            }
        }
        // sort x intercepts to be paried in order
        sort(x_ints, x_ints_n);
        ttf_log_r("intercepts for y=%d: ", y);

        float dinc = 0.1f;
        do {
            float* filtered_x = (float*) ttf_malloc((pixbuf->w) * sizeof(float));
            uint16_t m = 0;

            for (uint16_t i = 0; i < x_ints_n; i++) {
                // filter out blacklisted vertices
                float x_i = x_ints[i];
                for (uint16_t j = 0; j < blacklist_n; j++) {
                    if (absdiff(y, blacklist[j]->y) < (0.05f + dinc)) {
                        float d2 = (x_i - blacklist[j]->x) 
                                 * (x_i - blacklist[j]->x);
                                 //+ (y - blacklist[j]->y) * 0.001
                                 // * (y - blacklist[j]->y);
                        if (absdiff(x_i, blacklist[j]->x) < (0.2f + dinc)) {
                            ttf_log_r("SKIPPING %f because of (%f, %f) on y=%d\n",
                                x_i, blacklist[j]->x, blacklist[j]->y, y);
                            goto blacklisted;
                        }
                    }
                }

                if (i + 1 < x_ints_n 
                    && absdiff(x_ints[i], x_ints[i + 1]) < ((0.5f + dinc)  / scale_f))
                        continue;
                else
                    filtered_x[m++] = x_ints[i];
blacklisted:
                continue;
            }
            ttf_free(x_ints);
            x_ints = filtered_x;
            x_ints_n = m;
            dinc += 0.1f;
        } while (x_ints_n % 2 != 0 && dinc < 10.f);

        if (x_ints_n % 2 != 0) ttf_log_r("GAVE UP");

        // scanline color between consecutive x intercepts
        for (uint16_t i = 0; i < x_ints_n; i += 2) {
            if (i + 1 < x_ints_n) {
                ttf_log_r("COLOR x=[%f, %f], ", x_ints[i], x_ints[i + 1]);
                uint16_t x_start = x_ints[i];
                uint16_t x_end = x_ints[i + 1];
                for (uint16_t k = x_start + 1; k < x_end + 1; k++) {
                    pixbuf->buf[y * pixbuf->w + k] = 0xff;
                }
            }
        }
        ttf_log_r("\n");
    }
}

// main rasterization function 
GLYF_PIXBUF* rasterize_glyf(TTF_GLYF* glyf, float scale_f) {

    ttf_log_r("Allocating for Glyf 0x%x", glyf);
    if (glyf->cont_n <= 0) return NULL; 

    float offset_x = -glyf->x_min;
    float offset_y = -glyf->y_min;
    int16_t x_min = glyf->x_min / scale_f;
    int16_t y_min = glyf->y_min / scale_f;
    int16_t x_max = (float) glyf->x_max / scale_f + 0.5f;
    int16_t y_max = (float) glyf->y_max / scale_f + 0.5f;

    GLYF_PIXBUF* pixbuf = (GLYF_PIXBUF*) ttf_malloc(sizeof(GLYF_PIXBUF));
    pixbuf->w = (float) (x_max - x_min + 1) + 1.5f;
    pixbuf->h = (float) (y_max - y_min + 1) + 0.5f;
    pixbuf->shift_y = -glyf->y_min / scale_f - pixbuf->h + 0.5f;
    pixbuf->buf = ttf_malloc(pixbuf->w * pixbuf->h * sizeof(uint8_t));
    pixbuf->free = &free_pixbuf;

    ttf_log_r("cont_n: %d\n; x: [%d, %d], y: [%d, %d]\n", glyf->cont_n, 
        glyf->x_min, glyf->x_max, glyf->y_min, glyf->y_max);

    for (uint16_t r = 0; r < pixbuf->h; r++) 
        for (uint16_t c = 0; c < pixbuf->w; c++) 
            pixbuf->buf[r * pixbuf->w + c] = 0;

    TTF_GLYF_SIMP_D* gdata = (TTF_GLYF_SIMP_D*) glyf->data;

    int16_t start_c = 0;
    uint16_t state = 0;
    uint16_t prev_x = 0;
    uint16_t prev_y = 0;
    float ctx_x = 0;
    float ctx_y = 0;
    float x = 0;
    float y = 0;
    float x0 = 0;
    float y0 = 0;
    ttf_log_r("grid-width: %d x %d, grid-x: [%d, %d], grid-y: [%d, %d], offset: [%f, %f]\n", 
            pixbuf->w, pixbuf->h, x_min, x_max, y_min, y_max, offset_x, offset_y);

    _LINE** lines = (_LINE**) ttf_malloc((gdata->coords_n * 2 * 2 + 1) * sizeof(_LINE*));
    uint16_t line_n = 0;

    for (uint16_t i = 0; i < gdata->coords_n; i++) {
        x = (gdata->x_coords[i] + offset_x) / scale_f;
        y = (gdata->y_coords[i] + offset_y) / scale_f;

        ttf_log_r("(%f, %f)[%c][%s]\t", x, y, 
                (gdata->flags[i] & F_IS_ENDPOINT) ? 'e' : ' ', 
                (gdata->flags[i] & F_ON_CURVE_POINT) ? "OC" : "CP");

        // new contour start
        if (state == 0) {
            ctx_x = x;
            ctx_y = y;
            state = 1;
        }
        else if (state == 1) {
            if (gdata->flags[i] & F_ON_CURVE_POINT) {
                add_line(ctx_x, ctx_y, x, y);
                //line_to(pixbuf, ctx_x, ctx_y, x, y);
                ctx_x = x;
                ctx_y = y;
            }
            else {
                // skip this point for now. This point will be
                // a control point (prev_x, prev_y) in the next loop
                state = 2;
            }
        }
        else {
            x0 = (gdata->x_coords[i - 1] + offset_x) / scale_f;
            y0 = (gdata->y_coords[i - 1] + offset_y) / scale_f;
            // finish bezier curve
            if (gdata->flags[i] & F_ON_CURVE_POINT) {
                //add_line(ctx_x, ctx_y, x, y);
                add_curve(ctx_x, ctx_y, x0, y0, x, y);
                ctx_x = x;
                ctx_y = y;
                state = 1;
            }
            // chain bezier curves
            else {
                //add_line(ctx_x, ctx_y, (x0 + x) / 2.f, (y0 + y) / 2.f);
                add_curve(ctx_x, ctx_y, x0, y0, (x0 + x) / 2.f, (y0 + y) / 2.f);
                ctx_x = (x0 + x) / 2.f; 
                ctx_y = (y0 + y) / 2.f; 
            }
        }

        // finish contour
        if (gdata->flags[i] & F_IS_ENDPOINT) {
            x0 = (gdata->x_coords[start_c] + offset_x) / scale_f;
            y0 = (gdata->y_coords[start_c] + offset_y) / scale_f;
            if (state == 2) {
                // finish bezier curve
                if (gdata->flags[start_c] & F_ON_CURVE_POINT) {
                    //add_line(ctx_x, ctx_y, x0, y0);
                    add_curve(ctx_x, ctx_y, x, y, x0, y0);
                    //add_curve(ctx_x, ctx_y, x, y, , (y0  y) / 2.f);
                }
                // chain bezier curve to finish
                else {
                    //add_line(ctx_x, ctx_y, (x0 + x) / 2.f, (y0 + y) / 2.f);
                    add_curve(ctx_x, ctx_y, x, y, (x0 + x) / 2.f, (y0 + y) / 2.f);
                }
            }
            else {
                // finish with line to start
                add_line(x, y, x0, y0);
            }
            start_c = i + 1;
            state = 0;
        }
    }
    
    raster_v2(pixbuf, lines, line_n, scale_f);

    // do anti-aliasing by drawing anti-aliased outlines
    for (uint16_t i = 0; i < line_n; i++) {
        _LINE* l = lines[i];
        draw_line(pixbuf, l->x0, l->y0, l->x1, l->y1);
    }
    
    // filter out black single pixel lines
    uint16_t last_start = 0;
    uint8_t black_count = 0;
    uint8_t counting = 0;
    for (uint16_t r = 1; r < pixbuf->h - 1; r++) {
        for (uint16_t c = 0; c < pixbuf->w; c++) {
            uint8_t bri = pixbuf->buf[r * pixbuf->w + c];
            uint8_t bri_above = pixbuf->buf[(r-1) * pixbuf->w + c];
            uint8_t bri_below = pixbuf->buf[(r+1) * pixbuf->w + c];
            if (bri > 10 && counting == 0 && bri_above < 10 && bri_below < 10) {
                counting = 1;
                last_start = c;
            }
            if (bri > 10 && counting != 0 && (bri_above > 10 || bri_below > 10)) {
                for (uint16_t i = last_start; i < c + 1; i++) {
                    pixbuf->buf[r * pixbuf->w + i] = 0x0;
                }
                counting = 0; 
                last_start = 0;
            }
        }
        ttf_log_r("\n|");
    } 

    // filter out white single pixel lines 
    last_start = 0;
    counting = 0;
    for (uint16_t r = 1; r < pixbuf->h - 1; r++) {
        for (uint16_t c = 0; c < pixbuf->w; c++) {
            uint8_t bri = pixbuf->buf[r * pixbuf->w + c];
            uint8_t bri_above = pixbuf->buf[(r-1) * pixbuf->w + c];
            uint8_t bri_below = pixbuf->buf[(r+1) * pixbuf->w + c];
            if (bri < 0xff && counting == 0 && bri_above > 30 && bri_below > 30) {
                counting = 1;
                last_start = c;
            }
            if (counting != 0 && (bri > 50 || bri_above < 10 || bri_below < 10)) {
                if (c + 1 - last_start + 1 > 4) {
                    for (uint16_t i = last_start; i < c + 1; i++) {
                        pixbuf->buf[r * pixbuf->w + i] = 0xff;
                    }
                }
                counting = 0; 
                last_start = 0;
            }
        }
        ttf_log_r("\n|");
    }
 

    // reflect the shape vertically 
    uint8_t* ref = ttf_malloc(pixbuf->w * pixbuf->h * sizeof(uint8_t));
    for (uint16_t r = 0; r < pixbuf->h; r++) {
        for (uint16_t c = 0; c < pixbuf->w; c++) {
            ref[(pixbuf->h - r - 1) * pixbuf->w + c] = pixbuf->buf[r * pixbuf->w + c];
        }
        ttf_log_r("\n|");
    }
    ttf_log_r("----------------");

    ttf_free(pixbuf->buf);
    pixbuf->buf = ref;

    // print glyfph
    printbuf(pixbuf);

    //for (uint16_t i = 0; i < blacklist_n; i++) ttf_free(blacklist[i]);
    //ttf_free(blacklist);

    for (uint16_t i = 0; i < line_n; i++) ttf_free(lines[i]);
    ttf_free(lines);

    // return glyf pixel buffer
    return pixbuf;
}

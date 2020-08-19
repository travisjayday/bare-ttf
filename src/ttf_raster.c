#include "ttf_raster.h"

#define add_line(_x0, _y0, _x1, _y1) \
        _LINE* l = (_LINE*) malloc(sizeof(_LINE)); \
        l->x0 = _x0; \
        l->y0 = _y0; \
        l->x1 = _x1; \
        l->y1 = _y1; \
        if (_x1 - _x0 == 0) l->ver = 1; \
        else l->ver = 0; \
        if (_y1 - _y0 == 0) l->hor = 1; \
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
    add_line(_x0, _y0, _x2, _y2); \
    /*for (float t = 0; t <= 1; t += 0.49f) { \
        Bx = _x1 + (1.f - t) * (1.f - t) * (_x0 - _x1) + t * t * (_x2 - _x1); \
        By = _y1 + (1.f - t) * (1.f - t) * (_y0 - _y1) + t * t * (_y2 - _y1); \
        add_line(_ctx_x, _ctx_y, Bx, By); \
        _ctx_x = Bx; \
        _ctx_y = By; \
    }*/
        

uint16_t pixbuf_w;
uint16_t pixbuf_h;
int16_t x_min;
int16_t y_min;

void set_pixel(GLYF_PIXBUF* px, float x, float y) {
    px->buf[(uint32_t)(y * px->w + x + 0.5f)] = 0xff;
}

void q_bezier_curve(GLYF_PIXBUF* px,
        float x0, float y0, /* the initial point */
        float x1, float y1, /* the control point */
        float x2, float y2) /* the final   point */
{
    float t = 0.f;
    float Bx = 0.f;
    float By = 0.f;
    while (t <= 1.0f) {
        Bx = x1 + (1.f - t) * (1.f - t) * (x0 - x1) + t * t * (x2 - x1);
        By = y1 + (1.f - t) * (1.f - t) * (y0 - y1) + t * t * (y2 - y1);
        set_pixel(px, Bx, By);
        t += 0.005f;
    }
    ttf_log_r("\nbezier_to: (%f, %f) -- [%f, %f] --> (%f, %f)\n", x0, y0, x1, y1, x2, y2);
}

void line_to(GLYF_PIXBUF* px, 
        float x0, float y0, 
        float x1, float y1) {
    float t = 0.f;
    float Bx = 0.f;
    float By = 0.f;
    while (t <= 1.0f) {
        Bx = (1.f - t) * x0 + t * x1;
        By = (1.f - t) * y0 + t * y1;
        set_pixel(px, Bx, By);
        t += 0.005f;
    }
    ttf_log_r("\nline_to: (%f, %f) --> (%f, %f)\n", x0, y0, x1, y1);
}

float fabs(float f1, float f2) {
    if (f1 > f2) return f1 - f2;
    else return f2 - f1;
}


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


void printbuf(GLYF_PIXBUF* px) {
     for (uint16_t r = 0; r < px->h; r++) {
        for (uint16_t c = 0; c < px->w; c++) {
            ttf_log_r("%c", px->buf[r * px->w + c] == 0xff? '#' : ' ');
        }
        ttf_log_r("\n|");
    } 
}


float _sqrt(float x)
{
    float guess = 1;
    while (fabs((guess * guess) / x - 1.0, 0.0f) >= 0.005)
        guess = ((x / guess) + guess) / 2.f;
    return guess;
}

float l_angle(_LINE* l_i, _LINE* l_o) {

    // These lines intersect at END(l_i) or START(l_o)

    /*printf("Line IN((%.2f, %.2f), (%.2f, %.2f))\n", 
                l_i->x0, l_i->y0, l_i->x1, l_i->y1); 
    printf("Line OUT((%.2f, %.2f), (%.2f, %.2f))\n", 
                l_o->x0, l_o->y0, l_o->x1, l_o->y1); */


    // v = Rev(Dir(line_in))
    // where line_in := (l_i->x0, l_i->y0) --> (l_i->x1, l_i->y1)
    float v_x = l_i->x0 - l_i->x1;
    float v_y = l_i->y0 - l_i->y1; 

    // u = Dir(line_out)
    // where line_out := (l_o->x0, l_o->y0) --> (l_o->x1, l_o->y1)
    float u_x = l_o->x1 - l_o->x0;
    float u_y = l_o->y1 - l_o->y0;

    // to test whether these lines intersect sharply, find
    // the angle between them, so (u*v)/(mag(u) * mag(v))
    float u_d_v = (u_x * v_x) + (u_y * v_y);
    float mag_u = _sqrt(u_x * u_x + u_y * u_y);
    float mag_v = _sqrt(v_x * v_x + v_y * v_y);
    float cos_t = u_d_v / (mag_u * mag_v);  

    return cos_t;
}

void plot(GLYF_PIXBUF* px, uint32_t x, uint32_t y, float c) {
    if (y * px->w + x > px->w * px->h - 1) return;
    uint16_t bri = px->buf[y * px->w + x] + (uint8_t) (c * 255); 
    if (bri > 255) px->buf[y * px->w + x] = 255;
    else px->buf[y * px->w + x] = bri;
}

// integer part of x
uint32_t ipart(float x) {
    return (uint32_t) x;
}

uint32_t round(float x) {
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
    uint8_t steep = fabs(y1, y0) > fabs(x1, x0)? 1 : 0;

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
    float xend = round(x0);
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
    xend = round(x1);
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

GLYF_PIXBUF* rasterize_glyf(TTF_GLYF* glyf, float scale_f) {

    if (glyf->cont_n <= 0) return NULL; 

    float offset_x = -glyf->x_min;
    float offset_y = -glyf->y_min;

    GLYF_PIXBUF* pixbuf = (GLYF_PIXBUF*) malloc(sizeof(GLYF_PIXBUF));
    x_min = glyf->x_min / scale_f;
    y_min = glyf->y_min / scale_f;
    int16_t x_max = (float) glyf->x_max / scale_f + 0.5f;
    int16_t y_max = (float) glyf->y_max / scale_f + 0.5f;

    pixbuf->w = (float) (x_max - x_min + 1) + 0.5f;
    pixbuf->h = (float) (y_max - y_min + 1) + 0.5f;
 

    pixbuf->buf = malloc(pixbuf->w * pixbuf->h * sizeof(uint8_t));

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

    _LINE** lines = (_LINE**) malloc(gdata->coords_n * 2 * 2 * sizeof(_LINE*));
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
                // q_bezier_curve(pixbuf, ctx_x, ctx_y, x0, y0, x, y);
                add_curve(ctx_x, ctx_y, x0, y0, x, y);
                ctx_x = x;
                ctx_y = y;
                state = 1;
            }
            // chain bezier curves
            else {
                //add_line(ctx_x, ctx_y, (x0 + x) / 2.f, (y0 + y) / 2.f);
                add_curve(ctx_x, ctx_y, x0, y0, (x0 + x) / 2.f, (y0 + y) / 2.f);
                /*q_bezier_curve( 
                        pixbuf,
                        ctx_x, ctx_y,
                        x0, y0, 
                        (x0 + x) / 2.f, (y0 + y) / 2.f);*/
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
                    add_line(ctx_x, ctx_y, x0, y0);
                    // q_bezier_curve(pixbuf, ctx_x, ctx_y, x, y, x0, y0);
                }
                // chain bezier curve
                else {
                    //add_line(ctx_x, ctx_y, (x0 + x) / 2.f, (y0 + y) / 2.f);
                    
             
                    /* q_bezier_curve(
                            pixbuf,
                            ctx_x, ctx_y, 
                            x, y, 
                            (x + x0) / 2.f, (y + y0) / 2.f);*/
                }
            }
            else {
                // finish with line to start
                add_line(x, y, x0, y0);
                //line_to(pixbuf, x, y, x0, y0);
            }
            start_c = i + 1;
            state = 0;
        }
    }

    
    _POINT** blacklist = (_POINT**) malloc((line_n + 1) * sizeof(_POINT*));
    uint16_t blacklist_n = 0;
    for (uint16_t i = 0; i < line_n; i++) {
        _LINE* li = lines[i];
        _LINE* lo = lines[(i + 1) % line_n];
        
        if () {
            _POINT* p = malloc(sizeof(_POINT));
            p->x = lines[i]->x1;
            p->y = lines[i]->y1;
            //blacklist[blacklist_n++] = p;
            //lines[li]->y1 -= .3333;
            printf("BLACKLISTING POINT (%f, %f)", p->x, p->y);
        }

        if (li->hor || lo->hor) continue;
        float cos_t = l_angle(li, lo);
        if (cos_t < 1.5707f && cos_t > 0) {
            _POINT* p = malloc(sizeof(_POINT));
            p->x = li->x1;
            p->y = li->y1;
            blacklist[blacklist_n++] = p;
            printf("POINT (%f, %f) is ACCUTE\n", p->x, p->y);
        }                                       
    }

    ttf_log_r("colected %d lines\n", line_n);
    fflush(stdout);
    for (uint16_t y = 0; y < pixbuf->h; y++) {
        float* x_ints = (float*) malloc((pixbuf->w) * sizeof(float));
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
                for (float x = x_start - 0.01; x < x_end; x += 0.005f / fabs(l->m, 0)) {
                    float y_calc = l->m * (x - l->x0) + l->y0; 
                    //ttf_log_r("%.1f/%.1f ", y_calc, fabs(y_calc, y) );
                    if (fabs(y_calc, y) <= 0.0075f) {
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
            if (l->hor && fabs(l->y0, y) < 0.5f) {
                ttf_log_r("hline at y=%d from [%f, %f]\n", y, l->x0, l->x1);
                float x_start = l->x0 < l->x1? l->x0 : l->x1;
                float x_end = l->x0 < l->x1? l->x1 : l->x0;
 
                for (float i = x_start; i < x_end; i += 0.5f) {
                    pixbuf->buf[y * pixbuf->w + (uint16_t) (i + 0.5f)] = 0xff;
                }
                continue;
            }
        }
        sort(x_ints, x_ints_n);
        ttf_log_r("intercepts for y=%d: ", y);

        float* filtered_x = (float*) malloc((pixbuf->w) * sizeof(float));

        uint16_t m = 0;
        for (uint16_t i = 0; i < x_ints_n; i++) {
            
            // filter out accute vertices (blacklisted)
            float x_i = x_ints[i];
            for (uint16_t j = 0; j < blacklist_n; j++) {
                if (fabs(y, blacklist[j]->y) < 0.0000001) {
                    if (fabs(x_i, blacklist[j]->x) < 0.1)  {
                        printf("Blacked (%f, %f) %f/%d,%f", blacklist[j]->x, blacklist[j]->y, x_i, y, blacklist[j]->y);
                    goto blacklisted;
                    }
                }
                float d2 = (x_i - blacklist[j]->x) 
                         * (x_i - blacklist[j]->x)
                         + (y - blacklist[j]->y) * 0.001
                         * (y - blacklist[j]->y);
                if (d2 < 0.00005f) {
                    printf("SKIPPING %f because of (%f, %f) on y=%d\n",
                        x_i, blacklist[j]->x, blacklist[j]->y, y);
                    goto blacklisted;
                }
            }

            // filter out points that are very close to each other
            float d = fabs(x_ints[i], x_ints[i + 1]);
            if (fabs(x_ints[i], x_ints[i + 1]) < 0.333 / scale_f) continue;
            else
                filtered_x[m++] = x_ints[i];
blacklisted:
            0 == 0;
        }

        x_ints = filtered_x;
        x_ints_n = m;
        for (uint16_t i = 0; i < x_ints_n; i += 2) {
            if (i - 1 < x_ints_n) {
                if (fabs(x_ints[i], x_ints[i + 1]) < 0.1f) { 
                    ttf_log_r("SKIPPING CLOSE PointS: %f, %f", 
                            x_ints[i], x_ints[i + 1]);
                    i--;
                    continue;
                }
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

    for (uint16_t i = 0; i < line_n; i++) {
        _LINE* l = lines[i];
        draw_line(pixbuf, l->x0, l->y0, l->x1, l->y1);
    }

    uint8_t* ref = malloc(pixbuf->w * pixbuf->h * sizeof(uint8_t));

    for (uint16_t r = 0; r < pixbuf->h; r++) {
        for (uint16_t c = 0; c < pixbuf->w; c++) {
            ref[(pixbuf->h - r - 1) * pixbuf->w + c] = pixbuf->buf[r * pixbuf->w + c];
        }
        ttf_log_r("\n|");
    }
    ttf_log_r("----------------");

    pixbuf->buf = ref;

    printbuf(pixbuf);

    ttf_log_r("fins");
    fflush(stdout);

    return pixbuf;
}

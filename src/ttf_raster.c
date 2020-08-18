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
    printf("\nbezier_to: (%f, %f) -- [%f, %f] --> (%f, %f)\n", x0, y0, x1, y1, x2, y2);
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
    printf("\nline_to: (%f, %f) --> (%f, %f)\n", x0, y0, x1, y1);
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



GLYF_PIXBUF* rasterize_glyf(TTF_GLYF* glyf, float scale_f) {

    if (glyf->cont_n <= 0) return NULL; 

    float offset_x = glyf->x_min > 0? -glyf->x_min : glyf->x_min;
    float offset_y = glyf->y_min > 0? -glyf->y_min : glyf->y_min;
    //offset_y += 20;

    GLYF_PIXBUF* pixbuf = (GLYF_PIXBUF*) malloc(sizeof(GLYF_PIXBUF));
    x_min = glyf->x_min / scale_f;
    y_min = glyf->y_min / scale_f;
    int16_t x_max = (float) glyf->x_max / scale_f + 0.5f;
    int16_t y_max = (float) glyf->y_max / scale_f + 0.5f;

    pixbuf->w = (float) (x_max - x_min + 1) + 0.5f;
    pixbuf->h = (float) (y_max - y_min + 1) + 0.5f;
 

    pixbuf->buf = malloc(pixbuf->w * pixbuf->h * sizeof(uint8_t));

    printf("cont_n: %d\n; x: [%d, %d], y: [%d, %d]\n", glyf->cont_n, 
        glyf->x_min, glyf->x_max, glyf->y_min, glyf->y_max);

    for (uint16_t r = 0; r < pixbuf->h; r++) 
        for (uint16_t c = 0; c < pixbuf->w; c++) 
            pixbuf->buf[r * pixbuf->w + c] = '0';

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
    printf("grid-width: %d x %d, grid-x: [%d, %d], grid-y: [%d, %d], offset: [%f, %f]\n", 
            pixbuf->w, pixbuf->h, x_min, x_max, y_min, y_max, offset_x, offset_y);

    _LINE** lines = (_LINE**) malloc(100 * sizeof(_LINE*));
    uint16_t line_n = 0;

    for (uint16_t i = 0; i < gdata->coords_n; i++) {
        x = (gdata->x_coords[i] + offset_x) / scale_f;
        y = (gdata->y_coords[i] + offset_y) / scale_f;

        printf("(%f, %f)[%c][%s]\t", x, y, 
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
                add_line(ctx_x, ctx_y, x, y);
                // q_bezier_curve(pixbuf, ctx_x, ctx_y, x0, y0, x, y);
                ctx_x = x;
                ctx_y = y;
                state = 1;
            }
            // chain bezier curves
            else {
                add_line(ctx_x, ctx_y, (x0 + x) / 2.f, (y0 + y) / 2.f);
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
                    add_line(ctx_x, ctx_y, (x0 + x) / 2.f, (y0 + y) / 2.f);
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

    printf("colected %d lines\n", line_n);
    fflush(stdout);
    for (uint16_t y = 0; y < pixbuf->h; y++) {
        float* x_ints = (float*) malloc((pixbuf->w) * sizeof(float));
        uint16_t j = 0;
        printf("Considering y=%d\n\n", y);
        for (uint16_t i = 0; i < line_n; i++) {
            _LINE* l = lines[i];
            printf("Segment((%f, %f), (%f, %f))\n", 
                    l->x0, l->y0, l->x1, l->y1);
            if (!l->hor && !l->ver) {
                float x_start = l->x0 < l->x1? l->x0 : l->x1;
                float x_end = l->x0 < l->x1? l->x1 : l->x0;
                l->m = (l->y1 - l->y0) / (l->x1 - l->x0);
                printf("simuating over [%f, %f] with m=%f\n", x_start, x_end, l->m);
                for (float x = x_start; x < x_end; x += 0.0005f) {
                    float y_calc = l->m * (x - l->x0) + l->y0; 
                    //printf("%.1f ", y_calc);
                    if (fabs(y_calc, y) < 0.005f) {
                        x_ints[j++] = x;
                        printf("x_int: %f", x);
                        break;
                    }
                }
                printf("\n");
            }
            if (l->ver) {
                // alternaing < vs <= to prevent dropout lines at 'I' for example
                if ((l->y1 > l->y0 && y < l->y1 && y >= l->y0) ||
                        (l->y1 < l->y0 && y < l->y0 && y >= l->y1)) {
                //printf("vline at x=%f\n", l->x0);
                    x_ints[j++] = l->x0;
                    printf("xint: %f", l->x0);
                //printf("vline at x=%f\n", x_ints[0]);
                }
            }
            if (l->hor && fabs(l->y0, y) < 0.5f) {
                printf("hline at y=%d from [%f, %f]\n", y, l->x0, l->x1);
                float x_start = l->x0 < l->x1? l->x0 : l->x1;
                float x_end = l->x0 < l->x1? l->x1 : l->x0;
 
                for (float i = x_start; i < x_end; i += 0.5f) {
                    //pixbuf->buf[y * pixbuf->w + (uint16_t) (i + 0.5f)] = 0xff;
                }
                continue;
            }
        }
        sort(x_ints, j);
        printf("intercepts for y=%d: ", y);

        float* filtered_x = (float*) malloc((pixbuf->w) * sizeof(float));
        uint16_t m = 0;
        for (uint16_t i = 0; i < j; i++) {
            float d = fabs(x_ints[i], x_ints[i + 1]);
            if (fabs(x_ints[i], x_ints[i + 1]) > 0.01f) 
                filtered_x[m++] = x_ints[i];
        }
        x_ints = filtered_x;
        for (uint16_t i = 0; i < j; i += 2) {
            if (i - 1 < j) {
                if (fabs(x_ints[i], x_ints[i + 1]) < 0.1f) { 
                    printf("SKIPPING CLOSE PointS: %f, %f", 
                            x_ints[i], x_ints[i + 1]);
                    i--;
                    continue;
                }
                printf("COLOR x=[%f, %f], ", x_ints[i], x_ints[i + 1]);
                uint16_t x_start = x_ints[i];
                uint16_t x_end = x_ints[i + 1];
                for (uint16_t k = x_start; k < x_end + 1; k++) {
                    pixbuf->buf[y * pixbuf->w + k] = 0xff;
                }
            }
        }
        printf("\n");
    }


    uint8_t* ref = malloc(pixbuf->w * pixbuf->h * sizeof(uint8_t));

    for (uint16_t r = 0; r < pixbuf->h; r++) {
        for (uint16_t c = 0; c < pixbuf->w; c++) {
            ref[(pixbuf->h - r - 1) * pixbuf->w + c] = pixbuf->buf[r * pixbuf->w + c];
        }
        printf("\n|");
    }
    printf("----------------");

    pixbuf->buf = ref;

    for (uint16_t r = 0; r < pixbuf->h; r++) {
        for (uint16_t c = 0; c < pixbuf->w; c++) {
            printf("%c", pixbuf->buf[r * pixbuf->w + c]);
        }
        printf("\n|");
    }

    printf("fins");
    fflush(stdout);

    return pixbuf;
}

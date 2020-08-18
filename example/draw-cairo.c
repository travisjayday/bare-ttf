#include <gtk/gtk.h>
#include <cairo.h>
#include "ttf_reader.h"

#define DEFAULT_WIDTH  800
#define DEFAULT_HEIGHT 800

static void paint (GtkWidget      *widget,
		   GdkEventExpose *eev,
                   gpointer        data);

gint main (gint argc, gchar **argv) {
    GtkWidget *window;
    GtkWidget *canvas;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (gtk_main_quit), NULL);

    canvas = gtk_drawing_area_new ();

    gtk_widget_set_size_request (canvas, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    TTF_FONT* font = NULL;
    extract_font_from_file("nobuntu.ttf", &font);

    printf("Font data loaded at 0x%x", font);

    g_signal_connect (G_OBJECT (canvas), "expose-event",
                    G_CALLBACK (paint),
                    font
                  );

    gtk_container_add (GTK_CONTAINER (window), canvas);

    gtk_widget_show_all (window);

    gtk_main ();
    return 0;
}

void set_pixel(cairo_t* cr, uint16_t x, uint16_t y) {
    cairo_rectangle (cr, x, y, 1, 1);
    cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
    cairo_fill (cr);
}

void q_bezier_curve(cairo_t* cr, 
        uint16_t x0, uint16_t y0, /* the initial point */
        uint16_t x1, uint16_t y1, /* the control point */
        uint16_t x2, uint16_t y2) /* the final   point */
{
    float t = 0.f;
    float Bx = 0.f;
    float By = 0.f;
    while (t <= 1.0f) {
        Bx = x1 + (1.f - t) * (1.f - t) * (x0 - x1) + t * t * (x2 - x1);
        By = y1 + (1.f - t) * (1.f - t) * (y0 - y1) + t * t * (y2 - y1);
        set_pixel(cr, Bx, By);
        t += 0.005f;
    }
    printf("\nbezier_to: (%d, %d) -- [%d, %d] --> (%d, %d)\n", x0, y0, x1, y1, x2, y2);
}

void line_to(cairo_t* cr, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    float t = 0.f;
    float Bx = 0.f;
    float By = 0.f;
    while (t <= 1.0f) {
        Bx = (1.f - t) * x0 + t * x1;
        By = (1.f - t) * y0 + t * y1;
        set_pixel(cr, Bx, By);
        t += 0.005f;
    }
    printf("\nline_to: (%d, %d) --> (%d, %d)\n", x0, y0, x1, y1);
}


/* the actual function invoked to paint the canvas
 * widget, this is where most cairo painting functions
 * will go
 */
static void paint (GtkWidget *widget, GdkEventExpose *eev, gpointer data) {
    gint width, height;
    gint i;
    cairo_t *cr;

    width  = widget->allocation.width;
    height = widget->allocation.height;

    cr = gdk_cairo_create (widget->window);
  
    /* clear background */
    cairo_set_source_rgb (cr, 1,1,1);
    cairo_paint (cr);

    TTF_FONT* font = (TTF_FONT*) data;
    printf("Font data loaded at 0x%x", font);
    fflush(stdout);

    cairo_set_source_rgb(cr, 0.42, 0.65, 0.80);
    cairo_set_line_width(cr, 1);

    ttf_log("Found %d gyfs\n", font->glyfs_n);
    ttf_log("Loa35d glyfs at 0x%x\n", font->glyfs);
    uint8_t f = 68;

    for (uint32_t g = f; g < font->glyfs_n; g++) {
        TTF_GLYF* glyf = font->glyfs[g];
 
        if (glyf->cont_n <= 0) continue; 

        printf("cont_n: %d\n; x: [%d, %d], y: [%d, %d]\n", glyf->cont_n, 
            glyf->x_min, glyf->x_max, glyf->y_min, glyf->y_max);

        TTF_GLYF_SIMP_D* gdata = (TTF_GLYF_SIMP_D*) glyf->data;

        int16_t start_c = 0;
        uint16_t state = 0;
        uint16_t prev_x = 0;
        uint16_t prev_y = 0;
        uint16_t ctx_x = 0;
        uint16_t ctx_y = 0;
        uint16_t x = 0;
        uint16_t y = 0;
        uint16_t x0 = 0;
        uint16_t y0 = 0;
        uint16_t offset_x = 0;
        uint16_t offset_y = 30;
        uint16_t scale_f = 1;

        for (uint16_t i = 0; i < gdata->coords_n; i++) {
            x = (gdata->x_coords[i] + offset_x) / scale_f;
            y = (gdata->y_coords[i] + offset_y) / scale_f;

            printf("(%d, %d)[%c][%s]\t", x, y, 
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
                    line_to(cr, ctx_x, ctx_y, x, y);
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
                prev_x = (gdata->x_coords[i - 1] + offset_x) / scale_f;
                prev_y = (gdata->y_coords[i - 1] + offset_y) / scale_f;
                // finish bezier curve
                if (gdata->flags[i] & F_ON_CURVE_POINT) {
                    q_bezier_curve(cr, ctx_x, ctx_y, prev_x, prev_y, x, y);
                    ctx_x = x;
                    ctx_y = y;
                    state = 1;
                }
                // chain bezier curves
                else {
                    q_bezier_curve(cr, 
                            ctx_x, ctx_y,
                            prev_x, prev_y, 
                            (prev_x + x) / 2, (prev_y + y) / 2);
                    ctx_x = (prev_x + x) / 2; 
                    ctx_y = (prev_y + y) / 2; 
                }
            }

            // finish contour
            if (gdata->flags[i] & F_IS_ENDPOINT) {
                x0 = (gdata->x_coords[start_c] + offset_x) / scale_f;
                y0 = (gdata->y_coords[start_c] + offset_y) / scale_f;
                if (state == 2) {
                    // finish bezier curve
                    if (gdata->flags[start_c] & F_ON_CURVE_POINT) 
                        q_bezier_curve(cr, ctx_x, ctx_y, x, y, x0, y0);
                    // chain bezier curve
                    else 
                        q_bezier_curve(cr, 
                                ctx_x, ctx_y, 
                                x, y, 
                                (x + x0) / 2, (y + y0) / 2);
                }
                else 
                    // finish with line to start
                    line_to(cr, x, y, x0, y0);
                start_c = i + 1;
                state = 0;
            }
        }
        for (uint16_t i = 0; i < gdata->coords_n; i++) {
            int16_t x = gdata->x_coords[i] + offset_x;
            int16_t y = gdata->y_coords[i] + offset_y;
            uint8_t is_endpt = gdata->flags[i] & F_IS_ENDPOINT;
            uint8_t is_oncur = gdata->flags[i] & F_ON_CURVE_POINT;

            cairo_rectangle (cr, x, y, 10, 10);
            cairo_set_source_rgb (cr, is_oncur == 0? 0.5 : 0.0, is_oncur == 0? 1.0 : 0.0, is_oncur == 0? 1.0 : 0.0);  // green
            cairo_fill (cr);

        }
 
        printf("\n\n");
        if (g == f) break;
    }


    cairo_destroy (cr);
}


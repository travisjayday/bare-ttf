#include <gtk/gtk.h>
#include <cairo.h>
#include "ttf_parser.h"
#include "ttf_raster.h"

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

    // Load the TTF font from filename 
    TTF_FONT* font = NULL;
    extract_font_from_file(argv[1], &font);

    g_signal_connect (G_OBJECT (canvas), "expose-event",
                    G_CALLBACK (paint),
                    font
                  );

    gtk_container_add (GTK_CONTAINER (window), canvas);

    gtk_widget_show_all (window);

    gtk_main ();
    return 0;
}

uint8_t drawed = 0;

void cr_set_pixel(cairo_t* cr, uint16_t x, uint16_t y) {
    cairo_rectangle (cr, x, y, 1, 1);
    cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
    cairo_fill (cr);
}

void draw_glyf(cairo_t* cr, GLYF_PIXBUF* pixbuf, uint16_t x, uint16_t y) {
    for (uint16_t r = 0; r < pixbuf->h; r++) {
        for (uint16_t c = 0; c < pixbuf->w; c++) {
            uint8_t bri = pixbuf->buf[r * pixbuf->w + c];
            cairo_rectangle (cr, c + x, r + y, 1, 1);
            cairo_set_source_rgb (cr, bri / 255.f, bri / 255.f, bri / 255.f);  // green
            cairo_fill (cr);
        }
    }
}

void draw_string(cairo_t* cr, char* str, uint16_t x, uint16_t y, TTF_FONT* font, float size) {
    for (uint8_t i = 0; i < strlen(str); i++) {
        TTF_GLYF* glyf = glyf_from_char(font, str[i]); 
        GLYF_PIXBUF* pixbuf = NULL;
        if (glyf == NULL) { 
            printf("Crourrpeted cahr");
            continue;
        }
        if (glyf->cont_n != 0)
            pixbuf = rasterize_glyf(glyf, font->head->units_per_em / size);
        else 
            printf("Skipping char");

        uint16_t rsb = glyf->rsb / (font->head->units_per_em / size) + 0.5f;
        uint16_t lsb = glyf->lsb / (font->head->units_per_em / size) + 0.5f;

        x += lsb;

        if (glyf->cont_n != 0) {
            draw_glyf(cr, pixbuf, x, y + pixbuf->shift_y);
            printf("%c -> y:[%d, %d] -> v_shift: %d, bearings: [%d, %d]\n", 
                str[i], glyf->y_min, glyf->y_max, pixbuf->shift_y, glyf->lsb, rsb);
            x += pixbuf->w;
            pixbuf->free(pixbuf);
        }
        x += rsb;
    }
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
    cairo_set_source_rgb (cr, 0,0,0);
    cairo_paint (cr);

    TTF_FONT* font = (TTF_FONT*) data;

    if (drawed == 0) {
        drawed = 1;
        draw_string(cr, "The quick brown fox jumps over the hedge!", 20, 120, font, 30.0f);
        draw_string(cr, "Hellow, world!", 20, 220, font, 80.0f);
        draw_string(cr, "abcdefghijklmnopqrstuvwxyz1234567890[]/&,.!\\?", 20, 320, font, 25.0f);
        draw_string(cr, "Hellow, world!", 20, 420, font, 100.0f);
        font->free(font);
    }
    cairo_destroy (cr);
}


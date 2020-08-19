#include <gtk/gtk.h>
#include <cairo.h>
#include "ttf_reader.h"
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

    TTF_FONT* font = NULL;
    extract_font_from_file("comic.ttf", &font);

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

uint8_t drawed = 0;

void cr_set_pixel(cairo_t* cr, uint16_t x, uint16_t y) {
    cairo_rectangle (cr, x, y, 1, 1);
    cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
    cairo_fill (cr);
}

void draw_glyf(cairo_t* cr, GLYF_PIXBUF* pixbuf, uint16_t x, uint16_t y) {
    for (uint16_t r = 0; r < pixbuf->h; r++) {
        for (uint16_t c = 0; c < pixbuf->w; c++) {
            uint8_t bri = 255 - pixbuf->buf[r * pixbuf->w + c];
            cairo_rectangle (cr, c + x, r + y, 1, 1);
            cairo_set_source_rgb (cr, bri / 255.f, bri / 255.f, bri / 255.f);  // green
            cairo_fill (cr);
        }
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
    cairo_set_source_rgb (cr, 1,1,1);
    cairo_paint (cr);

    TTF_FONT* font = (TTF_FONT*) data;
    printf("Font data loaded at 0x%x", font);
    fflush(stdout);

    cairo_set_source_rgb(cr, 0.42, 0.65, 0.80);
    cairo_set_line_width(cr, 1);

    if (drawed == 0) {
        drawed = 1;
        ttf_log("Found %d gyfs\n", font->glyfs_n);
        ttf_log("Loa35d glyfs at 0x%x\n", font->glyfs);
        uint8_t f = 19;

        TTF_GLYF* glyf = font->glyfs[35];
        GLYF_PIXBUF* pixbuf = rasterize_glyf(glyf, 12.0);
        printf("Rasterized glyf size %d x %d", pixbuf->w, pixbuf->h);
        draw_glyf(cr, pixbuf, 20, 20);
        

       for (uint32_t g = f; g < f + 80; g++) {
           TTF_GLYF* glyf = font->glyfs[g];
           printf("GLYF #%d", g);
            GLYF_PIXBUF* pixbuf = rasterize_glyf(glyf, 30.0);
            if (pixbuf == NULL) continue;
            printf("Rasterized glyf size %d x %d", pixbuf->w, pixbuf->h);
            draw_glyf(cr, pixbuf, 20 + ((g - f) * 85) % 700, 20 + ((g - f) / 10) * 100);
        }

    }

    


    cairo_destroy (cr);
}


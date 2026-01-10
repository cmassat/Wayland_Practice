
#define _GNU_SOURCE

#include "common.h"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include <cairo/cairo.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <linux/input-event-codes.h>
#include "keyboard.h"
#include "cube.h"
#include "audio.h"
#include "wayland_client.h"
#include <time.h>

int running = 1;


int main(void)
{
    struct timespec last_ts;
    clock_gettime(CLOCK_MONOTONIC, &last_ts);

    struct window *win = wayland_create_window();
    void *data   = win->data;
    int stride   = win->stride;
    static void *last_data = NULL;
    cairo_surface_t *cairo_surface =
        cairo_image_surface_create_for_data(
            data,
            CAIRO_FORMAT_ARGB32,
            win->width, win->height,
            stride);

    cairo_t *cr = cairo_create(cairo_surface);

    
    //audio_start("music.mp3");
    float angle = 0.0f;
    while (running) {

        struct timespec now;
clock_gettime(CLOCK_MONOTONIC, &now);

double dt =
    (now.tv_sec  - last_ts.tv_sec) +
    (now.tv_nsec - last_ts.tv_nsec) * 1e-9;

last_ts = now;


    wayland_pump_events();

    if (win->buffer_dirty) {

    if (win->buffer) {
        wayland_destroy_buffer();
    }
    win->buffer = waylen_create_buffer(win, win->width, win->height);
    win->buffer_dirty = false;

    if (cr) {
        cairo_destroy(cr);
        
    }
    if (cairo_surface) { 
        cairo_surface_destroy(cairo_surface);
        
    }

    cairo_surface =
        cairo_image_surface_create_for_data(
            win->data,
            CAIRO_FORMAT_ARGB32,
            win->width,
            win->height,
            win->stride);

    cr = cairo_create(cairo_surface);
     /* FORCE first paint immediately */
   // cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
   // cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
   // cairo_paint(cr);
   
}

    //this clears the window
    // cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    // cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    // cairo_paint(cr);


    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    draw(win->width, win->height, cr, angle);

    //cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // white
    cairo_select_font_face(cr,
                        "Sans",
                        CAIRO_FONT_SLANT_NORMAL,
                        CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 24);

    cairo_move_to(cr, 20, 40);
    cairo_show_text(cr, "Hello Wayland");


    cairo_surface_flush(cairo_surface);

    wayland_render_frame(win);

    const double speed = 0.8; // radians per second
    angle += speed * dt;
    
}

    keyboard_destroy();
    wayland_destroy_buffer();
    audio_stop();
    return 0;
}

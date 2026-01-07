
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

int running = 1;

int main(void)
{
    struct window win = wayland_create_window();
    void *data = win.data;
    int width = win.width;
    int height = win.height;
    int stride = win.stride;
    cairo_surface_t *cairo_surface =
        cairo_image_surface_create_for_data(
            data,
            CAIRO_FORMAT_ARGB32,
            width, height,
            stride);

    cairo_t *cr = cairo_create(cairo_surface);

    
    audio_start("music.mp3");
    float angle = 0.0f;
    while (running && wayland_dispatch_complete()) {
        
        draw(width, height, cr, angle);
        wayland_render_frame();
        angle += 0.01f;
        usleep(16000); // ~60 FPS

    }
    keyboard_destroy();
    wayland_destroy_buffer();
    audio_stop();
    return 0;
}

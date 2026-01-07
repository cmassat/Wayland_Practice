#pragma once
#include <wayland-client.h>

typedef void (*quit_callback_t)(void *userdata);

void keyboard_init(struct wl_seat *seat,
                   struct wl_display *display,
                   quit_callback_t quit_cb,
                   void *userdata);

void keyboard_destroy(void);

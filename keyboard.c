#define _GNU_SOURCE

#include "keyboard.h"
#include <linux/input-event-codes.h>
#include <unistd.h>

static struct wl_keyboard *keyboard;
static quit_callback_t quit_callback;
static void *quit_userdata;


static void keyboard_keymap(void *data,
    struct wl_keyboard *keyboard,
    uint32_t format, int fd, uint32_t size)
{
    close(fd);
}

static void keyboard_enter(void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    struct wl_surface *surface,
    struct wl_array *keys)
{
}

static void keyboard_leave(void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    struct wl_surface *surface)
{
}

static void keyboard_key(void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial, uint32_t time,
    uint32_t key, uint32_t state)
{
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        if (key == KEY_ESC) {
            quit_callback(quit_userdata);
        }
    }
}

static void keyboard_modifiers(void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group)
{
}

static const struct wl_keyboard_listener listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
};

void keyboard_init(struct wl_seat *seat,
                   struct wl_display *display,
                   quit_callback_t quit_cb,
                   void *userdata)
{
    quit_callback = quit_cb;
    quit_userdata = userdata;

    keyboard = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(keyboard, &listener, NULL);
}

void keyboard_destroy(void)
{
    if (keyboard)
        wl_keyboard_destroy(keyboard);
}
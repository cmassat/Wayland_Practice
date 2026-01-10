#define _GNU_SOURCE

#include "common.h"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "keyboard.h"
#include <stdlib.h>
#include <time.h>

static struct wl_display     *display;
static struct wl_compositor  *compositor;
static struct wl_shm         *shm;
static struct xdg_wm_base    *wm_base;
struct wl_surface *surface;
//static void *data;

static struct wl_seat *seat;
struct wl_shm_pool *pool;
//static int fd; 
//static int size;
//static int stride;
static void on_quit(void *userdata)
{
    int *running_ptr = userdata;
    *running_ptr = 0;
}
bool frame_ready = true;

struct window {
    int width, height;
    int buffer_width, buffer_height;
    bool buffer_dirty;
    bool configured;
    struct wl_buffer *buffer;
    void *data;
    int fd;
    int size;
    int stride;
    bool painted;
};

static void frame_done(void *data,
                       struct wl_callback *cb,
                       uint32_t time)
{
    wl_callback_destroy(cb);
    *(bool *)data = true;
}

static const struct wl_callback_listener frame_listener = {
    .done = frame_done
};


static struct window win = {
    .width = 640,
    .height = 480,
};


static void
toplevel_configure(void *data,
                   struct xdg_toplevel *toplevel,
                   int32_t width,
                   int32_t height,
                   struct wl_array *states)
{
    struct window *win = data;

    if (width > 0 && height > 0) {
        win->width  = width;
        win->height = height;
    }
}


static const struct xdg_toplevel_listener toplevel_listener = {
    .configure = toplevel_configure,
};

static void xdg_wm_base_ping(void *data,
                 struct xdg_wm_base *wm_base,
                 uint32_t serial)
{
    xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void
registry_global(void *data,
                struct wl_registry *registry,
                uint32_t id,
                const char *interface,
                uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0)
        compositor = wl_registry_bind(registry, id,
                                      &wl_compositor_interface, 4);
    else if (strcmp(interface, "wl_shm") == 0)
        shm = wl_registry_bind(registry, id,
                               &wl_shm_interface, 1);
    else if (strcmp(interface, "xdg_wm_base") == 0) {
        wm_base = wl_registry_bind(registry, id,
                                   &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(wm_base, &wm_base_listener, NULL);
    }
    else if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(registry, id,
                            &wl_seat_interface, 1);
    }
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
};

static void buffer_release(void *data, struct wl_buffer *buffer)
{
    // mark buffer reusable or no-op for now
}

static const struct wl_buffer_listener buffer_listener = {
    .release = buffer_release,
};

struct wl_buffer *waylen_create_buffer(struct window *win, int width, int height) {

    win->stride = width * 4;
    win->size   = win->stride * height;

    win->fd = memfd_create("buffer", 0);
    ftruncate(win->fd, win->size);

    win->data = mmap(NULL, win->size,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED, win->fd, 0);

    struct wl_shm_pool *pool =
        wl_shm_create_pool(shm, win->fd, win->size);

    struct wl_buffer *buffer =
        wl_shm_pool_create_buffer(pool, 0,
                                  width, height,
                                  win->stride,
                                  WL_SHM_FORMAT_ARGB8888);

    wl_buffer_add_listener(buffer, &buffer_listener, win);
    wl_shm_pool_destroy(pool);

    return buffer;
}

void wayland_destroy_buffer()  {
    wl_buffer_destroy(win.buffer);
    munmap(win.data, win.size);
    close(win.fd);

    win.buffer = NULL;
    win.data = NULL;
    win.fd = -1;
}

static void
xdg_surface_configure(void *userdata,
                      struct xdg_surface *xdg_surface,
                      uint32_t serial)
{
    struct window *win = userdata;

    xdg_surface_ack_configure(xdg_surface, serial);

    if (win->width <= 0 || win->height <= 0)
        return;

    if (!win->buffer) {
        /* First configure: create buffer immediately */
        win->buffer = waylen_create_buffer(win, win->width, win->height);
        win->buffer_width  = win->width;
        win->buffer_height = win->height;
    } else if (win->buffer_width  != win->width ||
            win->buffer_height != win->height) {
        /* Resize: defer to main loop */
        win->buffer_dirty  = true;
        win->buffer_width  = win->width;
        win->buffer_height = win->height;
    }

    /* Initial commit is required, but must use existing buffer */
    if (win->buffer) {
        wl_surface_attach(surface, win->buffer, 0, 0);
        wl_surface_damage_buffer(surface, 0, 0,
                                 win->width, win->height);
        wl_surface_commit(surface);
        
    }

    win->configured = true;
}


// static void
// xdg_surface_configure(void *userdata,
//                       struct xdg_surface *xdg_surface,
//                       uint32_t serial)
// {
//     struct window *win = userdata;

//     xdg_surface_ack_configure(xdg_surface, serial);

//     if (win->width <= 0 || win->height <= 0)
//         return;

//     if (!win->buffer) {
//         win->buffer_dirty = true;
//         win->buffer_width  = win->width;
//         win->buffer_height = win->height;
//     } else if (win->buffer_width != win->width ||
//                win->buffer_height != win->height) {
//         win->buffer_dirty = true;
//         win->buffer_width  = win->width;
//         win->buffer_height = win->height;
//     }

//     win->configured = true;
// }



static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

int wayland_init() {

    fprintf(stderr, "XDG_RUNTIME_DIR=%s\n", getenv("XDG_RUNTIME_DIR"));
fprintf(stderr, "WAYLAND_DISPLAY=%s\n", getenv("WAYLAND_DISPLAY"));
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland\n");
        return 1;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    if (!compositor || !shm || !wm_base) {
        fprintf(stderr, "Missing required Wayland globals\n");
        exit(1);
    }


    if (seat) {
        keyboard_init(seat, display, on_quit, &running);
    }



    surface =
        wl_compositor_create_surface(compositor);

    struct xdg_surface *xdg_surface =
        xdg_wm_base_get_xdg_surface(wm_base, surface);
    xdg_surface_add_listener(xdg_surface,
                             &xdg_surface_listener,
                             &win);

struct xdg_toplevel *toplevel =
    xdg_surface_get_toplevel(xdg_surface);
        xdg_toplevel_add_listener(toplevel,
                          &toplevel_listener,
                          &win);
    xdg_toplevel_set_title(toplevel, "Wayland Line");

    wl_surface_commit(surface);
    return 0;
}

struct window *wayland_create_window() {
    wayland_init();

    while (!win.configured) {
        if (wl_display_dispatch(display) == -1) {
            break;
        }
    }

    return &win;
}

bool wayland_pump_events(void)
{
    /* Receive + dispatch compositor events */
    if (wl_display_dispatch(display) == -1)
        return false;

    return true;
}

bool wayland_dispatch_complete() {
    if (wl_display_dispatch_pending(display) == -1) {
        return false;
    }

    if (wl_display_flush(display) == -1) {
        return false;
    }

    return true;
}

void wayland_render_frame(struct window *win)
{
    if (!frame_ready)
        return;

    frame_ready = false;

    struct wl_callback *cb = wl_surface_frame(surface);
    wl_callback_add_listener(cb, &frame_listener, &frame_ready);

    wl_surface_attach(surface, win->buffer, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0,
                             win->width, win->height);
//      struct timespec now;
// clock_gettime(CLOCK_MONOTONIC, &now);
//     fprintf(stderr,
//         "COMMIT frame buffer=%p size=%dx%d time=%ld.%09ld\n",
//         (void*)win->buffer,
//         win->width,
//         win->height,
//         now.tv_sec,
//         now.tv_nsec);

    wl_surface_commit(surface);

    wl_display_flush(display);
}

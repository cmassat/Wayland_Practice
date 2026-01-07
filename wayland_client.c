#define _GNU_SOURCE
#include "common.h"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <poll.h>
#include "keyboard.h"

static struct wl_display     *display;
static struct wl_compositor  *compositor;
static struct wl_shm         *shm;
static struct xdg_wm_base    *wm_base;
struct wl_surface *surface;
static void *data;
struct wl_buffer *buffer;
static struct wl_seat *seat;
struct wl_shm_pool *pool;
static int fd; 
static int size;
static int stride;
static bool configured = false;
static void on_quit(void *userdata)
{
    int *running_ptr = userdata;
    *running_ptr = 0;
}

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

void *waylen_create_buffer(int width, int height) {

    stride = width * 4;
    size  = stride * height;

    fd = memfd_create("buffer", 0);
    ftruncate(fd, size);

    data = mmap(NULL, size,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);

    pool =
        wl_shm_create_pool(shm, fd, size);

    buffer =
        wl_shm_pool_create_buffer(pool, 0,
                          width, height,
                          stride,
                          WL_SHM_FORMAT_ARGB8888);
    wl_buffer_add_listener(buffer, &buffer_listener, NULL);
    wl_shm_pool_destroy(pool);
    pool = NULL;
    return data;
}

void wayland_destroy_buffer()  {
    wl_buffer_destroy(buffer);
    munmap(data, size);
    close(fd);

    buffer = NULL;
    data = NULL;
    fd = -1;
}

static void
xdg_surface_configure(void *userdata,
                      struct xdg_surface *surface,
                      uint32_t serial)
{
    struct window *win = userdata;
    xdg_surface_ack_configure(surface, serial);

    if (!configured) {
        configured = true;
        if (buffer)
            wayland_destroy_buffer();

        waylen_create_buffer(win->width, win->height);
        win->data = data;
        win->stride = stride;
    }
}


static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

int wayland_init() {
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland\n");
        return 1;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

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





struct window wayland_create_window() {
    wayland_init();

    while (!configured) {
        if (wl_display_dispatch(display) == -1) {
            break;
        }
    }

    return win;
}

bool wayland_dispatch_complete() {
    int display_fd = wl_display_get_fd(display);
    struct pollfd pfd = {
        .fd = display_fd,
        .events = POLLIN,
    };

    int poll_result = poll(&pfd, 1, 0);
    if (poll_result > 0) {
        if (wl_display_dispatch(display) == -1) {
            return false;
        }
    } else if (poll_result == 0) {
        if (wl_display_dispatch_pending(display) == -1) {
            return false;
        }
    } else {
        return false;
    }

    return wl_display_flush(display) != -1;
}

void wayland_render_frame() {
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, win.width, win.height);
    wl_surface_commit(surface);
}

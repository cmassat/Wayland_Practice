
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
};

struct window * wayland_create_window();
bool wayland_dispatch_complete();
void wayland_render_frame(struct window *win);
void *waylen_create_buffer(struct window *win,int width, int height);
void wayland_destroy_buffer();
bool wayland_pump_events();
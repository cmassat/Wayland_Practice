struct window {
    int width;
    int height;
    int stride;
    void *data;
};

struct window wayland_create_window();
bool wayland_dispatch_complete();
void wayland_render_frame();
void *waylen_create_buffer(int width, int height);
void wayland_destroy_buffer();

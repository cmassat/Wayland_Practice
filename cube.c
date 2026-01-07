#include <cairo/cairo.h>
#include <math.h>

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y; } Vec2;

Vec3 vertices[8] = {
    {-1, -1, -1},
    { 1, -1, -1},
    { 1,  1, -1},
    {-1,  1, -1},
    {-1, -1,  1},
    { 1, -1,  1},
    { 1,  1,  1},
    {-1,  1,  1},
};


int edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
};

Vec3 rotate_x(Vec3 v, float a)
{
    return (Vec3){
        v.x,
        v.y * cosf(a) - v.z * sinf(a),
        v.y * sinf(a) + v.z * cosf(a)
    };
}

Vec3 rotate_y(Vec3 v, float a)
{
    return (Vec3){
        v.x * cosf(a) + v.z * sinf(a),
        v.y,
       -v.x * sinf(a) + v.z * cosf(a)
    };
}

Vec2 project(Vec3 v, float scale, int w, int h)
{
    float z = v.z + 4.0f;  // push cube away from camera

    return (Vec2){
        v.x / z * scale + w / 2,
        v.y / z * scale + h / 2
    };
}

void draw(int win_width, int win_height, cairo_t *cr, float angle) {
    Vec2 projected[8];
    
    for (int i = 0; i < 8; i++) {
        Vec3 r = rotate_y(vertices[i], angle);
        r = rotate_x(r, angle * 0.7f);
        projected[i] = project(r, 200, win_width, win_height);
    }

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 2);

    for (int i = 0; i < 12; i++) {
        Vec2 a = projected[edges[i][0]];
        Vec2 b = projected[edges[i][1]];
        cairo_move_to(cr, a.x, a.y);
        cairo_line_to(cr, b.x, b.y);
    }

    cairo_stroke(cr);

}
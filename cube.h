
#pragma once
#include <cairo/cairo.h>
#include <math.h>

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y; } Vec2;

Vec3 rotate_y(Vec3 v, float a);

Vec2 project(Vec3 v, float scale, int w, int h);

void draw(int win_width, int win_height, cairo_t *cr, float angle );
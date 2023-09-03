#include "la.h"

#include <math.h>

/* 2-DIMENSIONAL INT VECTOR */

Vec2i vec2i(int x, int y)
{
    return (Vec2i) {
        .x = x,
        .y = y,
    };
}

Vec2i vec2is(int n)
{
    return vec2i(n, n);
}

Vec2i vec2i_add(Vec2i a, Vec2i b)
{
    return vec2i(a.x + b.x, a.y + b.y);
}

Vec2i vec2i_sub(Vec2i a, Vec2i b)
{
    return vec2i(a.x - b.x, a.y - b.y);
}

Vec2i vec2i_mul(Vec2i a, Vec2i b)
{
    return vec2i(a.x * b.x, a.y * b.y);
}

Vec2i vec2i_mul3(Vec2i a, Vec2i b, Vec2i c)
{
    return vec2i(a.x * b.x * c.x, a.y * b.y * c.y);
}

Vec2i vec2i_div(Vec2i a, Vec2i b)
{
    return vec2i(a.x / b.x, a.y / b.y);
}

int vec2i_cmp_yx(Vec2i a, Vec2i b)
{
    int diff = a.y - b.y;
    return diff ? diff : a.x - b.x;
}

/* 2-DIMENSIONAL UNSIGNED INT VECTOR */

Vec2ui vec2ui(uint32_t x, uint32_t y)
{
    return (Vec2ui) {
        .x = x,
        .y = y,
    };
}

Vec2ui vec2uis(uint32_t n)
{
    return vec2ui(n, n);
}

Vec2ui vec2ui_add(Vec2ui a, Vec2ui b)
{
    return vec2ui(a.x + b.x, a.y + b.y);
}

Vec2ui vec2ui_sub(Vec2ui a, Vec2ui b)
{
    return vec2ui(a.x - b.x, a.y - b.y);
}

Vec2ui vec2ui_mul(Vec2ui a, Vec2ui b)
{
    return vec2ui(a.x * b.x, a.y * b.y);
}

Vec2ui vec2ui_mul3(Vec2ui a, Vec2ui b, Vec2ui c)
{
    return vec2ui(a.x * b.x * c.x, a.y * b.y * c.y);
}

Vec2ui vec2ui_div(Vec2ui a, Vec2ui b)
{
    return vec2ui(a.x / b.x, a.y / b.y);
}

int vec2ui_cmp_yx(Vec2ui a, Vec2ui b)
{
    int diff = (int) a.y - (int) b.y;
    return diff ? diff : (int) a.x - (int) b.x;
}

/* 2-DIMENSIONAL FLOAT VECTOR */

Vec2f vec2f(float x, float y)
{
    return (Vec2f) {
        .x = x, .y = y
    };
}

Vec2f vec2fs(float n)
{
    return vec2f(n, n);
}

Vec2f vec2f_add(Vec2f a, Vec2f b)
{
    return vec2f(a.x + b.x, a.y + b.y);
}

Vec2f vec2f_sub(Vec2f a, Vec2f b)
{
    return vec2f(a.x - b.x, a.y - b.y);
}

Vec2f vec2f_mul(Vec2f a, Vec2f b)
{
    return vec2f(a.x * b.x, a.y * b.y);
}

Vec2f vec2f_mul3(Vec2f a, Vec2f b, Vec2f c)
{
    return vec2f(a.x * b.x * c.x, a.y * b.y * c.y);
}

Vec2f vec2f_div(Vec2f a, Vec2f b)
{
    return vec2f(a.x / b.x, a.y / b.y);
}

int vec2f_cmp_yx(Vec2f a, Vec2f b)
{
    int diff = a.y - b.y;
    return diff ? diff : a.x - b.x;
}

bool vec2f_eq(Vec2f a, Vec2f b)
{
    return (a.x == b.x && a.y == b.y);
}

bool vec2f_eq_approx(Vec2f a, Vec2f b)
{
    return (round(a.x) == round(b.x) && round(a.y) == round(b.y));
}

/* 4-DIMENSIONAL FLOAT VECTOR */

Vec4f vec4f(float x, float y, float z, float w)
{
    return (Vec4f) {
        .x = x, .y = y, .z = z, .w = w
    };
}

Vec4f vec4fs(float n)
{
    return vec4f(n, n, n, n);
}

Vec4f vec4f_add(Vec4f a, Vec4f b)
{
    return vec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

Vec4f vec4f_sub(Vec4f a, Vec4f b)
{
    return vec4f(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

Vec4f vec4f_mul(Vec4f a, Vec4f b)
{
    return vec4f(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

Vec4f vec4f_div(Vec4f a, Vec4f b)
{
    return vec4f(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

/*  lerpf */

float lerpf(float a, float b, float c)
{
    return a + (b - a) * c;
}
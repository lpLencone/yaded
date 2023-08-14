#include "la.h"

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

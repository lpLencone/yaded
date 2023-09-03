#ifndef YADED_LA_H_
#define YADED_LA_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int x, y;
} Vec2i;

Vec2i vec2i(int x, int y);
Vec2i vec2is(int n);
Vec2i vec2i_add(Vec2i a, Vec2i b);
Vec2i vec2i_sub(Vec2i a, Vec2i b);
Vec2i vec2i_mul(Vec2i a, Vec2i b);
Vec2i vec2i_mul3(Vec2i a, Vec2i b, Vec2i c);
Vec2i vec2i_div(Vec2i a, Vec2i b);
int vec2i_cmp_yx(Vec2i a, Vec2i b);

typedef struct {
    uint32_t x, y;
} Vec2ui;

Vec2ui vec2ui(uint32_t x, uint32_t y);
Vec2ui vec2uis(uint32_t n);
Vec2ui vec2ui_add(Vec2ui a, Vec2ui b);
Vec2ui vec2ui_sub(Vec2ui a, Vec2ui b);
Vec2ui vec2ui_mul(Vec2ui a, Vec2ui b);
Vec2ui vec2ui_mul3(Vec2ui a, Vec2ui b, Vec2ui c);
Vec2ui vec2ui_div(Vec2ui a, Vec2ui b);
int vec2ui_cmp_yx(Vec2ui a, Vec2ui b);

typedef struct {
    float x, y;
} Vec2f;

Vec2f vec2f(float x, float y);
Vec2f vec2fs(float n);
Vec2f vec2f_add(Vec2f a, Vec2f b);
Vec2f vec2f_sub(Vec2f a, Vec2f b);
Vec2f vec2f_mul(Vec2f a, Vec2f b);
Vec2f vec2f_mul3(Vec2f a, Vec2f b, Vec2f c);
Vec2f vec2f_div(Vec2f a, Vec2f b);
int vec2f_cmp_yx(Vec2f a, Vec2f b);
bool vec2f_eq(Vec2f a, Vec2f b);
bool vec2f_eq_approx(Vec2f a, Vec2f b);

typedef struct {
    float x, y, z, w;
} Vec4f;

Vec4f vec4f(float x, float y, float z, float w);
Vec4f vec4fs(float n);
Vec4f vec4f_add(Vec4f a, Vec4f b);
Vec4f vec4f_sub(Vec4f a, Vec4f b);
Vec4f vec4f_mul(Vec4f a, Vec4f b);
Vec4f vec4f_div(Vec4f a, Vec4f b);


float lerpf(float a, float b, float c);


#endif // YADED_LA_H_
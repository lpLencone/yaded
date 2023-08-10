#ifndef YADED_LA_H_
#define YADED_LA_H_

typedef struct {
    float x, y;
} Vec2f;

Vec2f vec2f(float x, float y);
Vec2f vec2fs(float n);
Vec2f vec2f_add(Vec2f a, Vec2f b);
Vec2f vec2f_sub(Vec2f a, Vec2f b);
Vec2f vec2f_mul(Vec2f a, Vec2f b);
Vec2f vec2f_div(Vec2f a, Vec2f b);

#endif // YADED_LA_H_
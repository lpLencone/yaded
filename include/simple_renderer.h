#ifndef YADED_SIMPLE_RENDERER_H_
#define YADED_SIMPLE_RENDERER_H_

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%20s : %4d : %-20s\n", __FILE__, __LINE__, __func__);
#endif

#include "la.h"
#include "gl_extra.h"

#define BUFFER_CAPACITY (1024 * 1024)

typedef enum {
    UNIFORM_TIME,
    UNIFORM_RESOLUTION,
    UNIFORM_CAMERA,
    UNIFORM_SCALE,
    UNIFORM_COUNT,
} Uniform_Enum;

typedef enum {
    SHADER_COLOR,
    SHADER_IMAGE,
    SHADER_TEXT,
    SHADER_PRIDE,
    SHADER_COUNT,
} Shader_Enum;

typedef struct {
    Vec2f uv;
    Vec2f pos;
    Vec4f color;
} Simple_Vertex;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint programs[SHADER_COUNT];
    Shader_Enum current_shader;

    GLint time;
    GLint resolution;
    GLint camera;
    GLint scale;
    
    Simple_Vertex buffer[BUFFER_CAPACITY];
    size_t buffer_count;
} Simple_Renderer;

void sr_init(Simple_Renderer *sr);

void sr_set_shader(Simple_Renderer *sr, Shader_Enum shader);

void sr_vertex(
    Simple_Renderer *sr,
    Vec2f p, Vec4f c, Vec2f uv);

void sr_triangle(
    Simple_Renderer *sr,
    Vec2f p0, Vec2f p1, Vec2f p2,
    Vec4f c0, Vec4f c1, Vec4f c2,
    Vec2f uv0, Vec2f uv1, Vec2f uv2);

void sr_quad(
    Simple_Renderer *sr,
    Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
    Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
    Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3);

void sr_solid_rect(
    Simple_Renderer *sr, 
    Vec2f p, Vec2f s, Vec4f c);

void sr_image_rect(
    Simple_Renderer *sr, 
    Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c);

void sr_flush(Simple_Renderer *sr);

#endif // YADED_SIMPLE_RENDERER_H_
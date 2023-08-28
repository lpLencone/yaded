#include "simple_renderer.h"

#include <assert.h>
#include <stdlib.h>

static void setup_vertices_and_buffers(Simple_Renderer *sr);
static void get_uniforms_loc(Simple_Renderer *sr);

void sr_init(
    Simple_Renderer *sr, 
    const char *vert_filename, 
    const char *color_frag_filename, 
    const char *image_frag_filename,
    const char *pride_frag_filename) 
{
    setup_vertices_and_buffers(sr);

    GLuint shaders[2];
    compile_shader(vert_filename, GL_VERTEX_SHADER, &shaders[0]);
    compile_shader(color_frag_filename, GL_FRAGMENT_SHADER, &shaders[1]);
    sr->current_shader = SHADER_COLOR;
    sr->programs[sr->current_shader] = glCreateProgram();
    attach_shaders(sr->programs[sr->current_shader], shaders, 2);
    if (!link_program(sr->programs[sr->current_shader])) {
        exit(1);
    }

    compile_shader(image_frag_filename, GL_FRAGMENT_SHADER, &shaders[1]);
    sr->current_shader = SHADER_IMAGE;
    sr->programs[sr->current_shader] = glCreateProgram();
    attach_shaders(sr->programs[sr->current_shader], shaders, 2);
    if (!link_program(sr->programs[sr->current_shader])) {
        exit(1);
    }

    compile_shader(pride_frag_filename, GL_FRAGMENT_SHADER, &shaders[1]);
    sr->current_shader = SHADER_PRIDE;
    sr->programs[sr->current_shader] = glCreateProgram();
    attach_shaders(sr->programs[sr->current_shader], shaders, 2);
    if (!link_program(sr->programs[sr->current_shader])) {
        exit(1);
    }
}

void sr_set_shader(Simple_Renderer *sr, Shader_Enum shader)
{
    sr->current_shader = shader;

    glBindVertexArray(sr->vao);
    glBindBuffer(GL_ARRAY_BUFFER, sr->vbo);
    glUseProgram(sr->programs[sr->current_shader]);
    
    get_uniforms_loc(sr);
}

void sr_vertex(
    Simple_Renderer *sr,
    Vec2f p, Vec4f c, Vec2f uv)
{
    assert(sr->buffer_count < BUFFER_CAPACITY);
    
    Simple_Vertex *vp = &sr->buffer[sr->buffer_count];
    vp->pos = p;
    vp->color = c;
    vp->uv = uv;

    sr->buffer_count++;
}

void sr_triangle(
    Simple_Renderer *sr,
    Vec2f p0, Vec2f p1, Vec2f p2,
    Vec4f c0, Vec4f c1, Vec4f c2,
    Vec2f uv0, Vec2f uv1, Vec2f uv2)
{
    sr_vertex(sr, p0, c0, uv0);
    sr_vertex(sr, p1, c1, uv1);
    sr_vertex(sr, p2, c2, uv2);
}

void sr_quad(
    Simple_Renderer *sr,
    Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
    Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
    Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3)
{
    sr_triangle(sr, p0, p1, p2, c0, c1, c2, uv0, uv1, uv2);
    sr_triangle(sr, p1, p2, p3, c1, c2, c3, uv1, uv2, uv3);
}

void sr_solid_rect(Simple_Renderer *sr, Vec2f p, Vec2f s, Vec4f c)
{
    // NOTE: p is the left-bottom coordinate of the program
    sr_quad(
        sr, 
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        c, c, c, c,
        vec2fs(0), vec2fs(0), vec2fs(0), vec2fs(0));
}

void sr_image_rect(Simple_Renderer *sr, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs)
{
    Vec4f c = vec4fs(0);
    sr_quad(
        sr, 
        p, vec2f_add(p, vec2f(s.x, 0)), vec2f_add(p, vec2f(0, s.y)), vec2f_add(p, s),
        c, c, c, c,
        uvp, vec2f_add(uvp, vec2f(uvs.x, 0)), vec2f_add(uvp, vec2f(0, uvs.y)), vec2f_add(uvp, uvs));
}

void sr_clear(Simple_Renderer *sr)
{
    sr->buffer_count = 0;
}

void sr_sync(const Simple_Renderer *sr)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, sr->buffer_count * sizeof(Simple_Vertex), 
                    sr->buffer);
}

void sr_draw(const Simple_Renderer *sr)
{
    glDrawArrays(GL_TRIANGLES, 0, sr->buffer_count);
}

// STATIC

typedef enum {
    SVA_UV,
    SVA_POS,
    SVA_COLOR,
} Simple_Vertex_Attrib;

static void setup_vertices_and_buffers(Simple_Renderer *sr)
{
    glGenVertexArrays(1, &sr->vao);
    glBindVertexArray(sr->vao);

    glGenBuffers(1, &sr->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sr->vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(sr->buffer), sr->buffer, GL_DYNAMIC_DRAW);

    // UV
    glEnableVertexAttribArray(SVA_UV);
    glVertexAttribPointer(
        SVA_UV, 2, GL_FLOAT, GL_FALSE,
        /*  stride: how many bytes to stride through to get the next
            data of that field */
        sizeof(Simple_Vertex), 
        /*  pointer: a pointer to the offset in a vertex where the 
            attribute is located at. */
        (GLvoid *) offsetof(Simple_Vertex, uv) 
    );

    // Position
    glEnableVertexAttribArray(SVA_POS);
    glVertexAttribPointer(
        SVA_POS, 2, GL_FLOAT, GL_FALSE, sizeof(Simple_Vertex),
        (GLvoid *) offsetof(Simple_Vertex, pos)
    );

    // Color
    glEnableVertexAttribArray(SVA_COLOR);
    glVertexAttribPointer(
        SVA_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(Simple_Vertex),
        (GLvoid *) offsetof(Simple_Vertex, color)
    );
}

static_assert(UNIFORM_COUNT == 4, "The amount of uniforms has changed");

static void get_uniforms_loc(Simple_Renderer *sr)
{
    sr->time = glGetUniformLocation(sr->programs[sr->current_shader], "time");
    sr->resolution = glGetUniformLocation(sr->programs[sr->current_shader], "resolution");
    sr->camera = glGetUniformLocation(sr->programs[sr->current_shader], "camera");
    sr->scale = glGetUniformLocation(sr->programs[sr->current_shader], "scale");
}
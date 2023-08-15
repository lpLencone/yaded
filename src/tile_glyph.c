#include "glyph.h"
#include "tile_glyph.h"
#include "stb_image.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#define TILE_GLYPH_BUFFER_CAPACITY (1024 * 1024)

static const Attr_Def glyph_attr_defs[COUNT_TILE_GLYPH_ATTRS] = {
    [TILE_GLYPH_ATTR_TILE]  = {
        .offset = offsetof(Tile_Glyph, tile),
        .comps = 2,
        .type = GL_INT,
    },
    [TILE_GLYPH_ATTR_CH]    = {
        .offset = offsetof(Tile_Glyph, ch),
        .comps = 1,
        .type = GL_INT,
    },
    [TILE_GLYPH_ATTR_FG_COLOR] = {
        .offset = offsetof(Tile_Glyph, fg_color),
        .comps = 4,
        .type = GL_FLOAT,
    },
    [TILE_GLYPH_ATTR_BG_COLOR] = {
        .offset = offsetof(Tile_Glyph, bg_color),
        .comps = 4,
        .type = GL_FLOAT,
    },
};

static Tile_Glyph tile_glyph_buffer[TILE_GLYPH_BUFFER_CAPACITY];
static size_t tile_glyph_buffer_count = 0;

static_assert(COUNT_TILE_GLYPH_ATTRS == 4, "The amount of glyph vertex attributes has changed");

static void     load_texture_atlas(const char *filename);
static GLuint   init_shaders(const char *vert_filename, const char *frag_filename);

void tile_glyph_buffer_init(Glyph_Uniform *tgu, const char *atlas_filename, 
                            const char *vert_filename, const char *frag_filename)
{
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tile_glyph_buffer), tile_glyph_buffer, GL_DYNAMIC_DRAW);

    for (Tile_Glyph_Attr attr = 0; attr < COUNT_TILE_GLYPH_ATTRS; attr++) {
        glEnableVertexAttribArray(attr);
        switch (glyph_attr_defs[attr].type) {
            case GL_FLOAT: {
                glVertexAttribPointer(
                    attr,
                    glyph_attr_defs[attr].comps, 
                    glyph_attr_defs[attr].type, 
                    GL_FALSE, sizeof(Tile_Glyph), 
                    (void *) glyph_attr_defs[attr].offset
                );
            } break;

            case GL_INT: {
                glVertexAttribIPointer(
                    attr,
                    glyph_attr_defs[attr].comps,
                    glyph_attr_defs[attr].type,
                    sizeof(Tile_Glyph),
                    (void *) glyph_attr_defs[attr].offset
                );
            } break;
            
            default:
                assert(false && "Unreachable");
                exit(1); // why would anyone disable assert though?
        }
        glVertexAttribDivisor(attr, 1);
    }

    load_texture_atlas(atlas_filename);
    GLuint program = init_shaders(vert_filename, frag_filename);

    glUseProgram(program);

    tgu->time = glGetUniformLocation(program, "time");
    tgu->resolution = glGetUniformLocation(program, "resolution");
    tgu->scale = glGetUniformLocation(program, "scale");
    tgu->camera = glGetUniformLocation(program, "camera");
}

void tile_glyph_buffer_clear(void)
{
    tile_glyph_buffer_count = 0;
}

void tile_glyph_buffer_push(Tile_Glyph glyph)
{
    assert(tile_glyph_buffer_count < TILE_GLYPH_BUFFER_CAPACITY);
    tile_glyph_buffer[tile_glyph_buffer_count++] = glyph;
}

void tile_glyph_buffer_sync(void)
{
    glBufferSubData(GL_ARRAY_BUFFER, 
                    0, 
                    tile_glyph_buffer_count * sizeof(Tile_Glyph), 
                    tile_glyph_buffer);
}


void tile_glyph_render_text(const char *s, Vec2i tile, Vec4f fg_color, Vec4f bg_color)
{
    size_t slen = strlen(s);
    for (size_t i = 0; i < slen; i++) {
        Tile_Glyph glyph = {
            .tile       = vec2i_add(tile, vec2i(i, 0)),
            .ch         = s[i],
            .fg_color   = fg_color,
            .bg_color   = bg_color,
        };
        tile_glyph_buffer_push(glyph);
    }
}

void tile_glyph_draw(void)
{
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, tile_glyph_buffer_count);
}

/* static */

static void load_texture_atlas(const char *filename)
{
    int w, h, n;
    unsigned char *pixels = stbi_load(filename, &w, &h, &n, STBI_rgb_alpha);
    if (pixels == NULL) {
        fprintf(stderr, "ERROR: could not load file %s: %s\n", filename, stbi_failure_reason());
        exit(1);
    }

    GLuint font_texture = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}


static GLuint init_shaders(const char *vert_filename, const char *frag_filename)
{
    GLuint vert_shader = 0;
    if (!compile_shader_file(vert_filename, GL_VERTEX_SHADER, &vert_shader)) {
        exit(1);
    };

    GLuint frag_shader = 0;
    if (!compile_shader_file(frag_filename, GL_FRAGMENT_SHADER, &frag_shader)) {
        exit(1);
    };

    GLuint program = 0;
    if (!link_program(vert_shader, frag_shader, &program)) {
        exit(1);
    }

    return program;
}







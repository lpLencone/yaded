#include "tile_glyph.h"
#include "stb_image.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define TILE_GLYPH_BUFFER_CAPACITY (512 * 1024)

typedef enum {
    TILE_GLYPH_ATTR_TILE = 0,
    TILE_GLYPH_ATTR_CH,
    TILE_GLYPH_ATTR_FG_COLOR,
    TILE_GLYPH_ATTR_BG_COLOR,
    COUNT_TILE_GLYPH_ATTRS,
} Tile_Glyph_Attr;

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

static_assert(COUNT_TILE_GLYPH_ATTRS == 4, "The amount of glyph vertex attributes has changed");

static void load_texture_atlas(const char *filename);
static void init_shaders(Tile_Glyph_Renderer *tgr, const char *vert_filename, 
                         const char *frag_filename);

static void tile_glyph_buffer_push(Tile_Glyph_Renderer * tgr, Tile_Glyph glyph);

Tile_Glyph_Renderer tile_glyph_renderer_init(
    const char *atlas_filename, const char *vert_filename, 
    const char *frag_filename)
{
    Tile_Glyph_Renderer tgr = {
        .time = 0, .resolution = 0, .scale = 0, .camera = 0,
        .buffer_count = 0, .buffer_capacity = TILE_GLYPH_BUFFER_CAPACITY
    };

    tgr.buffer = calloc(TILE_GLYPH_BUFFER_CAPACITY, sizeof(Tile_Glyph));

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Tile_Glyph) * tgr.buffer_capacity, tgr.buffer, GL_DYNAMIC_DRAW);

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
    init_shaders(&tgr, vert_filename, frag_filename);

    return tgr;
}

void tile_glyph_buffer_clear(Tile_Glyph_Renderer *tgr)
{
    tgr->buffer_count = 0;
}

void tile_glyph_buffer_sync(Tile_Glyph_Renderer *tgr)
{
    glBufferSubData(GL_ARRAY_BUFFER, 
                    0, 
                    tgr->buffer_count * sizeof(Tile_Glyph), 
                    tgr->buffer);
}

void tile_glyph_render_text(Tile_Glyph_Renderer *tgr, const char *s, Vec2i tile, Vec4f fg_color, Vec4f bg_color)
{
    size_t slen = strlen(s);
    for (size_t i = 0; i < slen; i++) {
        Tile_Glyph glyph = {
            .tile       = vec2i_add(tile, vec2i(i, 0)),
            .ch         = s[i],
            .fg_color   = fg_color,
            .bg_color   = bg_color,
        };
        tile_glyph_buffer_push(tgr, glyph);
    }
}

void tile_glyph_draw(Tile_Glyph_Renderer *tgr)
{
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, tgr->buffer_count);
}

/* static */
static void tile_glyph_buffer_push(Tile_Glyph_Renderer *tgr, Tile_Glyph glyph)
{
    assert(tgr->buffer_count < tgr->buffer_capacity);
    tgr->buffer[tgr->buffer_count++] = glyph;
}

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


static void init_shaders(Tile_Glyph_Renderer *tgr, const char *vert_filename, const char *frag_filename)
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

    glUseProgram(program);

    tgr->time = glGetUniformLocation(program, "time");
    tgr->resolution = glGetUniformLocation(program, "resolution");
    tgr->scale = glGetUniformLocation(program, "scale");
    tgr->camera = glGetUniformLocation(program, "camera");
}







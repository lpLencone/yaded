#include "tile_glyph.h"
#include "stb_image.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    TILE_GLYPH_ATTR_TILE = 0,
    TILE_GLYPH_ATTR_CH,
    TILE_GLYPH_ATTR_FG_COLOR,
    TILE_GLYPH_ATTR_BG_COLOR,
    COUNT_TILE_GLYPH_ATTRS,
} Tile_Glyph_Attr;

typedef struct {
    size_t offset;
    GLint  comps;
    GLenum type;
} Attr_Def;

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

static void load_texture_atlas(Tile_Glyph_Renderer *tgr, const char *filename);

static void glyph_push(Tile_Glyph_Renderer *tgr, Tile_Glyph glyph);

void tgr_init(Tile_Glyph_Renderer *tgr, const char *atlas_filename, 
              const char *vert_filename, const char *frag_filename)
{
    glGenVertexArrays(1, &tgr->vao);
    glBindVertexArray(tgr->vao);

    glGenBuffers(1, &tgr->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tgr->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tgr->glyph), tgr->glyph, GL_DYNAMIC_DRAW);

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

    load_texture_atlas(tgr, atlas_filename);


    const char *shaders_filenames[] = {
        vert_filename, frag_filename, "shaders/common.vert", "shaders/common.frag",
    };
    size_t n_shaders = sizeof(shaders_filenames) / sizeof(shaders_filenames[0]);

    GLuint *shaders = malloc(n_shaders * sizeof(GLuint));

    GLuint program = glCreateProgram();
    compile_shaders(shaders_filenames, n_shaders, shaders);
    attach_shaders(program, shaders, n_shaders);
    if (!link_program(program)) {
        fprintf(stderr, "At %s : %d\n", __FILE__, __LINE__);
    }

    free(shaders);

    glUseProgram(program);
    tgr->time = glGetUniformLocation(program, "time");
    tgr->resolution = glGetUniformLocation(program, "resolution");
    tgr->scale = glGetUniformLocation(program, "scale");
    tgr->camera = glGetUniformLocation(program, "camera");
}

void tgr_clear(Tile_Glyph_Renderer *tgr)
{
    tgr->glyph_count = 0;
}

void tgr_sync(Tile_Glyph_Renderer *tgr)
{
    glBufferSubData(
        GL_ARRAY_BUFFER, 
        0, 
        tgr->glyph_count * sizeof(Tile_Glyph), 
        tgr->glyph
    );
}

void tgr_render_text(Tile_Glyph_Renderer *tgr, const char *s, Vec2i tile, 
                  Vec4f fg_color, Vec4f bg_color)
{
    size_t slen = strlen(s);
    for (size_t i = 0; i < slen; i++) {
        Tile_Glyph glyph = {
            .tile     = vec2i_add(tile, vec2i(i, 0)),
            .ch       = s[i],
            .fg_color = fg_color,
            .bg_color = bg_color,
        };
        glyph_push(tgr, glyph);
    }
}

void tgr_draw(Tile_Glyph_Renderer *tgr)
{
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, tgr->glyph_count);
}

static void glyph_push(Tile_Glyph_Renderer *tgr, Tile_Glyph glyph)
{
    assert(tgr->glyph_count < sizeof(tgr->glyph) / sizeof(Tile_Glyph));
    tgr->glyph[tgr->glyph_count++] = glyph;
}

static void load_texture_atlas(Tile_Glyph_Renderer *tgr, const char *filename)
{
    int w, h, n;
    unsigned char *pixels = stbi_load(filename, &w, &h, &n, STBI_rgb_alpha);
    if (pixels == NULL) {
        fprintf(stderr, "ERROR: could not load file %s: %s\n", filename, stbi_failure_reason());
        exit(1);
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &tgr->font_texture);
    glBindTexture(GL_TEXTURE_2D, tgr->font_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    stbi_image_free(pixels);
}





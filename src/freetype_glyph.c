#include "freetype_glyph.h"

#include <assert.h>
#include <stdlib.h>

typedef enum {
    FT_GLYPH_ATTR_POS = 0,
    FT_GLYPH_ATTR_SIZE,
    FT_GLYPH_ATTR_CH,
    FT_GLYPH_ATTR_FG_COLOR,
    FT_GLYPH_ATTR_BG_COLOR,
    COUNT_FT_GLYPH_ATTRS,
} FreeType_Glyph_Attr;

typedef struct {
    size_t offset;
    GLint  comps;
    GLenum type;
} Attr_Def;

static const Attr_Def glyph_attr_defs[COUNT_FT_GLYPH_ATTRS] = {
    [FT_GLYPH_ATTR_POS]  = {
        .offset = offsetof(FreeType_Glyph, pos),
        .comps = 2,
        .type = GL_FLOAT,
    },
    [FT_GLYPH_ATTR_SIZE]  = {
        .offset = offsetof(FreeType_Glyph, size),
        .comps = 2,
        .type = GL_FLOAT,
    },
    [FT_GLYPH_ATTR_CH]    = {
        .offset = offsetof(FreeType_Glyph, ch),
        .comps = 1,
        .type = GL_INT,
    },
    [FT_GLYPH_ATTR_FG_COLOR] = {
        .offset = offsetof(FreeType_Glyph, fg_color),
        .comps = 4,
        .type = GL_FLOAT,
    },
    [FT_GLYPH_ATTR_BG_COLOR] = {
        .offset = offsetof(FreeType_Glyph, bg_color),
        .comps = 4,
        .type = GL_FLOAT,
    },
};

static_assert(COUNT_FT_GLYPH_ATTRS == 5, "The amount of glyph vertex attributes has changed");

static void init_shaders(FreeType_Glyph_Renderer *ftgr, const char *vert_filename, 
                         const char *frag_filename);

void ftgr_init(FreeType_Glyph_Renderer *ftgr,
               const char *vert_filename, const char *frag_filename)
{
    glGenVertexArrays(1, &ftgr->vao);
    glBindVertexArray(ftgr->vao);

    glGenBuffers(1, &ftgr->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ftgr->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ftgr->glyph), ftgr->glyph, GL_DYNAMIC_DRAW);

    for (FreeType_Glyph_Attr attr = 0; attr < COUNT_FT_GLYPH_ATTRS; attr++) {
        glEnableVertexAttribArray(attr);
        switch (glyph_attr_defs[attr].type) {
            case GL_FLOAT: {
                glVertexAttribPointer(
                    attr,
                    glyph_attr_defs[attr].comps, 
                    glyph_attr_defs[attr].type, 
                    GL_FALSE, sizeof(FreeType_Glyph), 
                    (void *) glyph_attr_defs[attr].offset
                );
            } break;

            case GL_INT: {
                glVertexAttribIPointer(
                    attr,
                    glyph_attr_defs[attr].comps,
                    glyph_attr_defs[attr].type,
                    sizeof(FreeType_Glyph),
                    (void *) glyph_attr_defs[attr].offset
                );
            } break;
            
            default:
                assert(false && "Unreachable");
                exit(1); // why would anyone disable assert though?
        }
        glVertexAttribDivisor(attr, 1);

        init_shaders(ftgr, vert_filename, frag_filename);
    }
}

void ftgr_clear(FreeType_Glyph_Renderer *ftgr)
{
    ftgr->glyph_count = 0;
}

void ftgr_sync(FreeType_Glyph_Renderer *ftgr)
{
    (void) ftgr;
}

void ftgr_render_text(FreeType_Glyph_Renderer *ftgr, const char *s, Vec2i tile, 
                   Vec4f fg_color, Vec4f bg_color)
{
    (void) ftgr;
    (void) s, (void) tile, (void) fg_color, (void) bg_color;
}

void ftgr_draw(FreeType_Glyph_Renderer *ftgr)
{
    (void) ftgr;
}




static void init_shaders(FreeType_Glyph_Renderer *ftgr, const char *vert_filename, 
                         const char *frag_filename)
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

    ftgr->time = glGetUniformLocation(program, "time");
    ftgr->resolution = glGetUniformLocation(program, "resolution");
    // ftgr->scale = glGetUniformLocation(program, "scale");
    ftgr->camera = glGetUniformLocation(program, "camera");
}



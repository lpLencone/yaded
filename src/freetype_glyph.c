#include "freetype_glyph.h"

#include <assert.h>
#include <stdlib.h>

typedef enum {
    FT_GLYPH_ATTR_POS = 0,
    FT_GLYPH_ATTR_SIZE,
    FT_GLYPH_ATTR_UV_POS,
    FT_GLYPH_ATTR_UV_SIZE,
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
    [FT_GLYPH_ATTR_POS] = {
        .offset = offsetof(FreeType_Glyph, pos),
        .comps = 2,
        .type = GL_FLOAT,
    },
    [FT_GLYPH_ATTR_SIZE] = {
        .offset = offsetof(FreeType_Glyph, size),
        .comps = 2,
        .type = GL_FLOAT,
    },
    [FT_GLYPH_ATTR_UV_POS] = {
        .offset = offsetof(FreeType_Glyph, uv_pos),
        .comps = 2,
        .type = GL_FLOAT,
    },
    [FT_GLYPH_ATTR_UV_SIZE] = {
        .offset = offsetof(FreeType_Glyph, uv_size),
        .comps = 2,
        .type = GL_FLOAT,
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

static_assert(COUNT_FT_GLYPH_ATTRS == 6, "The amount of glyph vertex attributes has changed");

static void init_glyph_texture_atlas(FreeType_Glyph_Renderer *ftgr, FT_Face face);
static void glyph_push(FreeType_Glyph_Renderer *ftgr, FreeType_Glyph glyph);

void ftgr_init(FreeType_Glyph_Renderer *ftgr, FT_Face face,
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

        GLshader gl_shaders[2] = {
            [0].filename = vert_filename,
            [0].shader_type = GL_VERTEX_SHADER,
            [1].filename = frag_filename,
            [1].shader_type = GL_FRAGMENT_SHADER,
        };
        compile_shaders(&ftgr->program, gl_shaders, 2);
    }

    ftgr->atlas_w = 0;
    ftgr->atlas_h = 0;
    init_glyph_texture_atlas(ftgr, face);

    glUseProgram(ftgr->program);

    ftgr->time = glGetUniformLocation(ftgr->program, "time");
    ftgr->resolution = glGetUniformLocation(ftgr->program, "resolution");
    ftgr->camera = glGetUniformLocation(ftgr->program, "camera");
}

void ftgr_clear(FreeType_Glyph_Renderer *ftgr)
{
    ftgr->glyph_count = 0;
}

void ftgr_sync(FreeType_Glyph_Renderer *ftgr)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, ftgr->glyph_count * sizeof(FreeType_Glyph), 
                    ftgr->glyph);
}

void ftgr_draw(FreeType_Glyph_Renderer *ftgr)
{
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ftgr->glyph_count);
}

void ftgr_render_string_n(FreeType_Glyph_Renderer *ftgr, const char *s, size_t n,
                          Vec2f pos, Vec4f fg_color, Vec4f bg_color)
{
    for (size_t i = 0; i < n; i++) {
        Glyph_Info gi = ftgr->gi[(int) s[i]];
        float x2 = pos.x + gi.bl;
        float y2 = -pos.y - gi.bt;
        float w  = gi.bw;
        float h  = gi.bh;

        pos.x += gi.ax;
        pos.y += gi.ay;

        FreeType_Glyph glyph = {
            .pos        = vec2f(x2, -y2),
            .size       = vec2f(w, -h),
            .uv_pos     = vec2f(gi.tx, 0.0f),
            .uv_size    = vec2f(gi.bw / (float) ftgr->atlas_w, gi.bh / (float) ftgr->atlas_h),
            .bg_color   = bg_color,
            .fg_color   = fg_color,
        };

        glyph_push(ftgr, glyph);
    }
}

void ftgr_render_string(FreeType_Glyph_Renderer *ftgr, const char *s, Vec2f pos, 
                        Vec4f fg_color, Vec4f bg_color)
{
    ftgr_render_string_n(ftgr, s, strlen(s), pos, fg_color, bg_color);
}

float ftgr_get_s_width_n(FreeType_Glyph_Renderer *ftgr, const char *s, size_t n)
{
    float width = 0;
    for (size_t i = 0; i < n; i++) {
        Glyph_Info gi = ftgr->gi[(int) s[i]];
        width += gi.ax;
    }
    return width;
}

float ftgr_get_s_width_n_pad(FreeType_Glyph_Renderer *ftgr, const char *s, size_t n, char pad)
{
    size_t i = 0;
    size_t slen = 0;
    float width = ftgr_get_s_width_n(ftgr, s, slen);

    printf("%ld\n", n);

    float ax = ftgr->gi[(int) pad].ax;
    while (i++ < n) {
        width += ax;
    }

    return width;
}

size_t ftgr_get_glyph_index_near(FreeType_Glyph_Renderer *ftgr, const char *s, float width)
{
    size_t i;
    float last_width = 0;

    for (i = 0; i < strlen(s); i++) {
        float this_width = ftgr_get_s_width_n(ftgr, s, i);
        if (this_width >= width) {
            return (i > 0) 
                ? (this_width - width < width - last_width) ? i : i - 1
                : 0;
        }
        last_width = this_width;
    }
    return i;
}

size_t ftgr_get_glyph_index_near_pad(FreeType_Glyph_Renderer *ftgr, const char *s,
                                     float width, char pad)
{
    size_t i;

    size_t slen = strlen(s);
    float current_width = 0;

    float ax = ftgr->gi[(int) pad].ax;

    for (i = 0; current_width < width; i++) {
        if (i <= slen) {
            current_width = ftgr_get_s_width_n(ftgr, s, i);
        } else {
            current_width += ax;
        }
    }
    return (i > 0) ? i - 1 : i;
}

void ftgr_use(const FreeType_Glyph_Renderer *ftgr)
{
    glBindVertexArray(ftgr->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ftgr->vbo);
    glUseProgram(ftgr->program);
}

static void glyph_push(FreeType_Glyph_Renderer *ftgr, FreeType_Glyph glyph)
{
    assert(ftgr->glyph_count < sizeof(ftgr->glyph) / sizeof(FreeType_Glyph));
    ftgr->glyph[ftgr->glyph_count++] = glyph;
}

static void init_glyph_texture_atlas(FreeType_Glyph_Renderer *ftgr, FT_Face face)
{
    for (size_t i = 32; i < 128; i++) {
        FT_Error error;
        if ((error = FT_Load_Char(face, i, FT_LOAD_RENDER))) {
            fprintf(stderr, "ERROR: %s\n", FT_Error_String(error));
            exit(1);
        }
        if ((error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))) {
            fprintf(stderr, "ERROR: %s\n", FT_Error_String(error));
            exit(1);
        }

        ftgr->atlas_w += face->glyph->bitmap.width;
        if (ftgr->atlas_h < face->glyph->bitmap.rows) {
            ftgr->atlas_h = face->glyph->bitmap.rows;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &ftgr->glyph_texture);
    glBindTexture(GL_TEXTURE_2D, ftgr->glyph_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ftgr->atlas_w, ftgr->atlas_h, 0, 
                 GL_RED, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int x = 0;
    FT_Error error;
    for (size_t i = 32; i < 128; i++) {
        if ((error = FT_Load_Char(face, i, FT_LOAD_RENDER))) {
            fprintf(stderr, "ERROR: %s\n", FT_Error_String(error));
            exit(1);
        }

        ftgr->gi[i].ax = face->glyph->advance.x >> 6;
        ftgr->gi[i].ay = face->glyph->advance.y >> 6;
        ftgr->gi[i].bw = face->glyph->bitmap.width;
        ftgr->gi[i].bh = face->glyph->bitmap.rows;
        ftgr->gi[i].bl = face->glyph->bitmap_left;
        ftgr->gi[i].bt = face->glyph->bitmap_top;
        ftgr->gi[i].tx = (float) x / (float) ftgr->atlas_w;

        glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, face->glyph->bitmap.width, 
                        face->glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, 
                        face->glyph->bitmap.buffer);

        x += face->glyph->bitmap.width;
    }
}




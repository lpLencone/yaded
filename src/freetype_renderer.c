#include "freetype_renderer.h"

#include <assert.h>
#include <stdlib.h>

typedef enum {
    FT_GLYPH_ATTR_POS,
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

static_assert(COUNT_FT_GLYPH_ATTRS == 6, "The amount of glyph vertex attributes has changed");

static void init_glyph_texture_atlas(FreeType_Renderer *ftr, FT_Face face);

void ftr_init(FreeType_Renderer *ftr, FT_Face face)
{
    ftr->atlas_w = 0;
    ftr->atlas_h = 0;
    init_glyph_texture_atlas(ftr, face);
}

Vec2f ftr_render_s_n(
    FreeType_Renderer *ftr, Simple_Renderer *sr,   
    const char *s, size_t n, Vec2f pos, Vec4f c)
{
    for (size_t i = 0; i < n; i++) {
        Glyph_Info gi = ftr->gi[(int) s[i]];
        float x2 = pos.x + gi.bl;
        float y2 = -pos.y - gi.bt;
        float w  = gi.bw;
        float h  = gi.bh;

        pos.x += gi.ax;
        pos.y += gi.ay;

        sr_image_rect(
            sr, 
            vec2f(x2, -y2), 
            vec2f(w, -h), 
            vec2f(gi.tx, 0.0f),
            vec2f(gi.bw / (float) ftr->atlas_w, gi.bh / (float) ftr->atlas_h),
            c
        );
    }
    return pos;
}

float ftr_get_s_width_n(FreeType_Renderer *ftr, const char *s, size_t n)
{
    float width = 0;
    for (size_t i = 0; i < n; i++) {
        width += ftr->gi[(int) s[i]].ax;
    }
    return width;
}

float ftr_get_s_width_n_pad(FreeType_Renderer *ftr, const char *s, size_t n, char pad)
{
    size_t i = 0;
    size_t slen = 0;
    float width = ftr_get_s_width_n(ftr, s, slen);

    float ax = ftr->gi[(int) pad].ax;
    while (i++ < n) {
        width += ax;
    }

    return width;
}

size_t ftr_get_glyph_index_near(FreeType_Renderer *ftr, const char *s, float width)
{
    size_t i;
    float last_width = 0;

    for (i = 0; i < strlen(s); i++) {
        float this_width = ftr_get_s_width_n(ftr, s, i);
        if (this_width >= width) {
            return (i > 0) 
                ? (this_width - width < width - last_width) ? i : i - 1
                : 0;
        }
        last_width = this_width;
    }
    return i;
}

size_t ftr_get_glyph_index_near_pad(FreeType_Renderer *ftr, const char *s,
                                     float width, char pad)
{
    size_t i;

    size_t slen = strlen(s);
    float current_width = 0;

    float ax = ftr->gi[(int) pad].ax;

    for (i = 0; current_width < width; i++) {
        if (i <= slen) {
            current_width = ftr_get_s_width_n(ftr, s, i);
        } else {
            current_width += ax;
        }
    }
    return (i > 0) ? i - 1 : i;
}

static void init_glyph_texture_atlas(FreeType_Renderer *ftr, FT_Face face)
{
    // The last flag makes the loading quite slow
    FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_(FT_RENDER_MODE_SDF);
    
    for (size_t i = 32; i < 128; i++) {
        FT_Error error;
        if ((error = FT_Load_Char(face, i, load_flags))) {
            fprintf(stderr, "ERROR: %s\n", FT_Error_String(error));
            exit(1);
        }
        if ((error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))) {
            fprintf(stderr, "ERROR: %s\n", FT_Error_String(error));
            exit(1);
        }

        ftr->atlas_w += face->glyph->bitmap.width;
        if (ftr->atlas_h < face->glyph->bitmap.rows) {
            ftr->atlas_h = face->glyph->bitmap.rows;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &ftr->glyph_texture);
    glBindTexture(GL_TEXTURE_2D, ftr->glyph_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RED, ftr->atlas_w, ftr->atlas_h, 0, 
        GL_RED, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int x = 0;
    FT_Error error;
    for (size_t i = 32; i < 128; i++) {
        if ((error = FT_Load_Char(face, i, load_flags))) {
            fprintf(stderr, "ERROR: %s\n", FT_Error_String(error));
            exit(1);
        }

        ftr->gi[i].ax = face->glyph->advance.x >> 6;
        ftr->gi[i].ay = face->glyph->advance.y >> 6;
        ftr->gi[i].bw = face->glyph->bitmap.width;
        ftr->gi[i].bh = face->glyph->bitmap.rows;
        ftr->gi[i].bl = face->glyph->bitmap_left;
        ftr->gi[i].bt = face->glyph->bitmap_top;
        ftr->gi[i].tx = (float) x / (float) ftr->atlas_w;

        glTexSubImage2D(
            GL_TEXTURE_2D, 0, x, 0, face->glyph->bitmap.width, 
            face->glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, 
            face->glyph->bitmap.buffer);

        x += face->glyph->bitmap.width;
    }
}

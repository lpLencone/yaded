#ifndef YADED_FREETYPEGLYPH_H_
#define YADED_FREETYPEGLYPH_H_

#include "gl_extra.h"
#include "la.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#ifndef FREETYPE_GLYPH_BUFFER_CAPACITY
#define FREETYPE_GLYPH_BUFFER_CAPACITY (1024 * 1024)
#endif // FREETYPE_GLYPH_BUFFER_CAPACITY

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%20s : %4d : %-20s\n", __FILE__, __LINE__, __func__);
#endif

typedef struct {
    Vec2f pos;
    Vec2f size;
    Vec2f uv_pos;
    Vec2f uv_size;
    Vec4f fg_color;
    Vec4f bg_color;
} FreeType_Glyph;

// https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_02
typedef struct {
  float ax; // advance.x
  float ay; // advance.y
  
  float bw; // bitmap.width;
  float bh; // bitmap.rows;
  
  float bl; // bitmap_left;
  float bt; // bitmap_top;
  
  float tx; // x offset of glyph in texture coordinates
} Glyph_Info;

typedef struct {
    FT_UInt atlas_w;
    FT_UInt atlas_h;

    GLuint program;

    GLuint glyph_texture;
    GLuint vao;
    GLuint vbo;

    GLint time;
    GLint scale;
    GLint camera;
    GLint resolution;

    FreeType_Glyph glyph[FREETYPE_GLYPH_BUFFER_CAPACITY];
    size_t glyph_count;

    Glyph_Info gi[128];

} FreeType_Glyph_Renderer;

void ftgr_init(FreeType_Glyph_Renderer *ftgr, FT_Face face,
               const char *vert_filename, const char *frag_filename);


void ftgr_clear(FreeType_Glyph_Renderer *ftgr);
void ftgr_sync(FreeType_Glyph_Renderer *ftgr);
void ftgr_draw(FreeType_Glyph_Renderer *ftgr);

void ftgr_render_string_n(FreeType_Glyph_Renderer *ftgr, const char *s, size_t n,
                          Vec2f pos, Vec4f fg_color, Vec4f bg_color);
void ftgr_render_string(FreeType_Glyph_Renderer *ftgr, const char *s, Vec2f pos, 
                        Vec4f fg_color, Vec4f bg_color);

float ftgr_get_s_width_n(FreeType_Glyph_Renderer *ftgr, const char *s, size_t n);
float ftgr_get_s_width_n_pad(FreeType_Glyph_Renderer *ftgr, const char *s, size_t n,
                             char pad); 

size_t ftgr_get_glyph_index_near(FreeType_Glyph_Renderer *ftgr, const char *s, 
                                 float width);
size_t ftgr_get_glyph_index_near_pad(FreeType_Glyph_Renderer *ftgr, const char *s,
                                     float width, char pad);


void ftgr_use(const FreeType_Glyph_Renderer *ftgr);

#endif // YADED_FREETYPEGLYPH_H_
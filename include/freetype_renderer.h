#ifndef MEDO_FREETYPE_RENDERER_H_
#define MEDO_FREETYPE_RENDERER_H_

#include "simple_renderer.h"

#include <ft2build.h>
#include <freetype/freetype.h>

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%15s : %4d : %-20s ", __FILE__, __LINE__, __func__);
#endif
#ifndef debug_println
#define debug_println debug_print puts("");
#endif

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
    GLuint glyph_texture;
    Glyph_Info gi[128];
} FreeType_Renderer;

void ftr_init(FreeType_Renderer *ftr, FT_Face face);

#define ftr_render_s(ftr, sr, s, pos, c) \
    ftr_render_s_n(ftr, sr, s, strlen(s), pos, c)
Vec2f ftr_render_s_n(
    FreeType_Renderer *ftr, Simple_Renderer *sr, 
    const char *s, size_t n, Vec2f pos, Vec4f c);

float ftr_get_s_width_n(FreeType_Renderer *ftr, const char *s, size_t n);
float ftr_get_s_width_n_pad(
    FreeType_Renderer *ftr, const char *s, size_t n, char pad); 

size_t ftr_get_glyph_index_near(
    FreeType_Renderer *ftr, const char *s, float width);
size_t ftr_get_glyph_index_near_pad(
    FreeType_Renderer *ftr, const char *s, float width, char pad);

#endif // MEDO_FREETYPE_RENDERER_H_
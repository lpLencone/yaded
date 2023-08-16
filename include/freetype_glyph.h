#ifndef YADED_FREETYPEGLYPH_H_
#define YADED_FREETYPEGLYPH_H_

#include "gl_extra.h"
#include "la.h"

#ifndef FREETYPE_GLYPH_BUFFER_CAPACITY
#define FREETYPE_GLYPH_BUFFER_CAPACITY (1024 * 1024)
#endif // FREETYPE_GLYPH_BUFFER_CAPACITY

typedef struct {
    Vec2f pos;
    Vec2f size;
    int ch;
    Vec4f fg_color;
    Vec4f bg_color;
} FreeType_Glyph;

typedef struct {
    // GLuint font_texture;
    GLuint vao;
    GLuint vbo;

    GLuint time;
    GLuint resolution;
    // GLuint scale;
    GLuint camera;

    FreeType_Glyph glyph[FREETYPE_GLYPH_BUFFER_CAPACITY];
    size_t glyph_count;
} FreeType_Glyph_Renderer;

void ftgr_init(FreeType_Glyph_Renderer *ftgr,
               const char *vert_filename, const char *frag_filename);


void ftgr_clear(FreeType_Glyph_Renderer *ftgr);
void ftgr_sync(FreeType_Glyph_Renderer *ftgr);
void ftgr_render_text(FreeType_Glyph_Renderer *ftgr, const char *s, Vec2i tile, 
                   Vec4f fg_color, Vec4f bg_color);
void ftgr_draw(FreeType_Glyph_Renderer *ftgr);


#endif // YADED_FREETYPEGLYPH_H_
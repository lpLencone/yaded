#ifndef YADED_TYLEGLYPH_H_
#define YADED_TYLEGLYPH_H_

#include "la.h"
#include "gl_extra.h"

typedef struct {
    Vec2i tile;
    int ch;
    Vec4f fg_color;
    Vec4f bg_color;
} Tile_Glyph;

typedef struct {
    size_t offset;
    GLint  comps;
    GLenum type;
} Attr_Def;

typedef struct {
    GLuint time;
    GLuint resolution;
    GLuint scale;
    GLuint camera;

    Tile_Glyph *buffer;
    size_t buffer_count;
    size_t buffer_capacity;
} Tile_Glyph_Renderer;

Tile_Glyph_Renderer tile_glyph_renderer_init(
    const char *atlas_filename, const char *vert_filename, 
    const char *frag_filename);

void tile_glyph_buffer_clear(Tile_Glyph_Renderer *tgr);
void tile_glyph_buffer_sync(Tile_Glyph_Renderer *tgr);
void tile_glyph_render_text(Tile_Glyph_Renderer *tgr, const char *s, Vec2i tile, Vec4f fg_color, Vec4f bg_color);
void tile_glyph_draw(Tile_Glyph_Renderer *tgr);


#endif // YADED_TYLEGLYPH_H_
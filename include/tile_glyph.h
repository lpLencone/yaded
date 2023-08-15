#ifndef YADED_TYLEGLYPH_H_
#define YADED_TYLEGLYPH_H_

#include "la.h"
#include "glyph.h"

typedef enum {
    TILE_GLYPH_ATTR_TILE = 0,
    TILE_GLYPH_ATTR_CH,
    TILE_GLYPH_ATTR_FG_COLOR,
    TILE_GLYPH_ATTR_BG_COLOR,
    COUNT_TILE_GLYPH_ATTRS,
} Tile_Glyph_Attr;

typedef struct {
    Vec2i tile;
    int ch;
    Vec4f fg_color;
    Vec4f bg_color;
} Tile_Glyph;

void tile_glyph_buffer_init(Glyph_Uniform *tgu, const char *atlas_filename, 
                            const char *vert_filename, const char *frag_filename);

void tile_glyph_buffer_clear(void);
void tile_glyph_buffer_push(Tile_Glyph glyph);
void tile_glyph_buffer_sync(void);
void tile_glyph_render_text(const char *s, Vec2i tile, Vec4f fg_color, Vec4f bg_color);
void tile_glyph_draw(void);


#endif // YADED_TYLEGLYPH_H_
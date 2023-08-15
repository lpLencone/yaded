#ifndef YADED_GLYPH_H_
#define YADED_GLYPH_H_

#include "gl_extra.h"

typedef struct {
    GLuint time;
    GLuint resolution;
    GLuint scale;
    GLuint camera;
} Glyph_Uniform;

typedef struct {
    size_t offset;
    GLint  comps;
    GLenum type;
} Attr_Def;

#endif // YADED_GLYPH_H_

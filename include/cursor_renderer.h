#ifndef YADED_CURSOR_RENDERER_H_
#define YADED_CURSOR_RENDERER_H_

#include "gl_extra.h"
#include "la.h"

typedef struct {
    GLuint program;

    GLuint resolution;
    GLuint camera;
    GLuint height;
    GLuint pos;
    GLuint time;
} Cursor_Renderer;

Cursor_Renderer cr_init(const char *vert_filename, const char *frag_filename);

void cr_move(const Cursor_Renderer *cr, Vec2f pos);
void cr_draw(void);
void cr_use(const Cursor_Renderer *cr);

#endif // YADED_CURSOR_RENDERER_H_
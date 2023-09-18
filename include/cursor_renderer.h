#ifndef MEDO_CURSOR_RENDERER_H_
#define MEDO_CURSOR_RENDERER_H_

#include "la.h"
#include "gl_extra.h"

typedef struct {
    GLuint program;

    GLuint time;
    GLuint last_moved;
    GLuint resolution;
    GLuint camera;
    GLuint scale;
    GLuint height;
    GLuint width;
    GLuint pos;

} Cursor_Renderer;

Cursor_Renderer cr_init(const char *vert_filename, const char *frag_filename);

void cr_use(const Cursor_Renderer *cr);
void cr_move(const Cursor_Renderer *cr, Vec2f pos);
void cr_draw(void);

#endif // MEDO_CURSOR_RENDERER_H_

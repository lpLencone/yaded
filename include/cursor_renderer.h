#ifndef YADED_CURSORRENDERER_H_
#define YADED_CURSORRENDERER_H_

#include "gl_extra.h"
#include "la.h"

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

#endif // YADED_CURSORRENDERER_H_
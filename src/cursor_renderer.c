#include "cursor_renderer.h"

#include <stdlib.h>

static void init_shaders(Cursor_Renderer *cursor, const char *vert_filename, 
                         const char *frag_filename);

Cursor_Renderer cr_init(const char *vert_filename, const char *frag_filename)
{
    Cursor_Renderer cursor;
    init_shaders(&cursor, vert_filename, frag_filename);
    return cursor;
}

void cr_move(const Cursor_Renderer *cr, Vec2f pos)
{
    glUniform2f(cr->pos, pos.x, pos.y);
}

void cr_draw(void)
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void cr_use(const Cursor_Renderer *cr)
{
    glUseProgram(cr->program);
}

static void init_shaders(Cursor_Renderer *cursor, const char *vert_filename, 
                         const char *frag_filename)
{
    GLuint vert_shader = 0;
    if (!compile_shader_file(vert_filename, GL_VERTEX_SHADER, &vert_shader)) {
        exit(1);
    };

    GLuint frag_shader = 0;
    if (!compile_shader_file(frag_filename, GL_FRAGMENT_SHADER, &frag_shader)) {
        exit(1);
    };

    if (!link_program(vert_shader, frag_shader, &cursor->program)) {
        exit(1);
    }

    glUseProgram(cursor->program);

    cursor->resolution = glGetUniformLocation(cursor->program, "resolution");
    cursor->camera = glGetUniformLocation(cursor->program, "camera");
    cursor->pos = glGetUniformLocation(cursor->program, "pos");
    cursor->height = glGetUniformLocation(cursor->program, "height");
    cursor->time = glGetUniformLocation(cursor->program, "time");
}
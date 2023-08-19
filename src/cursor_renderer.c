#include "cursor_renderer.h"

#include <stdlib.h>

static void get_uniforms(Cursor_Renderer *cr);

Cursor_Renderer cr_init(const char *vert_filename, const char *frag_filename)
{
    Cursor_Renderer cr;
    
    const char *shaders_filenames[] = {
        vert_filename, frag_filename, "shaders/common.vert", "shaders/common.frag",
    };
    size_t n_shaders = sizeof(shaders_filenames) / sizeof(shaders_filenames[0]);

    GLuint *shaders = malloc(n_shaders * sizeof(GLuint));

    cr.program = glCreateProgram();
    compile_shaders(shaders_filenames, n_shaders, shaders);
    attach_shaders(cr.program, shaders, n_shaders);
    if (!link_program(cr.program)) {
        fprintf(stderr, "At %s : %d\n", __FILE__, __LINE__);
    }

    free(shaders);

    get_uniforms(&cr);
    return cr;
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

static void get_uniforms(Cursor_Renderer *cr)
{
    cr_use(cr);
    
    cr->time = glGetUniformLocation(cr->program, "time");
    cr->last_moved = glGetUniformLocation(cr->program, "last_moved");
    cr->resolution = glGetUniformLocation(cr->program, "resolution");
    cr->camera = glGetUniformLocation(cr->program, "camera");
    cr->scale = glGetUniformLocation(cr->program, "scale");
    cr->height = glGetUniformLocation(cr->program, "height");
    cr->width = glGetUniformLocation(cr->program, "width");
    cr->pos = glGetUniformLocation(cr->program, "pos");
}
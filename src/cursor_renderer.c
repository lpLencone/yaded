#include "cursor_renderer.h"

#include <stdlib.h>

static void get_uniforms(Cursor_Renderer *cr);

Cursor_Renderer cr_init(const char *vert_filename, const char *frag_filename)
{
    Cursor_Renderer cr;
    
    GLshader gl_shaders[4] = {
        [0].filename = vert_filename,
        [0].shader_type = GL_VERTEX_SHADER,

        [1].filename = frag_filename,
        [1].shader_type = GL_FRAGMENT_SHADER,

        [2].filename = "shaders/common.vert",
        [2].shader_type = GL_VERTEX_SHADER,

        [3].filename = "shaders/common.frag",
        [3].shader_type = GL_FRAGMENT_SHADER,
    };
    compile_shaders(&cr.program, gl_shaders, sizeof(gl_shaders) / sizeof(gl_shaders[0]));

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
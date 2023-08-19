#ifndef YADED_GLEXTRA_H_
#define YADED_GLEXTRA_H_

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%20s : %4d : %-20s\n", __FILE__, __LINE__, __func__);
#endif

#define GLEW_STATIC
#include <GL/glew.h>

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include <stdbool.h>

typedef struct {
    const char *filename;
    GLenum shader_type;
} GLshader;

bool compile_shaders(GLuint *program, const GLshader *shaders, size_t n_shaders);

#endif // YADED_GLEXTRA_H_


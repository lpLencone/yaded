#include "gl_extra.h"
#include "file.h"

#include <SDL2/SDL_opengl.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *shader_type_as_cstr(GLuint shader);
static bool link_program(GLuint *program, GLuint *shaders, size_t n_shaders);
static bool compile_shader_file(const char *filename, GLenum shader_type, GLuint *shader);
static bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader);

bool compile_shaders(GLuint *program, const GLshader *gl_shaders, size_t n_shaders)
{
    GLuint *shaders = malloc(sizeof(GLuint) * n_shaders);
    
    for (size_t i = 0; i < n_shaders; i++) {
        if (!compile_shader_file(gl_shaders[i].filename, 
                                 gl_shaders[i].shader_type, 
                                 &shaders[i])) 
        {
            return false;
        }
    }

    if (!link_program(program, shaders, n_shaders)) {
        return false;
    }

    free(shaders);
    return true;
}

static bool compile_shader_file(const char *filename, GLenum shader_type, GLuint *shader)
{
    char *source = slurp_file_into_malloced_cstr(filename);
    
    if (source == NULL) {
        fprintf(stderr, "ERROR: failed to read file `%s`: %s\n", filename, strerror(errno));
        errno = 0;
        return false;
    }

    if (!compile_shader_source(source, shader_type, shader)) {
        fprintf(stderr, "ERROR: failed to compile `%s` shader file\n", filename);
        return false;
    }

    free(source);
    return true;
}

static bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader)
{
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
        fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(shader_type));
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }

    return true;
}

static bool link_program(GLuint *program, GLuint *shaders, size_t n_shaders)
{
    *program = glCreateProgram();

    for (size_t i = 0; i < n_shaders; i++) {
        glAttachShader(*program, shaders[i]);
    }

    glLinkProgram(*program);

    GLint linked = 0;
    glGetProgramiv(*program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
        fprintf(stderr, "Program Linking: %.*s\n", message_size, message);
    }

    for (size_t i = 0; i < n_shaders; i++) {
        glDeleteShader(shaders[i]);
    }

    return program;
}

static const char *shader_type_as_cstr(GLuint shader)
{
    switch (shader) {
        case GL_VERTEX_SHADER:
            return "GL_VERTEX_SHADER";
        case GL_FRAGMENT_SHADER:
            return "GL_FRAGMENT_SHADER";
        default:
            return "(Unknown)";
    }
}

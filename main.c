#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#define GLEW_STATIC
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#define OPENGL_RENDERER

#include "la.h"
#include "editor.h"
#include "gl_extra.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       600
#define FPS                 60
#define DELTA_TIME          (1.0f / FPS)
#define DELTA_TIME_MS       (1000 / FPS)

#define FONT_WIDTH          128
#define FONT_HEIGHT         64
#define FONT_COLS           18
#define FONT_ROWS           7
#define FONT_CHAR_WIDTH     (int) (FONT_WIDTH  / FONT_COLS)
#define FONT_CHAR_HEIGHT    (int) (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE          2.0f

// SDL check codes
void scc(int code)
{
    if (code < 0) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
}

// SDL check pointer
void *scp(void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }

    return ptr;
}

SDL_Surface *surface_from_file(const char *filename)
{
    int width, height, n;
    unsigned char *pixels = stbi_load(filename, &width, &height, &n, STBI_rgb_alpha);
    if (pixels == NULL) {
        fprintf(stderr, "ERROR: could not load file %s: %s\n", filename, stbi_failure_reason());
        exit(1);
    }

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const Uint32 rmask = 0xff000000;
    const Uint32 gmask = 0x00ff0000;
    const Uint32 bmask = 0x0000ff00;
    const Uint32 amask = 0x000000ff;
#else // little endian, like x86
    const Uint32 rmask = 0x000000ff;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x00ff0000;
    const Uint32 amask = 0xff000000;
#endif

    const int depth = 32;
    const int pitch = 4 * width;

    return scp(
        SDL_CreateRGBSurfaceFrom((void *) pixels, width, height, depth, pitch, 
                                 rmask, gmask, bmask, amask)
    );  
}

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

typedef struct {
    SDL_Texture *spritesheet;
    SDL_Rect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

Font font_load_from_file(SDL_Renderer *renderer, const char *filename)
{
    Font font = {0};

    SDL_Surface *font_surface = surface_from_file(filename);
    scc(SDL_SetColorKey(font_surface, SDL_TRUE, 0xFF000000));
    font.spritesheet = scp(SDL_CreateTextureFromSurface(renderer, font_surface));

    SDL_FreeSurface(font_surface);

    for (size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ascii++) {
        const size_t index = ascii - ASCII_DISPLAY_LOW;
        const size_t col = index % FONT_COLS;
        const size_t row = index / FONT_COLS;

        font.glyph_table[index] = (SDL_Rect) {
            .x = col * FONT_CHAR_WIDTH,
            .y = row * FONT_CHAR_HEIGHT,
            .w = FONT_CHAR_WIDTH,
            .h = FONT_CHAR_HEIGHT,
        };
    }

    return font;
}

void render_char(SDL_Renderer *renderer, const Font *font, char c, const Vec2f pos, float scale)
{
    assert(c >= ASCII_DISPLAY_LOW);
    assert(c <= ASCII_DISPLAY_HIGH);

    const SDL_Rect destine = {
        .x = (int) floorf(pos.x),
        .y = (int) floorf(pos.y),
        .w = (int) floorf(scale * FONT_CHAR_WIDTH),
        .h = (int) floorf(scale * FONT_CHAR_HEIGHT)
    };

    const size_t index = c - ASCII_DISPLAY_LOW;
    scc(SDL_RenderCopy(renderer, font->spritesheet, &font->glyph_table[index], &destine));
}

void set_texture_color(SDL_Texture *texture, Uint32 color)
{
    scc(SDL_SetTextureColorMod(
        texture, 
        (color >> (0 * 8)) & 0xff, 
        (color >> (1 * 8)) & 0xff,
        (color >> (2 * 8)) & 0xff
    ));

    scc(SDL_SetTextureAlphaMod(texture, (color >> (3 * 8)) & 0xff));
}

void render_text_sized(SDL_Renderer *renderer, Font *font, const char *s, 
                       size_t text_size, Vec2f pos, Uint32 color, float scale)
{
    set_texture_color(font->spritesheet, color);

    Vec2f pen = pos;
    for (size_t i = 0; i < text_size; i++) {
        render_char(renderer, font, s[i], pen, scale);
        pen.x += scale * FONT_CHAR_WIDTH;
    }
}

Editor e = {0};

Vec2f camera_pos = {0};
Vec2f camera_vel = {0};

#define UNHEX(color)            \
    (color) >> (0 * 8) & 0xff,  \
    (color) >> (0 * 1) & 0xff,  \
    (color) >> (0 * 2) & 0xff,  \
    (color) >> (0 * 3) & 0xff   \

Vec2f window_size(SDL_Window *window)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return vec2f((float) w, (float) h);
}

Vec2f camera_project_point(SDL_Window *window, Vec2f point)
{
    return vec2f_add(
        vec2f_sub(point, camera_pos),
        vec2f_mul(window_size(window), vec2fs(0.5f))
    );
}

void render_cursor(SDL_Window *window, SDL_Renderer *renderer, const Font *font, float scale)
{
    size_t ecx_pos = (e.cx > get_line_length(&e)) ? get_line_length(&e) : e.cx;

    Vec2f cursor_render_pos = camera_project_point(
        window, 
        vec2f(ecx_pos * FONT_CHAR_WIDTH  * scale, 
              e.cy    * FONT_CHAR_HEIGHT * scale)
    );

    SDL_Rect rect = {
        .x = (int) floorf(cursor_render_pos.x),
        .y = (int) floorf(cursor_render_pos.y),
        .w = FONT_CHAR_WIDTH * scale,
        .h = FONT_CHAR_HEIGHT * scale,
    };

    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFFFFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));

    // ABGR
    set_texture_color(font->spritesheet, 0xFF000000);

    Line *line = list_get(&e.lines, e.cy);
    if (e.cy != e.lines.length && e.cx < line->size) {
        render_char(renderer, font, line->s[e.cx], cursor_render_pos, scale);
    }
}

const int keymap[] = {
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_UP,
    SDLK_DOWN,
    SDLK_HOME,
    SDLK_END,
    SDLK_PAGEUP,
    SDLK_PAGEDOWN,
    SDLK_BACKSPACE,
    SDLK_DELETE,
    SDLK_RETURN,
    SDLK_TAB,
    SDLK_F3,
};

const size_t keymap_size = sizeof(keymap) / sizeof(keymap[0]);

EditorKeys find_key(int code) {
    for (size_t i = 0; i < keymap_size; i++) {
        if (keymap[i] == code) return i;
    }
    assert(0);
}

#ifndef OPENGL_RENDERER

int main(int argc, char *argv[])
{
    char *filename = NULL;
    if (argc > 1) {
        filename = argv[1];
    }

    scc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window = scp(
        SDL_CreateWindow("Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                         SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE)
    );

    SDL_Renderer *renderer = scp(
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)
    );

    Font font = font_load_from_file(renderer, "charmap-oldschool_white.png");

    e = editor_init(filename);

    float camera_scale = FONT_SCALE;

    bool quit = false;
    while (!quit) {
        const Uint32 start = SDL_GetTicks();
        
        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = true;
                } break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.sym) {
                        case SDLK_BACKSPACE: 
                        case SDLK_DELETE: 
                        case SDLK_RETURN: 
                        case SDLK_TAB: 
                        case SDLK_F3:
                        case SDLK_LEFT:
                        case SDLK_RIGHT: 
                        case SDLK_UP:
                        case SDLK_DOWN:
                        case SDLK_HOME: 
                        case SDLK_END:
                        case SDLK_PAGEUP:
                        case SDLK_PAGEDOWN: {
                            editor_process_key(&e, find_key(event.key.keysym.sym));
                        }
                    }
                } break;

                case SDL_MOUSEWHEEL: {
                    camera_scale *= (event.wheel.y > 0) ? 1.05 : 0.95;
                    assert(camera_scale > 0);
                } break;

                case SDL_TEXTINPUT: {
                    editor_insert_text(&e, event.text.text);
                } break;
            }
        }

        // size_t ecx_pos = (e.cx > get_line_length(&e)) ? get_line_length(&e) : e.cx;

        const Vec2f cursor_pos = vec2f(e.cx * FONT_CHAR_WIDTH  * camera_scale, 
                                       e.cy * FONT_CHAR_HEIGHT * camera_scale);

        camera_vel = vec2f_mul(vec2f_sub(cursor_pos, camera_pos), vec2fs(2.0f));
        camera_pos = vec2f_add(camera_pos, vec2f_mul(camera_vel, vec2fs(DELTA_TIME)));

        /*  Clear the screen */
        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));
        for (size_t cy = 0; cy < e.lines.length; cy++) {
            Line *line = list_get(&e.lines, cy);
            
            Vec2f line_pos = camera_project_point(
                window, 
                vec2f(0.0f, cy * camera_scale * FONT_CHAR_HEIGHT)
            );

            render_text_sized(renderer, &font, line->s, line->size, line_pos, 0xFFFFFFFF, camera_scale);
        }
        render_cursor(window, renderer, &font, camera_scale);

        SDL_RenderPresent(renderer);

        const Uint32 duration = (SDL_GetTicks() - start);
        if (duration < DELTA_TIME_MS) {
            SDL_Delay(DELTA_TIME_MS - duration);
        }
    }

    SDL_Quit();

    return 0;
}

#else

void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                     GLsizei length, const GLchar* message, const void* userParam)
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;
    
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

void gl_attr()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int major;
    int minor;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
    printf("GL version %d.%d\n", major, minor);
}

void init_shaders(GLint *time_uniform, GLint *resolution_uniform, GLint *scale_uniform)
{
    GLuint vert_shader = 0;
    if (!compile_shader_file("./shaders/font.vert", GL_VERTEX_SHADER, &vert_shader)) {
        exit(1);
    };

    GLuint frag_shader = 0;
    if (!compile_shader_file("./shaders/font.frag", GL_FRAGMENT_SHADER, &frag_shader)) {
        exit(1);
    };

    GLuint program = 0;
    if (!link_program(vert_shader, frag_shader, &program)) {
        exit(1);
    }

    glUseProgram(program);

    *time_uniform = glGetUniformLocation(program, "time");
    *resolution_uniform = glGetUniformLocation(program, "resolution");
    *scale_uniform = glGetUniformLocation(program, "scale");
}

void init_font_texture(void)
{
    const char *filename = "charmap-oldschool_white.png";
    int width, height, n;
    unsigned char *pixels = stbi_load(filename, &width, &height, &n, STBI_rgb_alpha);
    if (pixels == NULL) {
        fprintf(stderr, "ERROR: could not load file %s: %s\n", filename, stbi_failure_reason());
        exit(1);
    }

    GLuint font_texture = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

typedef struct {
    Vec2i tile;
    int ch;
    Vec4f color;
} Glyph;

typedef enum {
    GLYPH_ATTR_TILE = 0,
    GLYPH_ATTR_CH,
    GLYPH_ATTR_COLOR,
    COUNT_GLYPH_ATTRS,
} Glyph_Attr;

typedef struct {
    size_t offset;
    size_t comps;
    GLenum type;
} Glyph_Attr_Def;

static const Glyph_Attr_Def glyph_attr_defs[COUNT_GLYPH_ATTRS] = {
    [GLYPH_ATTR_TILE]  = {
        .offset = offsetof(Glyph, tile),
        .comps = 2,
        .type = GL_INT,
    },
    [GLYPH_ATTR_CH]    = {
        .offset = offsetof(Glyph, ch),
        .comps = 1,
        .type = GL_INT,
    },
    [GLYPH_ATTR_COLOR] = {
        .offset = offsetof(Glyph, color),
        .comps = 4,
        .type = GL_FLOAT,
    },
};

static_assert(COUNT_GLYPH_ATTRS == 3, "The amount of glyph vertex attributes has changed");

#define GLYPH_BUFFER_CAPACITY   1024

Glyph glyph_buffer[GLYPH_BUFFER_CAPACITY];
size_t glyph_buffer_count = 0;

void init_buffers(void)
{
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glyph_buffer), glyph_buffer, GL_DYNAMIC_DRAW);

    for (Glyph_Attr attr = 0; attr < COUNT_GLYPH_ATTRS; attr++) {
        glEnableVertexAttribArray(attr);
        switch (glyph_attr_defs[attr].type) {
            case GL_FLOAT: {
                glVertexAttribPointer(
                    attr,
                    glyph_attr_defs[attr].comps, 
                    glyph_attr_defs[attr].type, 
                    GL_FALSE, sizeof(Glyph), 
                    (void *) glyph_attr_defs[attr].offset
                );
            } break;

            case GL_INT: {
                glVertexAttribIPointer(
                    attr,
                    glyph_attr_defs[attr].comps,
                    glyph_attr_defs[attr].type,
                    sizeof(Glyph),
                    (void *) glyph_attr_defs[attr].offset
                );
            } break;
            
            default:
                assert(false && "Unreachable");
                exit(1); // why would anyone disable assert though?
        }
        glVertexAttribDivisor(attr, 1);
    }
}

void glyph_buffer_push(Glyph glyph)
{
    assert(glyph_buffer_count < GLYPH_BUFFER_CAPACITY);
    glyph_buffer[glyph_buffer_count++] = glyph;
}

void glyph_buffer_sync(void)
{
    glBufferSubData(GL_ARRAY_BUFFER, 
                    0, 
                    glyph_buffer_count * sizeof(Glyph), 
                    glyph_buffer);
}

void gl_render_text(const char *s, size_t slen, Vec2i tile, Vec4f color)
{
    for (size_t i = 0; i < slen; i++) {
        Glyph glyph = {
            .tile  = vec2i_add(tile, vec2i(i, 0)),
            .ch    = s[i],
            .color = color
        };
        glyph_buffer_push(glyph);
    }
}

int main(void)
{
    scc(SDL_Init(SDL_INIT_VIDEO));

    gl_attr();

    SDL_Window *window = scp(
        SDL_CreateWindow("Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                         SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL)
    );

    // SDL_GLContext context =
    scp(SDL_GL_CreateContext(window));
    
    const GLenum glInitStatus = glewInit();
    if (glInitStatus != GLEW_OK) {
        fprintf(stderr, "%s\n", glewGetErrorString(glInitStatus));
        exit(1);
    }

    if (!GLEW_ARB_draw_instanced) {
        fprintf(stderr, "ARB_draw_instanced is not supported; game may not work properly.\n");
        exit(1);
    }

    if (!GLEW_ARB_instanced_arrays) {
        fprintf(stderr, "ARB_instaced_arrays is not supported; game may not work properly.\n");
        exit(1);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(MessageCallback, 0);
    } else {
        fprintf(stderr, "WARNING! GLEW_ARB_debug_output is not available\n");
    }

    GLint time_uniform = 0;
    GLint resolution_uniform = 0;
    GLint scale_uniform = 0;
    init_shaders(&time_uniform, &resolution_uniform, &scale_uniform);
    glUniform2f(resolution_uniform, SCREEN_WIDTH, SCREEN_HEIGHT);
    glUniform2f(scale_uniform, 3.0f, 3.0f);

    init_font_texture();
    init_buffers();

    const char *s = "Hello, World!";
    gl_render_text(s, strlen(s), vec2is(0), vec4f(1.0f, 0.0f, 0.0f, 1.0f));
    glyph_buffer_sync();

    s = "Foo Bar";
    gl_render_text(s, strlen(s), vec2i(0, 1), vec4f(0.0f, 1.0f, 0.0f, 1.0f));
    glyph_buffer_sync();

    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = true;
                } break;

                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                    // Doesn't ever get here at all
                }
            }
        }

        {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glUniform2f(resolution_uniform, (float) w, (float) h);
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1f(time_uniform, (float) SDL_GetTicks() / 1000.0f);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, glyph_buffer_count);

        SDL_GL_SwapWindow(window);
    }

    return 0;
}


#endif // OPENGL_RENDERER

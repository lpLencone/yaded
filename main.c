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
#include "tile_glyph.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       600
#define FPS                 60
#define DELTA_TIME          (1.0f / FPS)
#define DELTA_TIME_MS       (1000 / FPS)

#define FONT_WIDTH          128.0f
#define FONT_HEIGHT         64.0f
#define FONT_COLS           18.0f
#define FONT_ROWS           7.0f
#define FONT_CHAR_WIDTH     (FONT_WIDTH  / FONT_COLS)
#define FONT_CHAR_HEIGHT    (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE          3.0f

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

void gl_attr(void)
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

void init_glew(void)
{
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

}

typedef struct {
    Vec2f pos;
    Vec2f size;
    int ch;
    Vec4f fg_color;
    Vec4f bg_color;
} FreeType_Glyph;

void gl_render_cursor(Editor *e)
{
    const Line *line = list_get(&e->lines, e->cy);
    char c[2] = {0};
    size_t ecx;
    if (e->cx < line->size) {
        c[0] = line->s[e->cx];
        ecx = e->cx;
    } else {
        c[0] = ' ';
        ecx = line->size;
    }

    tile_glyph_render_text(c, vec2i(ecx, -e->cy), vec4fs(0.0f), vec4fs(1.0f));
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

int main(int argc, char *argv[])
{
    Glyph_Uniform tgu = {
        .time = 0,
        .resolution = 0,
        .scale = 0,
        .camera = 0,
    };

    SDL_Window *window;
    
    scc(SDL_Init(SDL_INIT_VIDEO));

    gl_attr();

    window = scp(
        SDL_CreateWindow("Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL)
    );

    scp(SDL_GL_CreateContext(window));

    init_glew();
    
    tile_glyph_buffer_init(&tgu, "charmap-oldschool_white.png", "shaders/tile_glyph.vert", 
                           "shaders/tile_glyph.frag");

    glUniform2f(tgu.resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
    glUniform2f(tgu.scale, FONT_SCALE, FONT_SCALE);

    char *filename = NULL;
    if (argc > 1) {
        filename = argv[1];
    }

    Editor e = editor_init(filename);
    Vec2f camera_pos = {0};
    Vec2f camera_vel = {0};
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

                case SDL_TEXTINPUT: {
                    editor_insert_text(&e, event.text.text);
                } break;

                case SDL_MOUSEBUTTONDOWN: {
                    Vec2f mouse_coord = vec2f((float) event.button.x, (float) event.button.y);

                    switch (event.button.button) {
                        case SDL_BUTTON_LEFT: {
                            int w, h;
                            SDL_GetWindowSize(window, &w, &h);
                            Vec2f cursor_coord = vec2f_add(
                                mouse_coord, 
                                vec2f_sub(camera_pos, vec2f((float) w / 2, (float) h / 2))
                            );
                            int truex = (int) floorf(cursor_coord.x / ((float) FONT_CHAR_WIDTH * camera_scale));
                            if (truex < 0) {
                                truex = 0;
                            }
                            int truey = (int) floorf(cursor_coord.y / ((float) FONT_CHAR_HEIGHT * camera_scale)) + 1;
                            if (truey < 0) {
                                truey = 0;
                            }

                            editor_click(&e, (size_t) truex, (size_t) truey);
                        } break;
                    }
                } break;

                case SDL_MOUSEWHEEL: {
                    camera_scale *= (event.wheel.y > 0) ? 1.05 : 0.95;
                    assert(camera_scale > 0);
                    glUniform2f(tgu.scale, camera_scale, camera_scale);
                } break;
            }
        }

        {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glUniform2f(tgu.resolution, (float) w, (float) h);
        }

        const Vec2f cursor_pos = vec2f(e.cx * FONT_CHAR_WIDTH  * camera_scale, 
                                       e.cy * FONT_CHAR_HEIGHT * camera_scale);

        camera_vel = vec2f_mul(vec2f_sub(cursor_pos, camera_pos), vec2fs(2.0f));
        camera_pos = vec2f_add(camera_pos, vec2f_mul(camera_vel, vec2fs(DELTA_TIME)));

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1f(tgu.time, (float) SDL_GetTicks() / 1000.0f);
        glUniform2f(tgu.camera, camera_pos.x, -camera_pos.y);

        tile_glyph_buffer_clear();
        for (int cy = 0; cy < (int) e.lines.length; cy++) {
            const Line *line = list_get(&e.lines, cy);
            tile_glyph_render_text(line->s, vec2i(0, -cy), vec4fs(1.0f), vec4fs(0.0f));
        }
        gl_render_cursor(&e);
        tile_glyph_buffer_sync();

        tile_glyph_draw();

        const Uint32 duration = (SDL_GetTicks() - start);
        if (duration < DELTA_TIME_MS) {
            SDL_Delay(DELTA_TIME_MS - duration);
        }

        SDL_GL_SwapWindow(window);
    }

    return 0;
}

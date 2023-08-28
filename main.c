#include <math.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#define GLEW_STATIC
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include "la.h"
#include "editor.h"
#include "gl_extra.h"

#include "freetype_renderer.h"
#include "simple_renderer.h"
#define FONT_SIZE                   64
    
#define FONT_FILENAME               "fonts/VictorMono-Regular.ttf"
// #define CURSOR_VERT_FILENAME        "shaders/cursor_bar.vert"
// #define CURSOR_FRAG_FILENAME        "shaders/cursor_bar_smooth_blinking.frag"
// #define CURSOR_FRAG_FILENAME        "shaders/cursor_bar.frag"

#define CURSOR_WIDTH                5.0f
#define CURSOR_VELOCITY             20.0f
#define LINE_SEPARATION_HEIGHT      5 // pixels ig
#define INITIAL_SCALE               1.8f
#define FINAL_SCALE                 0.5f
#define MAX_LINE_S_DISPLAY          20.0f
#define ZOOM_THRESHOLD              250.0 * (INITIAL_SCALE / FINAL_SCALE)

#define SCREEN_WIDTH                800
#define SCREEN_HEIGHT               600
#define FPS                         60
#define DELTA_TIME                  (1.0f / FPS)
#define DELTA_TIME_MS               (1000 / FPS)

typedef struct {
    struct {
        Vec2f pos;
        Vec2f vel;
        float scale;
        float scale_vel;
    } cam;
    struct {
        Vec2f actual_pos;
        Vec2f render_pos;
        Vec2f vel;
        size_t last_cx;
        size_t last_cy;
        Uint32 last_moved; // in milisec
    } cur;
} Screen;

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

EditorKey find_move_key(int code) {
    for (EditorKey ek = EDITOR_MOVE_KEY_START; ek < EDITOR_MOVE_KEY_END; ek++) {
        if (keymap[ek] == code) return ek;
    }
    assert(0);
}

EditorKey find_edit_key(int code) {
    for (EditorKey ek = EDITOR_EDIT_KEY_START; ek < EDITOR_EDIT_KEY_END; ek++) {
        if (keymap[ek] == code) return ek;
    }
    assert(0);
}

EditorKey find_action_key(int code) {
    for (EditorKey ek = EDITOR_ACTION_KEY_START; ek < EDITOR_ACTION_KEY_END; ek++) {
        if (keymap[ek] == code) return ek;
    }
    assert(0);
}

FT_Face FT_init(void)
{
#define ft_check_error                                          \
    if (error) {                                                \
        fprintf(stderr, "ERROR: %s", FT_Error_String(error));   \
        exit(1);                                                \
    }                                                           \

    FT_Library library = {0};
    FT_Error error = FT_Init_FreeType(&library);
    ft_check_error

    const char *font_filename = FONT_FILENAME;
    FT_Face face;
    error = FT_New_Face(library, font_filename, 0, &face);
    ft_check_error   

    FT_UInt pixel_size = FONT_SIZE;
    error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
    ft_check_error

    return face;
#undef ft_check_error
}

void renderers_init(Simple_Renderer *sr, FreeType_Renderer *ftr, FT_Face face)
{
    ftr_init(ftr, face);

    sr_init(sr, "shaders/simple.vert", "shaders/simple_color.frag", "shaders/simple_image.frag", "shaders/simple_pride.frag");
    sr_set_shader(sr, SHADER_COLOR);
    glUniform2f(sr->resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
    sr_set_shader(sr, SHADER_IMAGE);
    glUniform2f(sr->resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
    sr_set_shader(sr, SHADER_PRIDE);
    glUniform2f(sr->resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
static_assert(SHADER_COUNT == 3, 
              "The number of shaders has changed; update resolution for the new shaders");
}

void renderer_draw(Simple_Renderer *sr, FreeType_Renderer *ftr,
                   Editor *e, Screen *scr)
{
    // Render Glyphs
    sr_set_shader(sr, SHADER_PRIDE);

    glUniform1f(sr->time, (float) SDL_GetTicks() / 1000.0f);
    glUniform2f(sr->camera, scr->cam.pos.x, -scr->cam.pos.y);
    glUniform2f(sr->scale, scr->cam.scale, scr->cam.scale);

    float max_line_width = 0;
    
    Vec2f pos = {0};
    for (size_t cy = 0; cy < e->lines.length; cy++) {
        pos.x = 0.0f;
        pos.y = - (float) cy * (FONT_SIZE);
        const char *s = editor_get_line_at(e, cy);
        ftr_render_string(ftr, sr, s, pos);

        float line_width = ftr_get_s_width_n(ftr, s, strlen(s)) / 0.5f;
        if (line_width > max_line_width) max_line_width = line_width;
    }
  
    sr_sync(sr);
    sr_draw(sr);
    sr_clear(sr);

    // Render Cursor

    sr_set_shader(sr, SHADER_COLOR);

    glUniform1f(sr->time, (float) SDL_GetTicks() / 1000.0f);
    glUniform2f(sr->camera, scr->cam.pos.x, -scr->cam.pos.y);
    glUniform2f(sr->scale, scr->cam.scale, scr->cam.scale);

    Uint32 CURSOR_BLINK_THRESHOLD = 500;
    Uint32 CURSOR_BLINK_PERIOD    = 500;
    Uint32 t = SDL_GetTicks() - scr->cur.last_moved;
    if (t < CURSOR_BLINK_THRESHOLD || (t / CURSOR_BLINK_PERIOD) % 2 != 0) {
        sr_solid_rect(sr, vec2f(scr->cur.render_pos.x, -scr->cur.render_pos.y),
                          vec2f(CURSOR_WIDTH, FONT_SIZE),
                          vec4fs(1.0));
    }

    sr_sync(sr);
    sr_draw(sr);
    sr_clear(sr);

    // Update camera scale

    float target_scale = SCREEN_WIDTH / max_line_width; 

    if (target_scale > INITIAL_SCALE) {
        target_scale = INITIAL_SCALE;
    } else if (target_scale < FINAL_SCALE) {
        target_scale = FINAL_SCALE;
    }
    
    scr->cam.scale_vel = 2.0f * (target_scale - scr->cam.scale);
    scr->cam.scale += scr->cam.scale_vel * DELTA_TIME;
}

void update_last_moved(Screen *scr)
{
    scr->cur.last_moved = SDL_GetTicks();
}

void cursor_move(Screen *scr, size_t cx, size_t cy)
{
    scr->cur.last_cx = cx;
    scr->cur.last_cy = cy;
}

static FreeType_Renderer ftr = {0};
static Simple_Renderer sr = {0};

int main(int argc, char *argv[])
{
    FT_Face face = FT_init();

    scc(SDL_Init(SDL_INIT_VIDEO));

    gl_attr();
    SDL_Window *window;
    window = scp(
        SDL_CreateWindow("ged: geimer editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                         SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL)
    );
    scp(SDL_GL_CreateContext(window));
    init_glew();
    renderers_init(&sr, &ftr, face);

    char *filename = NULL;
    if (argc > 1) {
        filename = argv[1];
    }

    Editor e = editor_init(filename);
    Screen scr = {0};
    scr.cam.scale = INITIAL_SCALE;

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
                        case SDLK_LEFT:
                        case SDLK_RIGHT:
                        case SDLK_UP:
                        case SDLK_DOWN:
                        case SDLK_HOME:
                        case SDLK_END:
                        case SDLK_PAGEUP:
                        case SDLK_PAGEDOWN: {
                            editor_process_key(&e, find_move_key(event.key.keysym.sym));
                            update_last_moved(&scr);
                            // update_last_stroke(&cr);
                        } break;

                        case SDLK_BACKSPACE:
                        case SDLK_DELETE:
                        case SDLK_RETURN:
                        case SDLK_TAB: {
                            editor_process_key(&e, find_edit_key(event.key.keysym.sym));
                            update_last_moved(&scr);
                            // update_last_stroke(&cr);
                        } break;

                        case SDLK_F3: {
                            editor_process_key(&e, find_action_key(event.key.keysym.sym));
                        } break;
                    }
                } break;

                case SDL_TEXTINPUT: {
                    editor_insert_s(&e, event.text.text);
                    update_last_moved(&scr);
                } break;
            }
        }

        // TODO: set viewport only on window change

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);

        // sr_set_shader(&sr, SHADER_COLOR);
        // glUniform2f(sr.resolution, (float) w, (float) h);

        // Update Cursor
        if (e.cy != scr.cur.last_cy) {
            const char *s = editor_get_line_at(&e, scr.cur.last_cy);
            const float last_width = (s != NULL) ? ftr_get_s_width_n(&ftr, s, e.cx) : 0;
            size_t ecx = ftr_get_glyph_index_near(&ftr, editor_get_line(&e), last_width);
            cursor_move(&scr, ecx, e.cy);
        }
        if (e.cx != scr.cur.last_cx) {
            cursor_move(&scr, e.cx, e.cy);
        }

        // Update cursor position on the screen
        scr.cur.actual_pos.y = e.cy * FONT_SIZE;
        size_t n = editor_get_line_size(&e);
        scr.cur.actual_pos.x = 
            ftr_get_s_width_n(&ftr, editor_get_line(&e), (e.cx > n) ? n : e.cx);

        scr.cur.vel = vec2f_mul(
            vec2f_sub(scr.cur.render_pos, scr.cur.actual_pos),
            vec2fs(CURSOR_VELOCITY)
        );
        scr.cur.render_pos = vec2f_sub(
            scr.cur.render_pos,
            vec2f_mul(scr.cur.vel, vec2fs(DELTA_TIME))
        );

        // Update camera position on the screen

        scr.cam.vel = vec2f_mul(
            vec2f_sub(scr.cur.render_pos, scr.cam.pos), 
            vec2fs(2.0f)
        );
        scr.cam.pos = vec2f_add(
            scr.cam.pos, 
            vec2f_mul(scr.cam.vel, vec2fs(DELTA_TIME))
        );

        // Draw
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer_draw(&sr, &ftr, &e, &scr);

        const Uint32 duration = (SDL_GetTicks() - start);
        if (duration < DELTA_TIME_MS) {
            SDL_Delay(DELTA_TIME_MS - duration);
        }

        SDL_GL_SwapWindow(window);
    }

    return 0;
}


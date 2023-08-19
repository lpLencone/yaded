#include <math.h>

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

// #define TILE_GLYPH_RENDERER

#ifdef TILE_GLYPH_RENDERER
#   include "tile_glyph.h"
#   define FONT_WIDTH          128.0f
#   define FONT_HEIGHT         64.0f
#   define FONT_COLS           18.0f
#   define FONT_ROWS           7.0f
#   define FONT_CHAR_WIDTH     (FONT_WIDTH  / FONT_COLS)
#   define FONT_CHAR_HEIGHT    (FONT_HEIGHT / FONT_ROWS)
#   define FONT_SCALE          3.0f
#else 
#   include "freetype_glyph.h"
#   include "cursor_renderer.h"
#   define FONT_SIZE                24

#   define FONT_FILENAME            "fonts/VictorMono-Regular.ttf"
#   define CURSOR_VERT_FILENAME     "shaders/cursor_bar.vert"
#   define CURSOR_FRAG_FILENAME     "shaders/cursor_bar_smooth_blinking.frag"
// #   define CURSOR_FRAG_FILENAME     "shaders/cursor_bar.frag"
#   define CURSOR_VELOCITY          20.0f
#   define LINE_SEPARATION_HEIGHT   5 // pixels ig
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       600
#define FPS                 60
#define DELTA_TIME          (1.0f / FPS)
#define DELTA_TIME_MS       (1000 / FPS)


typedef struct {
    struct {
        Vec2f pos;
        Vec2f vel;
    } cam;
    struct {
        Vec2f actual_pos;
        Vec2f render_pos;
        Vec2f vel;
        size_t last_cx;
        size_t last_cy;
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

EditorKeys find_key(int code) {
    for (size_t i = 0; i < keymap_size; i++) {
        if (keymap[i] == code) return i;
    }
    assert(0);
}

#ifdef TILE_GLYPH_RENDERER

static Tile_Glyph_Renderer tgr = {0};

void renderer_init(Tile_Glyph_Renderer *tgr)
{
    tgr_init(tgr, "charmap-oldschool_white.png", "shaders/tile_glyph.vert", 
             "shaders/tile_glyph.frag");

    glUniform2f(tgr->resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
    glUniform2f(tgr->scale, FONT_SCALE, FONT_SCALE);
}

void gl_render_cursor(Tile_Glyph_Renderer *tgr, Editor *e)
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

    tgr_render_text(tgr, c, vec2i(ecx, -e->cy), vec4fs(0.0f), vec4fs(1.0f));
}

void renderer_draw(Tile_Glyph_Renderer *tgr, Editor *e, Vec2f camera_pos)
{
    glUniform1f(tgr->time, (float) SDL_GetTicks() / 1000.0f);
    glUniform2f(tgr->cam, camera_pos.x, -camera_pos.y);

    tgr_clear(tgr);
    for (int cy = 0; cy < (int) e->lines.length; cy++) {
        const Line *line = list_get(&e->lines, cy);
        tgr_render_text(tgr, line->s, vec2i(0, -cy), vec4fs(1.0f), vec4fs(0.0f));
    }
    gl_render_cursor(tgr, e);
    tgr_sync(tgr);

    tgr_draw(tgr);
}

#else

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

void renderer_init(FreeType_Glyph_Renderer *ftgr, FT_Face face)
{
    ftgr_init(ftgr, face, "shaders/freetype_glyph.vert", "shaders/freetype_glyph.frag");
    glUniform2f(ftgr->resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void renderer_draw(FreeType_Glyph_Renderer *ftgr, Cursor_Renderer *cr, Editor *e, 
                   Screen *scr)
{
    /*  Render Glyphs */

    ftgr_use(ftgr);

    glUniform1f(ftgr->time, (float) SDL_GetTicks() / 1000.0f);
    glUniform2f(ftgr->camera, scr->cam.pos.x, -scr->cam.pos.y);

    ftgr_clear(ftgr);
    
    Vec2f pos = {0};
    for (size_t cy = 0; cy < e->lines.length; cy++) {
        pos.x = 0.0f;
        pos.y = - (float) cy * (FONT_SIZE);
        const char *s = editor_get_line_at(e, cy);
        ftgr_render_string(ftgr, s, pos, vec4fs(1.0f), vec4fs(0.0f));
    }

    ftgr_sync(ftgr);
    ftgr_draw(ftgr);

    /*  Render Cursor */

    cr_use(cr);

    glUniform1f(cr->time, ((float) SDL_GetTicks() / 1000.0f));
    glUniform2f(cr->camera, scr->cam.pos.x, -scr->cam.pos.y);
    glUniform2f(cr->pos, scr->cur.render_pos.x, -scr->cur.render_pos.y);

    cr_draw();
}

void cursor_move(Screen *scr, Cursor_Renderer *cr, size_t cx, size_t cy)
{
    scr->cur.last_cx = cx;
    scr->cur.last_cy = cy;

    cr_use(cr);
    glUniform1f(cr->last_moved, ((float) SDL_GetTicks() / 1000.0f));
}

static FreeType_Glyph_Renderer ftgr = {0};
static Cursor_Renderer cr = {0};

#endif  // TILE_GLYPH_RENDERER

int main(int argc, char *argv[])
{
#ifndef TILE_GLYPH_RENDERER
debug_print
    FT_Face face = FT_init();
#endif // TILE_GLYPH_RENDERER
debug_print

    scc(SDL_Init(SDL_INIT_VIDEO));

    gl_attr();
    SDL_Window *window;
    window = scp(
        SDL_CreateWindow("ged: geimer editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                         SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL)
    );
    scp(SDL_GL_CreateContext(window));
    init_glew();
debug_print

#ifdef TILE_GLYPH_RENDERER
    float camera_scale = FONT_SCALE;
    renderer_init(&tgr);
#else
debug_print
    renderer_init(&ftgr, face);
    cr = cr_init(CURSOR_VERT_FILENAME, CURSOR_FRAG_FILENAME);
    glUniform1f(cr.height, FONT_SIZE);
debug_print
#endif // TILE_GLYPH_RENDERER

    char *filename = NULL;
    if (argc > 1) {
        filename = argv[1];
    }

    Editor e = editor_init(filename);
    Screen scr = {0};

debug_print
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
                                vec2f_sub(scr.cam.pos, vec2f(w / 2.0f, h / 2.0f))
                            );

#ifdef TILE_GLYPH_RENDERER
                            int truex = (int) floorf(cursor_coord.x / (FONT_CHAR_WIDTH * camera_scale)); 
                            int truey = (int) floorf(cursor_coord.y / (FONT_CHAR_HEIGHT * camera_scale)) + 1;
                            if (truex < 0) truex = 0;
                            if (truey < 0) truey = 0;
                            editor_click(&e, truex, truey);
#else
                            cursor_coord.y += FONT_SIZE;
                            int truey = (int) floorf(cursor_coord.y / FONT_SIZE);
                            if (truey < 0) truey = 0;
                            if ((size_t) truey >= e.lines.length) truey = e.lines.length - 1;


                            if (cursor_coord.x < 0) cursor_coord.x = 0;
                            const char *s = editor_get_line_at(&e, truey);
                            size_t truex = (int) ftgr_get_glyph_index_near(&ftgr, s, cursor_coord.x);

                            editor_click(&e, truex, truey);
                            cursor_move(&scr, &cr, e.cx, e.cy);
#endif // TILE_GLYPH_RENDERER
                        } break;
                    }
                } break;
            }
        }

        {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            glViewport(0, 0, w, h);
#ifdef TILE_GLYPH_RENDERER
            glUniform2f(tgr.resolution, (float) w, (float) h);
#else
            ftgr_use(&ftgr);
            glUniform2f(ftgr.resolution, (float) w, (float) h);

            cr_use(&cr);
            glUniform2f(cr.resolution, (float) w, (float) h);
#endif // TILE_GLYPH_RENDERER
        }

#ifndef TILE_GLYPH_RENDERER
        
        // Update Cursor
        if (e.cy != scr.cur.last_cy) {
            const char *s = editor_get_line_at(&e, scr.cur.last_cy);
            const float last_width = ftgr_get_s_width_n(&ftgr, s, e.cx);
            e.cx = ftgr_get_glyph_index_near(&ftgr, editor_get_line(&e), last_width);
            cursor_move(&scr, &cr, e.cx, e.cy);
        }
        if (e.cx != scr.cur.last_cx) {
            cursor_move(&scr, &cr, e.cx, e.cy);
        }

        // Update cursor position on the screen
        scr.cur.actual_pos.y = e.cy * FONT_SIZE;
        scr.cur.actual_pos.x = ftgr_get_s_width_n(&ftgr, editor_get_line(&e), e.cx);

        scr.cur.vel = vec2f_mul(
            vec2f_sub(scr.cur.render_pos, scr.cur.actual_pos),
            vec2fs(CURSOR_VELOCITY)
        );
        scr.cur.render_pos = vec2f_sub(
            scr.cur.render_pos,
            vec2f_mul(scr.cur.vel, vec2fs(DELTA_TIME))
        );

#endif // TILE_GLYPH_RENDERER

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

#ifdef TILE_GLYPH_RENDERER
        scr.cur = vec2f(e.cx * FONT_CHAR_WIDTH * camera_scale,
                           e.cy * FONT_CHAR_HEIGHT * camera_scale);

        renderer_draw(&tgr, &e, scr.cam.pos);
#else
        renderer_draw(&ftgr, &cr, &e, &scr);
#endif // TILE_GLYPH_RENDERER

        const Uint32 duration = (SDL_GetTicks() - start);
        if (duration < DELTA_TIME_MS) {
            SDL_Delay(DELTA_TIME_MS - duration);
        }

        SDL_GL_SwapWindow(window);
    }

    return 0;
}


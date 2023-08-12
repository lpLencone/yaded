#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "la.h"
#include "editor.h"

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

Vec2f camera_project_point(SDL_Window *window, Vec2f point);

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

void render_cursor(SDL_Window *window, SDL_Renderer *renderer, const Font *font)
{
    size_t ecx_pos = (e.cx > get_line_length(&e)) ? get_line_length(&e) : e.cx;

    Vec2f cursor_render_pos = camera_project_point(
        window, 
        vec2f(ecx_pos * FONT_CHAR_WIDTH  * FONT_SCALE, 
              e.cy    * FONT_CHAR_HEIGHT * FONT_SCALE)
    );

    SDL_Rect rect = {
        .x = (int) floorf(cursor_render_pos.x),
        .y = (int) floorf(cursor_render_pos.y),
        .w = FONT_CHAR_WIDTH * FONT_SCALE,
        .h = FONT_CHAR_HEIGHT * FONT_SCALE,
    };

    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFFFFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));

    set_texture_color(font->spritesheet, 0xFF000000);

    Line *line = list_get(&e.lines, e.cy);
    if (e.cy != e.lines.length && e.cx < line->size) {
        render_char(renderer, font, line->s[e.cx], cursor_render_pos, FONT_SCALE);
    }
}

const int move_keymap[] = {
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_UP,
    SDLK_DOWN,
    SDLK_HOME,
    SDLK_END,
    SDLK_PAGEUP,
    SDLK_PAGEDOWN,
};

const size_t move_keymap_size = sizeof(move_keymap) / sizeof(move_keymap[0]);

EditorMoveKeys find_move_key(int code) {
    for (size_t i = 0; i < move_keymap_size; i++) {
        if (move_keymap[i] == code) return i;
    }
    assert(0);
}

const int edit_keymap[] = {
    SDLK_BACKSPACE,
    SDLK_DELETE,
    SDLK_RETURN,
    SDLK_TAB,
    SDLK_F3,
};

const size_t edit_keymap_size = sizeof(edit_keymap) / sizeof(edit_keymap[0]);

EditorEditKeys find_edit_key(int code) {
    for (size_t i = 0; i < edit_keymap_size; i++) {
        if (edit_keymap[i] == code) return i;
    }
    assert(0);
}

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

int main(int argc, char *argv[])
{
    char *filename = NULL;
    if (argc > 1) {
        filename = argv[1];
    }

    scc(SDL_Init(SDL_INIT_VIDEO));
    scc(TTF_Init());

    SDL_Window *window = scp(
        SDL_CreateWindow("Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                         SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE)
    );

    SDL_Renderer *renderer = scp(
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)
    );

    Font font = font_load_from_file(renderer, "charmap-oldschool_white.png");

    e = editor_init(filename);

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
                        case SDLK_F3: {
                            editor_edit(&e, find_edit_key(event.key.keysym.sym));
                        } break;

                        case SDLK_LEFT:
                        case SDLK_RIGHT: 
                        case SDLK_UP:
                        case SDLK_DOWN:
                        case SDLK_HOME: 
                        case SDLK_END:
                        case SDLK_PAGEUP:
                        case SDLK_PAGEDOWN: {
                            editor_move(&e, find_move_key(event.key.keysym.sym));
                        }
                    }
                } break;


                case SDL_TEXTINPUT: {
                    editor_insert_text(&e, event.text.text);
                } break;
            }
        }

        size_t ecx_pos = (e.cx > get_line_length(&e)) ? get_line_length(&e) : e.cx;

        const Vec2f cursor_pos = vec2f(ecx_pos * FONT_CHAR_WIDTH * FONT_SCALE, 
                                        e.cy * FONT_CHAR_HEIGHT * FONT_SCALE);

        camera_vel = vec2f_mul(vec2f_sub(cursor_pos, camera_pos), vec2fs(2.0f));
        camera_pos = vec2f_add(camera_pos, vec2f_mul(camera_vel, vec2fs(DELTA_TIME)));

        /*  Clear the screen */
        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));
        for (size_t cy = 0; cy < e.lines.length; cy++) {
            Line *line = list_get(&e.lines, cy);
            
            Vec2f line_pos = camera_project_point(
                window, 
                vec2f(0.0f, cy * FONT_SCALE * FONT_CHAR_HEIGHT)
            );

            render_text_sized(renderer, &font, line->s, line->size, line_pos, 0xFFFFFFFF, FONT_SCALE);
        }
        render_cursor(window, renderer, &font);

        SDL_RenderPresent(renderer);

        const Uint32 duration = (SDL_GetTicks() - start);
        if (duration < DELTA_TIME_MS) {
            SDL_Delay(DELTA_TIME_MS - duration);
        }
    }

    SDL_Quit();

    return 0;
}


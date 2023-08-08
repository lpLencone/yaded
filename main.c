#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "la.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FONT_WIDTH          128
#define FONT_HEIGHT         64
#define FONT_COLS           18
#define FONT_ROWS           7
#define FONT_CHAR_WIDTH     (int) (FONT_WIDTH  / FONT_COLS)
#define FONT_CHAR_HEIGHT    (int) (FONT_HEIGHT / FONT_ROWS)
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
        SDL_CreateRGBSurfaceFrom((void *)pixels, width, height, depth, pitch, 
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

void render_char(SDL_Renderer *renderer, Font *font, char c, Vec2f pos, float scale)
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

void render_text_sized(SDL_Renderer *renderer, Font *font, const char *text, 
                       size_t text_size, Vec2f pos, Uint32 color, float scale)
{
    scc(SDL_SetTextureColorMod(
        font->spritesheet, 
        (color >> (0 * 8)) & 0xff, 
        (color >> (1 * 8)) & 0xff,
        (color >> (2 * 8)) & 0xff
    ));

    scc(SDL_SetTextureAlphaMod(font->spritesheet, (color >> (3 * 8)) & 0xff));

    Vec2f pen = pos;
    for (size_t i = 0; i < text_size; i++) {
        render_char(renderer, font, text[i], pen, scale);
        pen.x += scale * FONT_CHAR_WIDTH;
    }
}

void render_text(SDL_Renderer *renderer, Font *font, const char *text, 
                 Vec2f pos, Uint32 color, float scale)
{
    render_text_sized(renderer, font, text, strlen(text), pos, color, scale);
}

#define BUFFER_CAPACITY 1024
char buffer[BUFFER_CAPACITY];
size_t buffer_cursor = 0;
size_t buffer_size = 0;

#define UNHEX(color) \
    (color) >> (0 * 8) & 0xff, \
    (color) >> (0 * 1) & 0xff, \
    (color) >> (0 * 2) & 0xff, \
    (color) >> (0 * 3) & 0xff \

void render_cursor(SDL_Renderer *renderer, Uint32 color)
{
    SDL_Rect rect = {
        .x = (int) floorf(buffer_cursor * FONT_CHAR_WIDTH * FONT_SCALE),
        .y = 0,
        .w = FONT_CHAR_WIDTH * FONT_SCALE,
        .h = FONT_CHAR_HEIGHT * FONT_SCALE,
    };

    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFFFFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));
}

int main(void)
{
    scc(SDL_Init(SDL_INIT_VIDEO));
    scc(TTF_Init());

    SDL_Window *window = scp(
        SDL_CreateWindow("Text Editor", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE)
    );

    SDL_Renderer *renderer = scp(
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)
    );

    Font font = font_load_from_file(renderer, "charmap-oldschool_white.png");

    bool quit = false;
    while (!quit) {
        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = true;
                } break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.sym) {
                        case SDLK_BACKSPACE: {
                            if (buffer_size > 0) {
                                buffer_size -= 1;
                                buffer_cursor = buffer_size;
                            }
                        }
                    }
                } break;


                case SDL_TEXTINPUT: {
                    size_t text_size = strlen(event.text.text);
                    const size_t free_space = BUFFER_CAPACITY - buffer_size;
                    if (text_size > free_space) {
                        text_size = free_space;
                    }
                    memcpy(buffer + buffer_size, event.text.text, text_size);
                    buffer_size += text_size;
                    buffer_cursor = buffer_size;
                } break;
            }
        }

        /*  Clear the screen */
        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));

        render_text_sized(renderer, &font, buffer, buffer_size, vec2fs(0.0f), 0xFFFFFFFF, FONT_SCALE);
        render_cursor(renderer, 0xFFFFFFFF);

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}
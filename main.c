#define _DEFAULT_SOURCE

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "la.h"
#include "editor.h"
#include "gl_extra.h"
#include "lexer.h"

#include "freetype_renderer.h"
#include "simple_renderer.h"

#define FONT_SIZE                   128

// #define FONT_FILENAME               "fonts/TSCu_Comic.ttf"
#define FONT_FILENAME               "fonts/iosevka-custom-regular.ttf"

#define CUR_INIT_WIDTH              5.0f
#define CUR_MOVE_VEL                20.0f
#define CUR_BLINK_VEL               6.0f

#define CAM_MOVE_VEL                2.5f
#define CAM_SCALE_VEL               3.0f
#define CAM_FINAL_SCALE             0.25f
#define CAM_INIT_SCALE              0.75f

#define SCREEN_WIDTH                800
#define SCREEN_HEIGHT               600
#define FPS                         60
#define DELTA_TIME                  (1.0f / FPS)
#define DELTA_TIME_MS               (1000 / FPS)

#define SDL_CTRL    ((event.key.keysym.mod & KMOD_CTRL)  != 0)
#define SDL_SHIFT   ((event.key.keysym.mod & KMOD_SHIFT) != 0)

typedef struct {
    struct {
        Vec2f pos;
        Vec2f vel;
        float scale;
        float scale_vel;
    } cam;
    struct {
        Vec2f vel;
        Vec2f actual_pos;
        Vec2f render_pos;
        float actual_width;
        float render_width;

        Vec2f selection_pos;
        float selection_width;

        float height;
        size_t last_cx;
        size_t last_cy;
        Uint32 last_moved; // in milisec
    } cur;
    struct {
        SDL_Keysym last_key;
        size_t ctrl_a_pressed;
    } state;
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

void MessageCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void *userParam)
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

FT_Face FT_init(void)
{
#define ft_check_error                                          \
    if (error) {                                                \
        fprintf(stderr, "ERROR: %s", FT_Error_String(error));   \
        exit(1);                                                \
    }                                                           

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

static Vec4f hex_to_vec4f(uint32_t color)
{
    // 0xRRGGBBAA
    uint32_t r = (color >> (3 * 8)) & 0xFF;
    uint32_t g = (color >> (2 * 8)) & 0xFF;
    uint32_t b = (color >> (1 * 8)) & 0xFF;
    uint32_t a = (color >> (0 * 8)) & 0xFF;
    return vec4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

static const char *keywords[] = {
    "float", "double", "int", "short", "long", "void", "char", "const",
    "unsigned", "size_t", "ssize_t", "typedef", "struct", "return", "if", 
    "for", "while", "do",
    
    NULL
};

void renderers_init(Simple_Renderer *sr, FreeType_Renderer *ftr, FT_Face face)
{
    ftr_init(ftr, face);
    sr_init(sr);

    for (size_t shader_i = 0; shader_i < SHADER_COUNT; shader_i++) {
        sr_set_shader(sr, shader_i);
        glUniform2f(sr->resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void renderer_draw(SDL_Window *window, Simple_Renderer *sr, FreeType_Renderer *ftr,
                   Editor *e, Screen *scr)
{
    // Set background color
    Vec4f bg = hex_to_vec4f(0x181818FF);

    glClearColor(bg.x, bg.y, bg.z, bg.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: set viewport only on window change
    int scr_width, scr_height;
    SDL_GetWindowSize(window, &scr_width, &scr_height);
    glViewport(0, 0, scr_width, scr_height);

    // Render Glyphs
    sr_set_shader(sr, SHADER_TEXT);

    glUniform1f(sr->time, (float) SDL_GetTicks() / 1000.0f);
    glUniform2f(sr->camera, scr->cam.pos.x, -scr->cam.pos.y);
    glUniform2f(sr->scale, scr->cam.scale, scr->cam.scale);
    glUniform2f(sr->resolution, scr_width, scr_height);
    float max_line_width = 0;

    const char *data = editor_get_data(e);

    Vec2f pos = {0};
    float line_width = 0;
    Lexer l = lexer_init(data, strlen(data), keywords);
    
    size_t last_i = 0;
    Token token = {0};

    while ((token = lexer_next(&l)).kind != TOKEN_END) {
        Vec4f color;
        switch (token.kind) {
            case TOKEN_BLOCK_COMMENT:
            case TOKEN_INLINE_COMMENT: 
                                color = hex_to_vec4f(0x905425FF); break;
            case TOKEN_CHRLIT:
            case TOKEN_STRLIT:  color = hex_to_vec4f(0xAA8A60FF); break;
            case TOKEN_HASH:    color = hex_to_vec4f(0x9A9AA0FF); break;
            case TOKEN_SYMBOL:  color = hex_to_vec4f(0xCFCFCFFF); break;
            case TOKEN_NUMLIT:  color = hex_to_vec4f(0x90EE90FF); break;
            case TOKEN_KEYWORD: color = hex_to_vec4f(0xDFDF45FF); break;
            case TOKEN_INVALID: color = hex_to_vec4f(0xAA4554FF); break;
            default: color = hex_to_vec4f(0xCFCFCFFF); break;
        }

        for (size_t i = 0; i < token.len; i++) {
            if (data[last_i + i] == '\n') {
                line_width = pos.x;

                pos.y -= (float) FONT_SIZE;
                pos.x = 0;
            } else {
                pos = ftr_render_s_n(ftr, sr, data + last_i + i, 1, pos, color);
                line_width = pos.x;
            }
        }
        last_i += token.len;

        if (line_width > max_line_width) max_line_width = line_width;
    }

    // Render Cursor
    sr_set_shader(sr, SHADER_COLOR);

    glUniform1f(sr->time, (float) SDL_GetTicks() / 1000.0f);
    glUniform2f(sr->camera, scr->cam.pos.x, -scr->cam.pos.y);
    glUniform2f(sr->scale, scr->cam.scale, scr->cam.scale);
    glUniform2f(sr->resolution, scr_width, scr_height);

    switch (e->mode) {
        case EM_EDITING:
        case EM_SELECTION: {
            scr->cur.actual_width = CUR_INIT_WIDTH;
            float CURSOR_BLINK_THRESHOLD = 0.5 * 3.14 / CUR_BLINK_VEL;
            float t = (float) (SDL_GetTicks() - scr->cur.last_moved) / 1000.0f;
            bool threshold = t > CURSOR_BLINK_THRESHOLD;
            float blinking_state = (1 + sin(CUR_BLINK_VEL * t)) / 2;
            Vec4f color = (threshold) ? vec4fs(blinking_state) : vec4fs(1.0f);

            sr_solid_rect(
                sr, vec2f(scr->cur.render_pos.x, -scr->cur.render_pos.y),
                    vec2f(scr->cur.actual_width, scr->cur.height),
                    color);

            // Render selection background
            if (e->mode == EM_SELECTION) {
                Vec2ui csbegin = e->c;  // Cursor select begin
                Vec2ui csend = e->cs;   // Cursor select end
                
                if (vec2ui_cmp_yx(e->c, e->cs) > 0) {
                    Vec2ui temp = csbegin;
                    csbegin = csend;
                    csend = temp;
                }
                
                for (uint32_t cy = csbegin.y; cy <= csend.y; cy++) {
                    scr->cur.selection_pos.x = 0;

                    const char *s = editor_get_line_at(e, cy);
                    size_t slen = strlen(s);

                    size_t line_cx_begin = 0;
                    size_t line_cx_end = slen;
                    if (cy == csbegin.y) {
                        line_cx_begin = (csbegin.x < slen) ? csbegin.x : slen;
                    }
                    if (cy == csend.y) {
                        line_cx_end = (csend.x < slen) ? csend.x : slen;
                    } else {
                        line_cx_end++; // Select one more character, as though it was a `\n`
                    }

                    scr->cur.selection_width = 
                        ftr_get_s_width_n_pad(ftr, &s[line_cx_begin], line_cx_end - line_cx_begin, ' ');

                    scr->cur.selection_pos.x = ftr_get_s_width_n(ftr, s, line_cx_begin);
                    sr_solid_rect(
                        sr, vec2f(scr->cur.selection_pos.x, - (int) cy * FONT_SIZE),
                            vec2f(scr->cur.selection_width, scr->cur.height),
                            vec4fs(0.5f)
                    );
                }
            }
        } break;

        case EM_BROWSING: {
            const char *s = editor_get_line(e);
            const size_t slen = strlen(s);
            scr->cur.actual_width = ftr_get_s_width_n(ftr, s, slen);
            scr->cur.render_width += 
                (scr->cur.actual_width - scr->cur.render_width) * 10 * DELTA_TIME;

            sr_solid_rect(
                sr, vec2f(scr->cur.render_pos.x - scr->cur.render_width / 2, -scr->cur.render_pos.y),
                    vec2f(scr->cur.render_width, scr->cur.height),
                    vec4fs(0.5f)
            );
        } break;

        default:
            assert(0);
    }

    sr_flush(sr);



    // Update camera

    const float cam_x_start_limit = (max_line_width < 0.4f * scr_width / scr->cam.scale)
        ? max_line_width
        : 0.4f * scr_width / scr->cam.scale; // 1/10 of scr_width from left edge (1/2 - 1/10 = 2/5)

    const float cam_x_end_limit = max_line_width - 0.3f * scr_width / scr->cam.scale; // 1/5 from right edge (1/5 - 1/2 = -3/10)

    Vec2f cam_focus_pos = scr->cur.render_pos;

    if (cam_focus_pos.x > cam_x_end_limit) {
        cam_focus_pos.x = cam_x_end_limit;
    }
    if (cam_focus_pos.x < cam_x_start_limit) {
        cam_focus_pos.x = cam_x_start_limit;
    }

    scr->cam.vel = vec2f_mul(
        vec2f_sub(cam_focus_pos, scr->cam.pos), 
        vec2fs(CAM_MOVE_VEL)
    );
    scr->cam.pos = vec2f_add(
        scr->cam.pos, 
        vec2f_mul(scr->cam.vel, vec2fs(DELTA_TIME))
    );

    float target_scale = 0.5f * scr_width / max_line_width; 

    if (target_scale > CAM_INIT_SCALE) {
        target_scale = CAM_INIT_SCALE;
    } else if (target_scale < CAM_FINAL_SCALE) {
        target_scale = CAM_FINAL_SCALE;
    }

    scr->cam.scale_vel = CAM_SCALE_VEL * (target_scale - scr->cam.scale);
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
        SDL_CreateWindow(
            "ged: geimer editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL)
    );
    scp(SDL_GL_CreateContext(window));
    init_glew();
    renderers_init(&sr, &ftr, face);

    assert(argc > 1);
    Editor e = editor_init(argv[1]);
    
    Screen scr = {0};
    scr.cam.scale = CAM_INIT_SCALE;
    scr.cur.actual_width = CUR_INIT_WIDTH;
    scr.cur.render_width = CUR_INIT_WIDTH;
    scr.cur.height = FONT_SIZE;

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
                        case SDLK_w: {
                            if (SDL_CTRL) {
                                quit = true;
                            }
                        } break;

                        case SDLK_UP: {
                            if (SDL_CTRL && SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_PREV_EMPTY_LINE);
                            } else if (SDL_CTRL) {
                                editor_process_key(&e, EK_PREV_EMPTY_LINE);
                            } else if (SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_UP);
                            } else {
                                editor_process_key(&e, EK_UP);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_DOWN: {
                            if (SDL_CTRL && SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_NEXT_EMPTY_LINE);
                            } else if (SDL_CTRL) {
                                editor_process_key(&e, EK_NEXT_EMPTY_LINE);
                            } else if (SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_DOWN);
                            } else {
                                editor_process_key(&e, EK_DOWN);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_RIGHT: {
                            if (SDL_CTRL && SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_RIGHTW);
                            } else if (SDL_CTRL) {
                                editor_process_key(&e, EK_RIGHTW);
                            } else if (SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_RIGHT);
                            } else {
                                editor_process_key(&e, EK_RIGHT);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_LEFT: {
                            if (SDL_CTRL && SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_LEFTW);
                            } else if (SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_LEFT);
                            } else if (SDL_CTRL) {
                                editor_process_key(&e, EK_LEFTW);
                            } else {
                                editor_process_key(&e, EK_LEFT);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_HOME: {
                            if (SDL_CTRL && SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_HOME);
                            } else if (SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_LINE_HOME);
                            } else if (SDL_CTRL) {
                                editor_process_key(&e, EK_HOME);
                            } else {
                                editor_process_key(&e, EK_LINE_HOME);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_END: {
                            if (SDL_CTRL && SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_END);
                            } else if (SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_LINE_END);
                            } else if (SDL_CTRL) {
                                editor_process_key(&e, EK_END);
                            } else {
                                editor_process_key(&e, EK_LINE_END);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_BACKSPACE: {
                            editor_process_key(&e, EK_BACKSPACE);
                            update_last_moved(&scr);
                        } break;

                        case SDLK_DELETE: {
                            editor_process_key(&e, EK_DELETE);
                            update_last_moved(&scr);
                        } break;

                        case SDLK_TAB: {
                            editor_process_key(&e, EK_TAB);
                            update_last_moved(&scr);
                        } break;

                        case SDLK_RETURN: {
                            if (SDL_CTRL) {
                                editor_process_key(&e, EK_LINE_BELOW);
                            } else if (SDL_SHIFT) {
                                editor_process_key(&e, EK_LINE_ABOVE);
                            } else {
                                editor_process_key(&e, EK_RETURN);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_s: {
                            if (SDL_CTRL) {
                                editor_process_key(&e, EK_SAVE);
                            }
                        } break;

                        case SDLK_o: {
                            if (SDL_CTRL) {
                                editor_open(&e, "..");
                            }
                        } break;

                        case SDLK_a: {
                            if (SDL_CTRL) {
                                if ((scr.state.last_key.sym != SDLK_a) ||
                                    (scr.state.last_key.mod & KMOD_CTRL) == 0 ||
                                    (scr.state.last_key.mod & KMOD_SHIFT) != 0)
                                {
                                    scr.state.ctrl_a_pressed = 0;
                                }

                                const char *s = editor_get_line(&e);
                                if ((isspace(s[e.c.x]) || s[e.c.x] == '\0') &&
                                    (scr.state.ctrl_a_pressed == 0)) 
                                {
                                    scr.state.ctrl_a_pressed++;
                                    
                                    if (strlen(s) == 0) {
                                        scr.state.ctrl_a_pressed++;
                                    }
                                }

                                switch (scr.state.ctrl_a_pressed) {
                                    case 0: {
                                        editor_process_key(&e, EK_SELECT_WORD);
                                    } break;
                                    case 1: {
                                        editor_process_key(&e, EK_SELECT_LINE);
                                    } break;
                                    default: {
                                        editor_process_key(&e, EK_SELECT_ALL);
                                    }
                                }
                                scr.state.ctrl_a_pressed++;
                                update_last_moved(&scr);
                            }
                        } break;

                        case SDLK_b: {
                             if (SDL_CTRL) {
                                if (SDL_SHIFT) {
                                    editor_process_key(&e, EK_SELECT_NEXT_BLOCK);
                                } else {
                                    editor_process_key(&e, EK_SELECT_OUTER_BLOCK);
                                }
                             }
                        } break;

                        case SDLK_c: {
                            if (SDL_CTRL && e.mode == EM_SELECTION) {
                                SDL_SetClipboardText(editor_retrieve_selection(&e));
                                editor_process_key(&e, EK_COPY);
                            }
                        } break;

                        case SDLK_v: {
                            if (e.clipboard == NULL || (SDL_HasClipboardText() && 
                                                        strcmp(e.clipboard, SDL_GetClipboardText()) != 0))
                            {
                                e.clipboard = SDL_GetClipboardText();
                            }
                            if (SDL_CTRL && e.clipboard != NULL) {
                                editor_process_key(&e, EK_PASTE);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_x: {
                            if (SDL_CTRL && e.mode == EM_SELECTION) {
                                SDL_SetClipboardText(editor_retrieve_selection(&e));
                                editor_process_key(&e, EK_CUT);
                            }
                        } break;

                        case SDLK_F5: {
                            sr_reload_shaders(&sr);
                        } break;
                    }
                    scr.state.last_key = event.key.keysym;
                } break;
                static_assert(EK_COUNT == 44, "The number of editor keys has changed");

                case SDL_TEXTINPUT: {
                    editor_write(&e, event.text.text);
                    update_last_moved(&scr);
                } break;
            }
        }

        // Update Cursor
        if (e.c.y != scr.cur.last_cy) {
            const char *s = (scr.cur.last_cy < e.lines.length) 
                ? editor_get_line_at(&e, scr.cur.last_cy) 
                : NULL;
                
            const float last_width = (s != NULL) ? ftr_get_s_width_n(&ftr, s, e.c.x) : 0;
            size_t ecx = ftr_get_glyph_index_near(&ftr, editor_get_line(&e), last_width);
            cursor_move(&scr, ecx, e.c.y);
        }
        if (e.c.x != scr.cur.last_cx) {
            cursor_move(&scr, e.c.x, e.c.y);
        }

        // Update cursor position on the screen
        scr.cur.actual_pos.y = e.c.y * FONT_SIZE;
        size_t n = editor_get_line_size(&e);
        scr.cur.actual_pos.x = (e.mode != EM_BROWSING) 
            ? ftr_get_s_width_n(&ftr, editor_get_line(&e), (e.c.x > n) ? n : e.c.x)
            : ftr_get_s_width_n(&ftr, editor_get_line(&e), n) / 2;

        scr.cur.vel = vec2f_mul(
            vec2f_sub(scr.cur.render_pos, scr.cur.actual_pos),
            vec2fs(CUR_MOVE_VEL)
        );
        scr.cur.render_pos = vec2f_sub(
            scr.cur.render_pos,
            vec2f_mul(scr.cur.vel, vec2fs(DELTA_TIME))
        );

        renderer_draw(window, &sr, &ftr, &e, &scr);

        const Uint32 duration = (SDL_GetTicks() - start);
        if (duration < DELTA_TIME_MS) {
            SDL_Delay(DELTA_TIME_MS - duration);
        }

        SDL_GL_SwapWindow(window);
    }

    editor_clear(&e);

    return 0;
}


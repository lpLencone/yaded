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

#include "freetype_renderer.h"
#include "simple_renderer.h"

#define FONT_SIZE                   64
    
#define FONT_FILENAME               "fonts/VictorMono-Regular.ttf"
// #define FONT_FILENAME               "fonts/TSCu_Comic.ttf"

#define CUR_INIT_WIDTH              5.0f
#define CUR_MOVE_VEL                20.0f
#define CUR_BLINK_VEL               6.0f
#define CAM_FINAL_SCALE             0.5f
#define CAM_INIT_SCALE              1.8f

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
    0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    SDLK_PAGEUP,
    SDLK_PAGEDOWN,
    SDLK_BACKSPACE,
    SDLK_DELETE,
    SDLK_TAB,
};

const size_t keymap_size = sizeof(keymap) / sizeof(keymap[0]);

EditorKey find_move_key(int code) {
    for (EditorKey ek = EK_LEFT; ek <= EK_PAGEDOWN; ek++) {
        if (keymap[ek] == code) return ek;
    }
    assert(0);
}

EditorKey find_edit_key(int code) {
    for (EditorKey ek = EK_BACKSPACE; ek <= EK_TAB; ek++) {
        if (keymap[ek] == code) return ek;
    }
    assert(0);
}

static_assert(EK_COUNT == 36, "The number of editor keys has changed");

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

void renderers_init(Simple_Renderer *sr, FreeType_Renderer *ftr, FT_Face face)
{
    ftr_init(ftr, face);
    sr_init(sr);

    for (size_t shader_i = 0; shader_i < SHADER_COUNT; shader_i++) {
        sr_set_shader(sr, shader_i);
        glUniform2f(sr->resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

typedef struct {
    size_t i;
    size_t len;
} Key_Pair;

int key_compare(const void *key1, const void *key2)
{
    return ((Key_Pair *) key1)->i - ((Key_Pair *) key2)->i;
}

void renderer_draw(SDL_Window *window, Simple_Renderer *sr, FreeType_Renderer *ftr,
                   Editor *e, Screen *scr)
{
    // TODO: set viewport only on window change
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);

    // Render Glyphs
    sr_set_shader(sr, SHADER_TEXT);

    glUniform1f(sr->time, (float) SDL_GetTicks() / 1000.0f);
    glUniform2f(sr->camera, scr->cam.pos.x, -scr->cam.pos.y);
    glUniform2f(sr->scale, scr->cam.scale, scr->cam.scale);
    glUniform2f(sr->resolution, w, h);
    float max_line_width = 0;
    const char *keys[] = {"for ", "if ", "void ", "int ", "#define ", "#include ", "while ", "do ", ";", "const ", "float ", "size_t ", "Vec2f ", "Vec4f "};
    
    Vec2f pos = {0};
    List keysl = list_init(NULL, key_compare);
    for (size_t cy = 0; cy < e->lines.length; cy++) {
        pos.x = 0.0f;
        pos.y = - (float) cy * (FONT_SIZE);

        const char *s = editor_get_line_at(e, cy);
        const char *needle = NULL;
        
        for (size_t key_i = 0; key_i < sizeof(keys) / sizeof(keys[0]); key_i++) {
            needle = strstr(s, keys[key_i]);
            if (needle != NULL) {
                Key_Pair kpair = {
                    .i = needle - s,
                    .len = strlen(keys[key_i])
                };
                list_append(&keysl, &kpair, sizeof(kpair));
            }
        }
        list_quicksort(&keysl);
        
        Node *cursor = keysl.head;
        size_t last_i = 0;
        while (cursor != NULL) {
            Key_Pair *kpair = cursor->data;
            pos = ftr_render_s_n(ftr, sr, s + last_i, kpair->i - last_i, pos, vec4fs(0.9f));
            pos = ftr_render_s_n(ftr, sr, s + kpair->i, kpair->len, pos, vec4f(0.5f, 1.0f, 0.3f, 1.0f));
            last_i = kpair->i + kpair->len;
            cursor = cursor->next;
        }
        pos = ftr_render_s(ftr, sr, s + last_i, pos, vec4fs(0.9f));

        float line_width = pos.x / 0.5f;
        if (line_width > max_line_width) max_line_width = line_width;

        list_clear(&keysl);
    }
  
    // Render Cursor
    sr_set_shader(sr, SHADER_COLOR);

    glUniform1f(sr->time, (float) SDL_GetTicks() / 1000.0f);
    glUniform2f(sr->camera, scr->cam.pos.x, -scr->cam.pos.y);
    glUniform2f(sr->scale, scr->cam.scale, scr->cam.scale);
    glUniform2f(sr->resolution, w, h);

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

    // Update camera scale

    float target_scale = SCREEN_WIDTH / max_line_width; 

    if (target_scale > CAM_INIT_SCALE) {
        target_scale = CAM_INIT_SCALE;
    } else if (target_scale < CAM_FINAL_SCALE) {
        target_scale = CAM_FINAL_SCALE;
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

                        case SDLK_PAGEUP:
                        case SDLK_PAGEDOWN: {
                            editor_process_key(&e, find_move_key(event.key.keysym.sym));
                            update_last_moved(&scr);
                        } break;

                        case SDLK_UP: {
                            if (SDL_SHIFT) {
                                editor_process_key(&e, EK_SELECT_UP);
                            } else {
                                editor_process_key(&e, EK_UP);
                            }
                            update_last_moved(&scr);
                        } break;

                        case SDLK_DOWN: {
                            if (SDL_SHIFT) {
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

                        case SDLK_BACKSPACE:
                        case SDLK_DELETE:
                        case SDLK_TAB: {
                            editor_process_key(&e, find_edit_key(event.key.keysym.sym));
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
                                editor_process_key(&e, EK_SELECT_ALL);
                            }
                            update_last_moved(&scr);
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
                        }
                    }
    static_assert(EK_COUNT == 36, "The number of editor keys has changed");
                } break;

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


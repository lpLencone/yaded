#define _DEFAULT_SOURCE

#include "editor.h"
#include "ds/string_builder.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SV_IMPLEMENTATION
#include "sv.h"

#define sv_c_str(c_chunk, sv_chunk)                     \
    c_chunk = malloc(sv_chunk.count + 1);               \
    memcpy(c_chunk, sv_chunk.data, sv_chunk.count);     \
    c_chunk[sv_chunk.count] = '\0';                

// File I/O
static void save_file(const Editor *e);
static void open_file(Editor *e, const char *filename);
static void open_dir(Editor *e, const char *dirname);

// Editor Operations
#define editor_delete_char(e) editor_delete_char_at(e, e->c);
static void editor_browsing(Editor *e, EditorKey key);
static void editor_action(Editor *e, EditorKey key);
static size_t editor_select(Editor *e, EditorKey key, size_t cur);
static size_t editor_selection_delete(Editor *e, size_t begin, size_t end);
static void editor_selection_copy(Editor *e);
#define editor_paste(e) editor_write(e, e->clipboard);

static void editor_search_start(Editor *e);
static Vec2i editor_search_next(Editor *e, Vec2ui pos);
static Vec2i editor_search_prev(Editor *e, Vec2ui pos);

static void update_pathname(Editor *e, const char *path, size_t pathlen);

static void find_scope_end(Editor *e);

Editor editor_init(void)
{
    Editor e = {0};

    da_zero(&e.pathname);
    e.clipboard = NULL;
    e.mode = EM_EDITING;

    char cwdbuf[256];
    getcwd(cwdbuf, sizeof(cwdbuf));
    char *save_ptr;
    char *path;
    path = strtok_r(cwdbuf, "/", &save_ptr);
    while (path != NULL) {
        sb_append_cstr(&e.pathname, "/");
        sb_append_cstr(&e.pathname, path);
        path = strtok_r(save_ptr, "/", &save_ptr);
    }

    e.be = (Basic_Editor) {0};
    editor_open(&e, ".", 1);

    e.match = vec2is(-1);

    return e;
}

void editor_clear(Editor *e)
{
    e->c.x = 0;
    e->c.y = 0;
    e->mode = EM_EDITING;

    e->be.cur = 0;
    e->be.data.size = 0;
}

static_assert(sizeof(Editor) == 280, "Editor structure has changed");

void editor_process_key(Editor *e, EditorKey key)
{
    switch (e->mode) {
        case EM_SELECTION: 
        case EM_EDITING: {
            switch (key) {
                case EK_LEFT:
                case EK_RIGHT:
                case EK_UP:
                case EK_DOWN:
                case EK_LEFTW:
                case EK_RIGHTW:
                case EK_LINE_HOME:
                case EK_LINE_END:
                case EK_HOME:
                case EK_END:
                case EK_PAGEUP:
                case EK_PAGEDOWN: 
                case EK_NEXT_PARAGRAPH: 
                case EK_PREV_PARAGRAPH: {
                    if (e->mode == EM_SELECTION) {
                        editor_selection_stop(e);
                    }

                    if (e->mode == EM_SELECTION) {
                        if ((key == (EK_LEFT || EK_UP) && 
                                vec2ui_cmp_yx(e->c, e->cs) > 0) ||
                            (key == (EK_RIGHT || EK_DOWN) &&
                                vec2ui_cmp_yx(e->c, e->cs) < 0))
                        {
                            e->select_cur = e->be.cur;
                            return;
                        }
                    }

                    e->be.cur = editor_move(e, key, e->be.cur);
                } break;

                case EK_BACKSPACE:
                case EK_DELETE:
                case EK_BACKSPACEW:
                case EK_DELETEW:
                case EK_LINE_BELOW:
                case EK_LINE_ABOVE:
                case EK_REMOVE_LINE:
                case EK_MERGE_LINE:
                case EK_RETURN: // Return effect defaults to BREAK_LINE
                case EK_BREAK_LINE:
                case EK_TAB: {
                    if (e->mode == EM_SELECTION) {
                        e->be.cur = editor_selection_delete(e, e->be.cur, e->select_cur);
                        e->mode = EM_EDITING;
                        if (key == EK_BACKSPACE || key == EK_DELETE ||
                            key == EK_BACKSPACEW || key == EK_DELETEW) 
                        {
                            return;
                        }
                    }
                    e->c = editor_edit(e, key, e->c);
                } break;

                case EK_INDENT:
                case EK_UNINDENT: {
                    e->c = editor_edit(e, key, e->c);
                } break;

                case EK_SAVE:
                case EK_COPY:
                case EK_PASTE: 
                case EK_CUT: 
                case EK_SEARCH_START:{
                    editor_action(e, key);
                } break;

                case EK_SELECT_LEFT:
                case EK_SELECT_RIGHT:
                case EK_SELECT_UP:
                case EK_SELECT_DOWN: 
                case EK_SELECT_LEFTW: 
                case EK_SELECT_RIGHTW:
                case EK_SELECT_LINE_HOME:
                case EK_SELECT_LINE_END:
                case EK_SELECT_WORD:
                case EK_SELECT_LINE:
                case EK_SELECT_OUTER_BLOCK:
                case EK_SELECT_NEXT_BLOCK:
                case EK_SELECT_HOME:
                case EK_SELECT_END:
                case EK_SELECT_NEXT_PARAGRAPH:
                case EK_SELECT_PREV_PARAGRAPH:
                case EK_SELECT_ALL: {
                    if (e->mode != EM_SELECTION) {
                        editor_selection_start(e, e->c);
                    }
                    e->be.cur = editor_select(e, key, e->be.cur);
                } break;

                case EK_ESC: {
                    if (e->mode == EM_SELECTION) {
                        editor_selection_stop(e);
                    }
                    e->mode = EM_EDITING;
                } break;

                default:
                    printf("unreachable: %s: %d\n", __FILE__, __LINE__);
            }
        } break;

        case EM_BROWSING: {
            switch (key) {
                case EK_LEFT:
                case EK_RIGHT:
                case EK_UP:
                case EK_DOWN:
                case EK_PAGEUP:
                case EK_PAGEDOWN: 
                case EK_RETURN:
                case EK_HOME:
                case EK_END: {
                    editor_browsing(e, key);
                } break;

                default: break;
            }
        } break;

        case EM_SEARCHING: {
            switch (key) {
                case EK_ESC: {
                    e->mode = EM_EDITING;
                    if (e->match.x != -1) {
                        size_t searchlen = strlen(e->searchbuf);
                        e->c.x += searchlen;
                        e->cs = e->c;
                        for (size_t i = searchlen; i > 0; i--) {
                            e->select_cur = editor_move(e, EK_LEFT, e->select_cur);
                        }
                        e->mode = EM_SELECTION;
                    }
                } break;

                // case EK_SEARCH_NEXT: {
                //     Vec2ui pos = editor_move(e, EK_RIGHT, e->c);
                //     e->match = editor_search_next(e, pos);
                //     if (e->match.x != -1) {
                //         e->c = vec2ui(e->match.x, e->match.y);
                //     }
                // } break;

                // case EK_SEARCH_PREV: {
                //     Vec2ui pos = {0};
                //     if (e->c.x == 0 && e->c.y == 0) {
                //         pos.y = e->lines.length - 1;
                //         pos.x = strlen(editor_get_line_at(e, pos.y));
                //     } else {
                //         pos = editor_move(e, EK_LEFT, e->c);
                //     }
                //     e->match = editor_search_prev(e, pos);
                //     if (e->match.x != -1) {
                //         e->c = vec2ui(e->match.x, e->match.y);
                //     }
                // } break;
                
                case EK_BACKSPACE: {
                    size_t searchlen = strlen(e->searchbuf);
                    if (searchlen > 0) {
                        e->searchbuf[searchlen - 1] = '\0';
                    }
                    e->match = editor_search_next(e, e->c);
                    if (e->match.x != -1) {
                        e->c = vec2ui(e->match.x, e->match.y);
                    }
                }

                default:
                    break;
            }
        } break;
    }
}

static_assert(EK_COUNT == 52, "The number of editor keys has changed");

size_t editor_write_at(Editor *e, const char *s, size_t at)
{
    if (e->mode == EM_BROWSING) {
        return at;
    }

    if (e->mode == EM_SEARCHING) {
        size_t buf_i = strlen(e->searchbuf);
        assert(buf_i + strlen(s) < 64);
        strcpy(e->searchbuf + buf_i, s);
        e->searchbuf[buf_i + strlen(s)] = '\0';

        e->match = editor_search_next(e, e->c);
        if (e->match.x != -1) {
            // return vec2ui(e->match.x, e->match.y);
        }
        
        return at;
    }

    if (e->mode == EM_SELECTION) {
        // at = editor_selection_delete(e, );
        e->mode = EM_EDITING;
    }

    at = be_insert_sn(&e->be, s, strlen(s));
    return at;
}

void editor_selection_start(Editor *e, Vec2ui pos)
{
    assert(e->mode != EM_SELECTION);
    e->mode = EM_SELECTION;
    e->select_cur = e->be.cur;
}

void editor_selection_stop(Editor *e)
{
    assert(e->mode == EM_SELECTION);
    e->mode = EM_EDITING;
}

size_t editor_get_line_size(const Editor *e)
{
    const Line *line = list_get(&e->lines, e->c.y);
    return (line == NULL) ? 0 : line->size;
}

const char *editor_get_line_at(const Editor *e, size_t at)
{
    const Line *line = list_get(&e->lines, at);
    return (line == NULL) ? NULL : line->s;
}

char *editor_retrieve_selection(const Editor *e)
{
    assert(e->mode == EM_SELECTION);

    Vec2ui begin = e->c;
    Vec2ui end = e->cs;
    if (vec2ui_cmp_yx(e->c, e->cs) > 0) {
        Vec2ui temp = begin;
        begin = end;
        end = temp;
    }

    da_var_zero(selectbuf, char);
    for (int cy = begin.y; cy <= (int) end.y; cy++) {
        const char *s = editor_get_line_at(e, cy);
        size_t slen = strlen(s);

        size_t line_cx_begin = 0;
        size_t line_cx_end = slen;
        if (cy == (int) begin.y) {
            line_cx_begin = (begin.x < slen) ? begin.x : slen;
        }
        if (cy == (int) end.y) {
            line_cx_end = (end.x < slen) ? end.x : slen;
        }

        da_append_n(&selectbuf, &s[line_cx_begin], line_cx_end - line_cx_begin);
        if (cy != (int) end.y) {
            da_append(&selectbuf, "\n");
        }
    }
    da_append(&selectbuf, "\0");


    char *s;
    da_get_copy(&selectbuf, s);
    da_end(&selectbuf);

    return s;
}

/* Line operations */

void editor_remove_line_at(Editor *e, size_t at)
{
    assert(at < e->lines.length);
    list_remove(&e->lines, at);
}

void editor_new_line_at(Editor *e, const char *s, size_t at)
{
    Line line = line_init(s);
    list_insert(&e->lines, &line, sizeof(line), at);
}

/* Editor Operations */

size_t editor_move(Editor *e, EditorKey key, size_t cur)
{
#define issymbol(c) (isalnum(c) || c == '_')
    switch (key) {
        case EK_LEFT: {
            if (cur > 0) cur--;
        } break;

        case EK_RIGHT: {
            if (cur < e->be.data.size) cur++;
        } break;

        case EK_UP: {
            cur = be_move_up(&e->be, cur);
        } break;

        case EK_DOWN: {
            cur = be_move_down(&e->be, cur);
        } break;

        case EK_LEFTW: {
            cur = be_move_leftw(&e->be, cur);
        } break;

        case EK_RIGHTW: {
            cur = be_move_rightw(&e->be, cur);
        } break;

        case EK_LINE_HOME: {
            cur = be_move_home(&e->be, cur);
        } break;

        case EK_LINE_END: {
            cur = be_move_end(&e->be, cur);
        } break;

        case EK_HOME: {
            cur = 0;
        } break;

        case EK_END: {
            cur = e->be.data.size;
        } break;
        
        default:
            assert(0);
    }

    return cur;
#undef issymbol
}

Vec2ui editor_edit(Editor *e, EditorKey key, Vec2ui pos)
{
    switch (key) {
        case EK_BACKSPACE: {
            be_backspace(&e->be);
        } break;

        case EK_DELETE: {
            be_delete(&e->be);
        } break;

        // case EK_BACKSPACEW: {
        //     editor_selection_start(e, e->c);
        //     pos = editor_move(e, EK_LEFTW, pos);
        //     e->be.cur = editor_selection_delete(e, e->be.cur, e->select_cur);
        //     editor_selection_stop(e);
        // } break;

        // case EK_DELETEW: {
        //     editor_selection_start(e, e->c);
        //     pos = editor_move(e, EK_RIGHTW, pos);
        //     e->be.cur = editor_selection_delete(e, e->be.cur, e->select_cur);
        //     editor_selection_stop(e);
        // } break;

        case EK_TAB: {
            Line_ line = be_get_line_at_(&e->be, e->be.cur);
            size_t col = e->be.cur - line.home;
            size_t tabstop = 4 - (col % 4);
            for (size_t i = 0; i < tabstop; i++) {
                e->be.cur = editor_write_at(e, " ", e->be.cur);
            }
        } break;

        case EK_INDENT: {
            if (e->mode == EM_SELECTION) {
                e->mode = EM_EDITING;
                size_t cybegin = e->c.y;
                size_t cyend = e->cs.y;
                if (e->c.y > e->cs.y) {
                    size_t temp = cybegin;
                    cybegin = cyend;
                    cyend = temp;
                }
                for (size_t y = cybegin; y <= cyend; y++) {
                    // editor_write_at(e, "    ", vec2ui(0, y));
                }
                e->mode = EM_SELECTION;
                e->cs.x += 4;
            } else {
                // editor_write_at(e, "    ", vec2ui(0, e->c.y));
            }
            e->c.x += 4;
        } break; 

        case EK_UNINDENT: {
            if (e->mode == EM_SELECTION) {
                size_t cybegin = e->c.y;
                size_t cyend = e->cs.y;
                if (e->c.y > e->cs.y) {
                    size_t temp = cybegin;
                    cybegin = cyend;
                    cyend = temp;
                }
                for (size_t y = cybegin; y <= cyend; y++) {
                    Line *line = list_get(&e->lines, y);
                    assert(line != NULL);
                    for (int i = 0; i < 4; i++) {
                        if (line->size == 0 || line->s[0] != ' ') break;
                        line_delete_char(line, 0);
                        if (y == cyend) {
                            e->c.x--;
                            e->cs.x--;
                        }
                    }
                }
            }
            Line *line = list_get(&e->lines, e->c.y);
            assert(line != NULL);
            for (int i = 0; i < 4; i++) {
                if (line->size == 0 || line->s[0] != ' ') break;
                line_delete_char(line, 0);
                e->c.x--;
            }
        } break; 

        case EK_LINE_BELOW: {
            editor_new_line_at(e, "", e->c.y + 1);
            e->be.cur = editor_move(e, EK_DOWN, e->be.cur);
        } break;

        case EK_LINE_ABOVE: {
            editor_new_line(e, "");
        } break;

        case EK_REMOVE_LINE: {
            editor_remove_line(e);
            e->be.cur = editor_move(e, EK_UP, e->be.cur);
        } break;

        case EK_RETURN:
        case EK_BREAK_LINE: {
            e->be.cur = be_insert_s(&e->be, "\n");
        } break;

        default:
            assert(0);
    }

    return pos;
}

static void editor_action(Editor *e, EditorKey key)
{
    switch (key) {
        case EK_SAVE: {
            save_file(e);
        } break;

        case EK_SEARCH_START: {
            if (e->mode != EM_SEARCHING) {
                editor_search_start(e);
            }
        } break;

        case EK_COPY: {
            editor_selection_copy(e);
        } break;

        case EK_PASTE: {
            // e->c = editor_paste(e);
        } break;

        case EK_CUT: {
            editor_selection_copy(e);
            e->be.cur = editor_selection_delete(e, e->be.cur, e->select_cur);
        } break;

        default:
            assert(0);
    }
}

static void editor_browsing(Editor *e, EditorKey key)
{
    switch (key) {
        case EK_LEFT:
        case EK_UP: {
            e->be.cur = editor_move(e, EK_UP, e->be.cur);
        } break;
            
        case EK_RIGHT:
        case EK_DOWN: {
            e->be.cur = editor_move(e, EK_DOWN, e->be.cur);
        } break;

        case EK_RETURN: {
            Line_ line = be_get_line_at_(&e->be, e->be.cur);
            editor_open(e, &e->be.data.data[line.home], line.end - line.home);
        } break;

        case EK_HOME: {
            e->c.y = 0;
        } break;

        case EK_END: {
            if (e->lines.length > 0) {
                e->c.y = e->lines.length - 1;
            }
        } break;    

        default:
            assert(0);
    }
}

static size_t editor_select(Editor *e, EditorKey key, size_t cur)
{
    switch (key) {
        case EK_SELECT_LEFT: {
            cur = editor_move(e, EK_LEFT, cur);
        } break;

        case EK_SELECT_RIGHT: {
            cur = editor_move(e, EK_RIGHT, cur);
        } break;

        case EK_SELECT_UP: {
            cur = editor_move(e, EK_UP, cur);
        } break;

        case EK_SELECT_DOWN: {
            cur = editor_move(e, EK_DOWN, cur);
        } break;

        case EK_SELECT_LEFTW: {
            cur = editor_move(e, EK_LEFTW, cur);
        } break;

        case EK_SELECT_RIGHTW: {
            cur = editor_move(e, EK_RIGHTW, cur);
        } break;

        case EK_SELECT_LINE_HOME: {
            cur = editor_move(e, EK_LINE_HOME, cur);
        } break;

        case EK_SELECT_LINE_END: {
            cur = editor_move(e, EK_LINE_END, cur);
        } break;

        // case EK_SELECT_WORD: {
        //     const char *s = editor_get_line(e);

        //     if (isspace(s[pos.x]) || s[pos.x] == '\0') {
        //         e->mode = EM_EDITING;
        //         return pos;
        //     }

        //     while ((!isspace(s[pos.x]) && s[pos.x] != '\0') && pos.x > 0) {
        //         pos = editor_move(e, EK_LEFT, pos);
        //     }
        //     if (isspace(s[pos.x])) {
        //         pos = editor_move(e, EK_RIGHT, pos);
        //     }
        //     e->cs = pos;
        //     pos = editor_move(e, EK_RIGHTW, pos);
        // } break;

        // case EK_SELECT_LINE: {
        //     e->cs = vec2ui(0, pos.y);
        //     pos = editor_move(e, EK_LINE_END, pos);
        // } break;

        // case EK_SELECT_OUTER_BLOCK: {
        //     if (pos.x == 0 && pos.y == 0) {
        //         e->mode = EM_EDITING;
        //         return pos;
        //     }

        //     const char *s = NULL;
            
        //     size_t stack_i = 0;
        //     Vec2ui save_cursor = pos;
        //     while (pos.x > 0 || pos.y > 0) { // will not find brackets here, even if there's any
        //         pos = editor_move(e, EK_LEFT, pos);
        //         s = editor_get_line(e);
        //         if (*strchrnul("{[(", s[pos.x]) != '\0') {
        //             if (stack_i == 0) break;
        //             stack_i--;
        //         }
        //         if (*strchrnul("}])", s[pos.x]) != '\0') {
        //             stack_i++;
        //         }
        //     }

        //     if (*strchrnul("{[(", s[pos.x]) != '\0') { // if found, find the corresponding closing bracket
        //         e->cs = pos;
        //         find_scope_end(e);
        //         pos = editor_move(e, EK_RIGHT, pos);
        //     } else {
        //         pos = save_cursor;
        //     }
        // } break;

        // case EK_SELECT_NEXT_BLOCK: {
        //     const char *s = editor_get_line(e);
        //     size_t slen = strlen(s);

        //     while ((isspace(s[pos.x]) || s[pos.x] == '\0') && 
        //            (pos.x < slen || pos.y + 1 < e->lines.length)) 
        //     {
        //         pos = editor_move(e, EK_RIGHT, pos);
        //         s = editor_get_line(e);
        //         slen = strlen(s);
        //     }

        //     if (*strchrnul("{[(", s[pos.x]) != '\0') {
        //         find_scope_end(e);
        //         pos = editor_move(e, EK_RIGHT, pos);
        //     } else {
        //         while ((*strchrnul("{[(", s[pos.x]) == '\0') &&
        //                (pos.x < slen || pos.y + 1 < e->lines.length)) 
        //         {
        //             pos = editor_move(e, EK_RIGHT, pos);
        //             s = editor_get_line(e);
        //         }
        //     }
        // } break;

        case EK_SELECT_HOME: {
            cur = editor_move(e, EK_HOME, cur);
        } break;

        case EK_SELECT_END: {
            cur = editor_move(e, EK_END, cur);
        } break;
    
        case EK_SELECT_NEXT_PARAGRAPH: {
            cur = editor_move(e, EK_NEXT_PARAGRAPH, cur);
        } break;
        
        case EK_SELECT_PREV_PARAGRAPH: {
            cur = editor_move(e, EK_PREV_PARAGRAPH, cur);
        } break;
        
        case EK_SELECT_ALL: {
            e->cs = vec2uis(0);
            cur = editor_move(e, EK_END, cur);
        } break;

        default:
            assert(0);
    }

    return cur;
}

static_assert(EK_COUNT == 52, "The number of editor keys has changed");

static size_t editor_selection_delete(Editor *e, size_t begin, size_t end)
{
    assert(e->mode == EM_SELECTION);

    if (begin > end) {
        size_t temp = begin;
        begin = end;
        end = temp;
    }

    be_delete_sn_from(&e->be, end - begin, begin);

    return begin;
}

static void editor_selection_copy(Editor *e)
{
    if (e->clipboard != NULL) {
        free(e->clipboard);
        e->clipboard = NULL;
    }
    e->clipboard = editor_retrieve_selection(e);
}

static void editor_search_start(Editor *e)
{
    e->searchbuf[0] = '\0';
    e->match = vec2is(-1);

    if (e->mode == EM_SELECTION) {
        const char *s = editor_retrieve_selection(e);
        if (strlen(s) < sizeof(e->searchbuf)) {
            strcpy(e->searchbuf, s);
            e->searchbuf[strlen(s)] = '\0';
            if (vec2ui_cmp_yx(e->c, e->cs) > 0) {
                e->c = e->cs;
            }
            e->match = vec2i(e->c.x, e->c.y);
        }
    }

    e->mode = EM_SEARCHING;
}

static Vec2i editor_search_next(Editor *e, Vec2ui pos)
{
    size_t searchlen = strlen(e->searchbuf);

    // From the cur up to the end of the file
    for (size_t cy = pos.y; cy < e->lines.length; cy++) {
        const Line *line = (Line *) list_get(&e->lines, cy);
        size_t start_cx = (cy == pos.y) ? pos.x : 0;
        for (size_t cx = start_cx; cx < line->size; cx++) {
            if (strncmp(&line->s[cx], e->searchbuf, searchlen) == 0) {
                return vec2i(cx, cy);
            }
        }
    }

    // From the start of the file up to the cur
    for (size_t cy = 0; cy <= pos.y; cy++) {
        const Line *line = (Line *) list_get(&e->lines, cy);
        size_t end_cx = (cy == pos.y) ? pos.x : line->size;
        for (size_t cx = 0; cx < end_cx; cx++) {
            if (strncmp(&line->s[cx], e->searchbuf, searchlen) == 0) {
                return vec2i(cx, cy);
            }
        }
    }

    return vec2is(-1);
}

static Vec2i editor_search_prev(Editor *e, Vec2ui pos)
{
    size_t searchlen = strlen(e->searchbuf);

    // From the cur down to the start of the file
    for (int cy = pos.y; cy >= 0; cy--) {
        const Line *line = (Line *) list_get(&e->lines, cy);
        if (line->size == 0) continue;
        size_t start_cx = (cy == (int) pos.y) ? pos.x : line->size - 1;
        for (int cx = start_cx; cx >= 0; cx--) {
            if (strncmp(&line->s[cx], e->searchbuf, searchlen) == 0) {
                return vec2i(cx, cy);
            }
        }
    }

    // From the end of the file down to the cur
    for (int cy = (int) e->lines.length - 1; cy >= (int) pos.y; cy--) {
        const Line *line = (Line *) list_get(&e->lines, cy);
        if (line->size == 0) continue;
        size_t end_cx = (cy == (int) pos.y) ? pos.x : 0;
        for (int cx = (int) line->size - 1; cx >= (int) end_cx; cx--) {
            if (strncmp(&line->s[cx], e->searchbuf, searchlen) == 0) {
                return vec2i(cx, cy);
            }
        }
    }

    return vec2is(-1);
}

/* File I/O */

void editor_open(Editor *e, const char *path, size_t pathlen)
{
    assert(path != NULL);

    update_pathname(e, path, pathlen);

    const char *pathname = e->pathname.data;
    errno = 0;
    struct stat statbuf;
    if (stat(pathname, &statbuf) != 0) {
        if (errno == ENOENT) return; // File does not exist, don't crash
        fprintf(stderr, "Could not get stat for \"%s\": %s", pathname, strerror(errno));
        exit(1);
    }

    editor_clear(e);

    mode_t mode = statbuf.st_mode & S_IFMT;
    if (mode == S_IFDIR) { // Directory
        open_dir(e, pathname);
        e->mode = EM_BROWSING;
    } else if (mode == S_IFREG) { // Regular file
        open_file(e, pathname);
        e->mode = EM_EDITING;
    }
}

static void save_file(const Editor *e)
{
    FILE *file = fopen(e->pathname.data, "w");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\": %s\n", e->pathname.data, strerror(errno));
        exit(1);
    }

    fwrite(e->be.data.data, 1, e->be.data.size, file);
    fclose(file);
}       

static void open_file(Editor *e, const char *filename)
{
    be_load_from_file(&e->be, filename);
}

static int entrycmp(const void *ap, const void *bp)
{
    const char *a = *(const char**)ap;
    const char *b = *(const char**)bp;
    return strcmp(a, b);
}

static void open_dir(Editor *e, const char *dirname)
{
    DIR *dirp = opendir(dirname);
    if (dirp == NULL) {
        fprintf(stderr, "Could not open dir \"%s\": %s\n", dirname, strerror(errno));
        exit(1);
    }

    da_var_zero(entries, char *);

    errno = 0;
    struct dirent *direntry;
    while ((direntry = readdir(dirp)) != NULL) {
        char *s = strdup(direntry->d_name);
        da_append(&entries, &s);
    }

    if (errno) {
        fprintf(stderr, "Could not read dir: \"%s\": %s\n", dirname, strerror(errno));
        if (closedir(dirp) != 0) {
            fprintf(stderr, "Could not close dir: \"%s\": %s\n", dirname, strerror(errno));
        };
        exit(1);
    }

    qsort(entries.data, entries.size, TYPESIZE(&entries), entrycmp);

    size_t cur = e->be.cur;
    for (size_t i = 0; i < entries.size; i++) {
        cur = editor_write_at(e, entries.data[i], cur);
        if (i + 1 != entries.size) {
            cur = editor_write_at(e, "\n", cur);
        }
    }

    da_end(&entries);
}

static int str_rindex_n(const char *s, size_t n, char c)
{
    for (int i = n; i >= 0; i--) {
        if (s[i] == c) {
            return i;
        }
    }
    return -1;
}

static void update_pathname(Editor *e, const char *path, size_t pathlen)
{
    assert(e->pathname.size > 0);

    char *pathdup = strndup(path, pathlen);
    char *save_ptr;
    pathdup = strtok_r(pathdup, "/", &save_ptr);
    while (pathdup != NULL) {
        if (strcmp(pathdup, "..") == 0) {
            int index = str_rindex_n(e->pathname.data, e->pathname.size, '/');
            if (index <= 0) index = 1;
            e->pathname.size = index;
        } else if (strcmp(pathdup, ".") != 0) {
            sb_pop_null(&e->pathname);
            sb_append_cstr(&e->pathname, "/");
            sb_append_n(&e->pathname, pathdup, pathlen);
        }
        pathdup = strtok_r(save_ptr, "/", &save_ptr);
    }
    sb_append_null(&e->pathname);

    free(pathdup);
}

// static void find_scope_end(Editor *e)
// {
//     const char *s = editor_get_line(e);
//     size_t slen = strlen(s);

//     size_t scope = 1;
//     while (scope > 0 && (e->c.x < slen || e->c.y + 1 < e->lines.length)) {
//         e->be.cur = editor_move(e, EK_RIGHT, e->be.cur);
//         s = editor_get_line(e);

//         if      (*strchrnul("{[(", s[e->c.x]) != '\0') scope++;
//         else if (*strchrnul("}])", s[e->c.x]) != '\0') scope--;
//     }
// }

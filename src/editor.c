#define _DEFAULT_SOURCE

#include "editor.h"
#include "ds/dynamic_array.h"

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
static void editor_select(Editor *e, EditorKey key);
static void editor_selection_delete(Editor *e);
static void editor_selection_copy(Editor *e);
#define editor_paste(e) editor_write(e, e->clipboard);

static void editor_search_start(Editor *e);
static Vec2i editor_search_match(Editor *e, Vec2ui pos);

static const char *get_pathname_cstr(const List *pathname);
static void update_pathname(List *pathname, const char *path);

static void find_scope_end(Editor *e);

static void line_dealloc(void *line);
static int line_compare(const void *line1, const void *line2);

Editor editor_init(const char *pathname)
{
    Editor e = {0};

    e.pathname = list_init(NULL, NULL);
    e.clipboard = NULL;

    char cwdbuf[256];
    getcwd(cwdbuf, sizeof(cwdbuf));
    char *save_ptr;
    char *path;
    path = strtok_r(cwdbuf, "/", &save_ptr);
    while (path != NULL) {
        list_append(&e.pathname, path, strlen(path) + 1);
        path = strtok_r(save_ptr, "/", &save_ptr);
    }

    e.lines = list_init(line_dealloc, line_compare);
    e.mode = EM_EDITING;

    editor_open(&e, pathname);
    if (e.lines.length == 0) {
        Line line = line_init("");
        list_append(&e.lines, &line, sizeof(line));
    }

    e.match = vec2is(-1);

    return e;
}

void editor_clear(Editor *e)
{
    e->c.x = 0;
    e->c.y = 0;
    e->mode = EM_EDITING;

    list_clear(&e->lines);
}

static_assert(sizeof(Editor) == 168, "Editor structure has changed");

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
                case EK_NEXT_EMPTY_LINE: 
                case EK_PREV_EMPTY_LINE: {
                    e->mode = EM_EDITING;

                    if (e->mode == EM_SELECTION) {
                        if ((key == (EK_LEFT || EK_UP) && 
                                vec2ui_cmp_yx(e->c, e->cs) > 0) ||
                            (key == (EK_RIGHT || EK_DOWN) &&
                                vec2ui_cmp_yx(e->c, e->cs) < 0))
                        {
                            e->c = e->cs;
                            return;
                        }
                    }

                    e->c = editor_move(e, key, e->c);
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
                        editor_selection_delete(e);
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
                case EK_SELECT_NEXT_EMPTY_LINE:
                case EK_SELECT_PREV_EMPTY_LINE:
                case EK_SELECT_ALL: {
                    if (e->mode != EM_SELECTION) {
                        e->cs = e->c;
                        e->mode = EM_SELECTION;
                    }
                    editor_select(e, key);
                } break;

                case EK_ESC: {
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
                            e->cs = editor_move(e, EK_LEFT, e->cs);
                        }
                        e->mode = EM_SELECTION;
                    }
                } break;

                case EK_SEARCH_NEXT: {
                    Vec2ui pos = editor_move(e, EK_RIGHT, e->c);
                    e->match = editor_search_match(e, pos);
                    if (e->match.x != -1) {
                        e->c = vec2ui(e->match.x, e->match.y);
                    }
                } break;

                default:
                    break;
            }
        } break;
    }
}

static_assert(EK_COUNT == 51, "The number of editor keys has changed");

Vec2ui editor_write_at(Editor *e, const char *s, Vec2ui pos)
{
    if (e->mode == EM_BROWSING) {
        return pos;
    }

    if (e->mode == EM_SEARCHING) {
        size_t buf_i = strlen(e->searchbuf);
        assert(buf_i + strlen(s) < 64);
        strcpy(e->searchbuf + buf_i, s);
        e->searchbuf[buf_i + strlen(s)] = '\0';

        e->match = editor_search_match(e, e->c);
        if (e->match.x != -1) {
            return vec2ui(e->match.x, e->match.y);
        }
        
        return pos;
    }

    if (e->mode == EM_SELECTION) {
        editor_selection_delete(e);
        pos = e->c;
        e->mode = EM_EDITING;
    }

    String_View sv_chunk = sv_from_parts(s, strlen(s));
    while (sv_chunk.count > 0) {
        String_View sv_line = {0};
        char *cstr;

        if (sv_try_chop_by_delim(&sv_chunk, '\n', &sv_line)) {
            sv_c_str(cstr, sv_line);
        } else {
            sv_c_str(cstr, sv_chunk);
            sv_chunk = SV_NULL;
        }

        if (pos.y >= e->lines.length) {
            Line line = line_init_n(cstr, strlen(cstr));
            list_insert(&e->lines, &line, sizeof(line), pos.y);
        } else {
            Line *line = list_get(&e->lines, pos.y);
            if (pos.x > line->size) {
                pos.x = line->size;
            }
            line_write_n(line, cstr, strlen(cstr), pos.x);
        }
        pos.x += strlen(cstr);

        if (sv_chunk.data != NULL) { // there's a newline
            editor_break_line_at(e, pos);
            pos.y++;
            pos.x = 0;
        }
    }

    return pos;
}

char *editor_get_data(const Editor *e)
{
    da_var_zero(data, char);

    for (int cy = 0; cy < (int) e->lines.length; cy++) {
        const char *s = editor_get_line_at(e, cy);
        da_append_n(&data, s, strlen(s));
        if (cy + 1 != (int) e->lines.length) {
            da_append(&data, "\n");
        }
    }

    da_append(&data, "\0");

    char *s;
    da_get_copy(&data, s);
    da_end(&data);

    return s;
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

    Vec2ui csbegin = e->c;
    Vec2ui csend = e->cs;
    if (vec2ui_cmp_yx(e->c, e->cs) > 0) {
        Vec2ui temp = csbegin;
        csbegin = csend;
        csend = temp;
    }

    da_var_zero(selectbuf, char);
    for (int cy = csbegin.y; cy <= (int) csend.y; cy++) {
        const char *s = editor_get_line_at(e, cy);
        size_t slen = strlen(s);

        size_t line_cx_begin = 0;
        size_t line_cx_end = slen;
        if (cy == (int) csbegin.y) {
            line_cx_begin = (csbegin.x < slen) ? csbegin.x : slen;
        }
        if (cy == (int) csend.y) {
            line_cx_end = (csend.x < slen) ? csend.x : slen;
        }

        da_append_n(&selectbuf, &s[line_cx_begin], line_cx_end - line_cx_begin);
        if (cy != (int) csend.y) {
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

void editor_merge_line_at(Editor *e, size_t at)
{
    assert(at + 1 < e->lines.length);

    Line *line       = list_get(&e->lines, at);
    Line *line_after = list_get(&e->lines, at + 1);
    line_write(line, line_after->s, line->size);

    editor_remove_line_at(e, at + 1);
}

void editor_break_line_at(Editor *e, const Vec2ui pos)
{
    assert(pos.y < e->lines.length);

    Line *line = list_get(&e->lines, pos.y);
    assert(pos.x <= line->size);

    Line new_line = line_init(&line->s[pos.x]);
    list_insert(&e->lines, &new_line, sizeof(new_line), pos.y + 1);

    while (pos.x < line->size) {
        line_delete_char(line, pos.x);
    }
}

void editor_new_line_at(Editor *e, const char *s, size_t at)
{
    Line line = line_init(s);
    list_insert(&e->lines, &line, sizeof(line), at);
}

/* Editor Operations */

Vec2ui editor_delete_char_at(Editor *e, Vec2ui at)
{
    Line *line = list_get(&e->lines, at.y);
    if (at.y + 1 >= e->lines.length &&
        at.x >= line->size) 
    {
        return at;
    }

    if (at.x > line->size) at.x = line->size;

    if (at.x == line->size) {
        editor_merge_line_at(e, at.y);
    } else {
        line_delete_char(line, at.x);
    }

    return at;
}

Vec2ui editor_move(Editor *e, EditorKey key, Vec2ui pos)
{
    assert(pos.y < e->lines.length);
    Line *line = list_get(&e->lines, pos.y);

    switch (key) {
        case EK_LEFT: {
            if (pos.x > line->size) {
                pos.x = line->size;
            }

            if (pos.x > 0) {
                pos.x--;
            } else if (pos.y > 0) {
                pos = editor_move(e, EK_UP, pos);
                pos = editor_move(e, EK_LINE_END, pos);
            }
        } break;

        case EK_RIGHT: {
            if (pos.x < line->size) {
                pos.x++;
            } else if (pos.y + 1 < e->lines.length) {
                pos = editor_move(e, EK_DOWN, pos);
                pos = editor_move(e, EK_LINE_HOME, pos);
            }
        } break;

        case EK_UP: {
            if (pos.y > 0) {
                pos.y--;
            }
        } break;

        case EK_DOWN: {
            if (pos.y + 1 < e->lines.length) {
                pos.y++;
            }
        } break;

        case EK_LEFTW: {
            pos = editor_move(e, EK_LEFT, pos);

            const char *s = editor_get_line_at(e, pos.y);
            while (s[pos.x] == ' ' && pos.x > 0) {
                s = editor_get_line_at(e, pos.y);
                pos = editor_move(e, EK_LEFT, pos);
            }
            while (s[pos.x] != ' ' && pos.x > 0) {
                s = editor_get_line_at(e, pos.y);
                pos = editor_move(e, EK_LEFT, pos);
            }
            if (pos.x != 0) {
                pos = editor_move(e, EK_RIGHT, pos);
            }
        } break;

        case EK_RIGHTW: {
            pos = editor_move(e, EK_RIGHT, pos);

            const char *s = editor_get_line_at(e, pos.y);
            while (s[pos.x] == ' ') {
                s = editor_get_line_at(e, pos.y);
                pos = editor_move(e, EK_RIGHT, pos);
            }
            while (s[pos.x] != ' ' && pos.x < strlen(s)) {
                s = editor_get_line_at(e, pos.y);
                pos = editor_move(e, EK_RIGHT, pos);
            }
        } break;

        case EK_LINE_HOME: {
            pos.x = 0;
        } break;

        case EK_LINE_END: {
            pos.x = line->size;
        } break;

        case EK_HOME: {
            pos.x = 0;
            pos.y = 0;
        } break;

        case EK_END: {
            pos.y = (e->lines.length > 0) ? e->lines.length - 1 : 0;
            pos.x = line->size;
        } break;

        case EK_NEXT_EMPTY_LINE: {
            pos = editor_move(e, EK_DOWN, pos);
            while (pos.y + 1 < e->lines.length && editor_get_line_size(e) > 0) {
                pos = editor_move(e, EK_DOWN, pos);
            }
        } break;

        case EK_PREV_EMPTY_LINE: {
            pos = editor_move(e, EK_UP, pos);
            while (pos.y > 0 && editor_get_line_size(e) > 0) {
                pos = editor_move(e, EK_UP, pos);
            }
        } break;

        case EK_PAGEUP: {
            // TODO
        } break;

        case EK_PAGEDOWN: {
            // TODO
        } break;

        default:
            assert(0);
    }

    return pos;
}

Vec2ui editor_edit(Editor *e, EditorKey key, Vec2ui pos)
{
    switch (key) {
        case EK_BACKSPACE: {
            if (pos.y == 0 && pos.x == 0) return pos;
            pos = editor_move(e, EK_LEFT, pos);
            pos = editor_delete_char_at(e, pos);
        } break;

        case EK_DELETE: {
            pos = editor_delete_char_at(e, pos);
        } break;

        case EK_BACKSPACEW: {
            editor_process_key(e, EK_SELECT_LEFTW);
            editor_selection_delete(e);
        } break;

        case EK_DELETEW: {
            editor_process_key(e, EK_SELECT_RIGHTW);
            editor_selection_delete(e);
        } break;

        case EK_TAB: {
            pos = editor_write(e, "    ");
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
                    editor_write_at(e, "    ", vec2ui(0, y));
                }
                e->mode = EM_SELECTION;
                e->cs.x += 4;
            } else {
                editor_write_at(e, "    ", vec2ui(0, e->c.y));
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
            pos = editor_move(e, EK_DOWN, pos);
        } break;

        case EK_LINE_ABOVE: {
            editor_new_line(e, "");
        } break;

        case EK_REMOVE_LINE: {
            editor_remove_line(e);
            pos = editor_move(e, EK_UP, pos);
        } break;

        case EK_MERGE_LINE: {
            editor_merge_line(e);
        } break;

        case EK_RETURN:
        case EK_BREAK_LINE: {
            editor_break_line(e);
            pos = editor_move(e, EK_DOWN, pos);
            pos = editor_move(e, EK_LINE_HOME, pos);
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

        case EK_SEARCH_NEXT: {
            assert(e->mode == EM_SEARCHING);
            e->match = editor_search_match(e, e->c);
            if (e->match.x != -1) {
                e->c = vec2ui(e->match.x, e->match.y);
            }
        } break;

        case EK_COPY: {
            editor_selection_copy(e);
        } break;

        case EK_PASTE: {
            e->c = editor_paste(e);
        } break;

        case EK_CUT: {
            editor_selection_copy(e);
            editor_selection_delete(e);
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
            e->c = editor_move(e, EK_UP, e->c);
        } break;
            
        case EK_RIGHT:
        case EK_DOWN: {
            e->c = editor_move(e, EK_DOWN, e->c);
        } break;

        case EK_RETURN: {
            editor_open(e, editor_get_line(e));
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

static void editor_select(Editor *e, EditorKey key)
{
    switch (key) {
        case EK_SELECT_LEFT: {
            e->c = editor_move(e, EK_LEFT, e->c);
        } break;

        case EK_SELECT_RIGHT: {
            e->c = editor_move(e, EK_RIGHT, e->c);
        } break;

        case EK_SELECT_UP: {
            e->c = editor_move(e, EK_UP, e->c);
        } break;

        case EK_SELECT_DOWN: {
            e->c = editor_move(e, EK_DOWN, e->c);
        } break;

        case EK_SELECT_LEFTW: {
            e->c = editor_move(e, EK_LEFTW, e->c);
        } break;

        case EK_SELECT_RIGHTW: {
            e->c = editor_move(e, EK_RIGHTW, e->c);
        } break;

        case EK_SELECT_LINE_HOME: {
            e->c = editor_move(e, EK_LINE_HOME, e->c);
        } break;

        case EK_SELECT_LINE_END: {
            e->c = editor_move(e, EK_LINE_END, e->c);
        } break;

        case EK_SELECT_WORD: {
            const char *s = editor_get_line(e);

            if (isspace(s[e->c.x]) || s[e->c.x] == '\0') {
                e->mode = EM_EDITING;
                return;
            }

            while ((!isspace(s[e->c.x]) && s[e->c.x] != '\0') && e->c.x > 0) {
                e->c = editor_move(e, EK_LEFT, e->c);
            }
            if (isspace(s[e->c.x])) {
                e->c = editor_move(e, EK_RIGHT, e->c);
            }
            e->cs = e->c;
            e->c = editor_move(e, EK_RIGHTW, e->c);
        } break;

        case EK_SELECT_LINE: {
            e->cs = vec2ui(0, e->c.y);
            e->c = editor_move(e, EK_LINE_END, e->c);
        } break;

        case EK_SELECT_OUTER_BLOCK: {
            if (e->c.x == 0 && e->c.y == 0) {
                e->mode = EM_EDITING;
                return;
            }

            const char *s = NULL;
            
            size_t stack_i = 0;
            Vec2ui save_cursor = e->c;
            while (e->c.x > 0 || e->c.y > 0) { // will not find brackets here, even if there's any
                e->c = editor_move(e, EK_LEFT, e->c);
                s = editor_get_line(e);
                if (*strchrnul("{[(", s[e->c.x]) != '\0') {
                    if (stack_i == 0) break;
                    stack_i--;
                }
                if (*strchrnul("}])", s[e->c.x]) != '\0') {
                    stack_i++;
                }
            }

            if (*strchrnul("{[(", s[e->c.x]) != '\0') { // if found, find the corresponding closing bracket
                e->cs = e->c;
                find_scope_end(e);
                e->c = editor_move(e, EK_RIGHT, e->c);
            } else {
                e->c = save_cursor;
            }
        } break;

        case EK_SELECT_NEXT_BLOCK: {
            const char *s = editor_get_line(e);
            size_t slen = strlen(s);

            while ((isspace(s[e->c.x]) || s[e->c.x] == '\0') && 
                   (e->c.x < slen || e->c.y + 1 < e->lines.length)) 
            {
                e->c = editor_move(e, EK_RIGHT, e->c);
                s = editor_get_line(e);
                slen = strlen(s);
            }

            if (*strchrnul("{[(", s[e->c.x]) != '\0') {
                find_scope_end(e);
                e->c = editor_move(e, EK_RIGHT, e->c);
            } else {
                while ((*strchrnul("{[(", s[e->c.x]) == '\0') &&
                       (e->c.x < slen || e->c.y + 1 < e->lines.length)) 
                {
                    e->c = editor_move(e, EK_RIGHT, e->c);
                    s = editor_get_line(e);
                }
            }
        } break;

        case EK_SELECT_HOME: {
            e->c = editor_move(e, EK_HOME, e->c);
        } break;

        case EK_SELECT_END: {
            e->c = editor_move(e, EK_END, e->c);
        } break;
    
        case EK_SELECT_NEXT_EMPTY_LINE: {
            e->c = editor_move(e, EK_NEXT_EMPTY_LINE, e->c);
        } break;
        
        case EK_SELECT_PREV_EMPTY_LINE: {
            e->c = editor_move(e, EK_PREV_EMPTY_LINE, e->c);
        } break;
        
        case EK_SELECT_ALL: {
            e->cs = vec2uis(0);
            e->c = editor_move(e, EK_END, e->c);
        } break;

        default:
            assert(0);
    }
}

static_assert(EK_COUNT == 51, "The number of editor keys has changed");

static void editor_selection_delete(Editor *e)
{
    assert(e->mode == EM_SELECTION);

    Vec2ui csbegin = e->c;
    Vec2ui csend = e->cs;
    if (vec2ui_cmp_yx(e->c, e->cs) > 0) {
        Vec2ui temp = csbegin;
        csbegin = csend;
        csend = temp;
    }
    
    for (int cy = csend.y; cy >= (int) csbegin.y; cy--) {
        if (cy != (int) csend.y && cy != (int) csbegin.y) {
            editor_remove_line_at(e, cy);
            continue;
        }

        const char *s = editor_get_line_at(e, cy);
        size_t slen = strlen(s);

        size_t line_cx_begin = 0;
        size_t line_cx_end = slen;
        if (cy == (int) csbegin.y) {
            line_cx_begin = (csbegin.x < slen) ? csbegin.x : slen;
        }
        if (cy == (int) csend.y) {
            line_cx_end = (csend.x < slen) ? csend.x : slen;
        }
        for (int cx = (int) line_cx_end - 1; cx >= (int) line_cx_begin; cx--) {
            editor_delete_char_at(e, vec2ui(cx, cy));
        }
        if (cy == (int) csbegin.y && cy != (int) csend.y) {
            editor_merge_line_at(e, cy);
        }
    }
    e->c = csbegin;
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
    e->mode = EM_SEARCHING;
    e->searchbuf[0] = '\0';
    e->match = vec2is(-1);
}

static Vec2i editor_search_match(Editor *e, Vec2ui pos)
{
    size_t searchlen = strlen(e->searchbuf);

    // From the cursor up to the end of the file
    for (size_t cy = pos.y; cy < e->lines.length; cy++) {
        const Line *line = (Line *) list_get(&e->lines, cy);
        size_t start_cx = (cy == pos.y) ? pos.x : 0;
        for (size_t cx = start_cx; cx < line->size; cx++) {
            if (strncmp(&line->s[cx], e->searchbuf, searchlen) == 0) {
                return vec2i(cx, cy);
            }
        }
    }

    // From the start of the file up to the cursor
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

/* File I/O */

void editor_open(Editor *e, const char *path)
{
    assert(path != NULL);

    update_pathname(&e->pathname, path);
    const char *pathname = get_pathname_cstr(&e->pathname);
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
    const char *pathname = get_pathname_cstr(&e->pathname);
    FILE *file = fopen(pathname, "w");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\": %s\n", pathname, strerror(errno));
        exit(1);
    }

    for (size_t i = 0; i < e->lines.length; i++) {
        Line *line = list_get(&e->lines, i);
        fwrite(line->s, 1, line->size, file);
        fputc('\n', file);
    }
    fclose(file);
}       

static void open_file(Editor *e, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\": %s\n", filename, strerror(errno));
        exit(1);
    }

    static char chunk[64 * 1024];
    while (!feof(file)) {
        size_t n_bytes = fread(chunk, 1, sizeof(chunk), file);
        String_View sv_chunk = {
            .data = chunk,
            .count = n_bytes
        };

        while (sv_chunk.count > 0) {
            String_View sv_line = {0};
            char *c_chunk;
            
            if (sv_try_chop_by_delim(&sv_chunk, '\n', &sv_line)) {
                sv_c_str(c_chunk, sv_line);
                Line line = line_init(c_chunk);
                list_append(&e->lines, &line, sizeof(line));
            } else {
                sv_c_str(c_chunk, sv_chunk);
                Line line = line_init(c_chunk);
                list_append(&e->lines, &line, sizeof(line));
                sv_chunk = SV_NULL;
            }
            free(c_chunk);
        }
    }
}

static void open_dir(Editor *e, const char *dirname)
{
    DIR *dirp = opendir(dirname);
    if (dirp == NULL) {
        fprintf(stderr, "Could not open dir \"%s\": %s\n", dirname, strerror(errno));
        exit(1);
    }

    errno = 0;
    struct dirent *direntry;
    while ((direntry = readdir(dirp)) != NULL) {
        Line line = line_init(direntry->d_name);
        list_append(&e->lines, &line, sizeof(line));
    }

    if (errno) {
        fprintf(stderr, "Could not read dir: \"%s\": %s\n", dirname, strerror(errno));
        if (closedir(dirp) != 0) {
            fprintf(stderr, "Could not close dir: \"%s\": %s\n", dirname, strerror(errno));
        };
        exit(1);
    }

    list_quicksort(&e->lines);
}

static const char *get_pathname_cstr(const List *pathname)
{
    if (pathname->length == 0) {
        return "/";
    }

    static char buffer[256] = {0};
    char *buffer_ptr = buffer;
    for (size_t i = 0; i < pathname->length; i++) {
        *buffer_ptr++ = '/';
        const char *path = (char *) list_get(pathname, i);
        strcpy(buffer_ptr, path);
        buffer_ptr += strlen(path);
    }
    *buffer_ptr = '\0';
    return buffer;
}

static void update_pathname(List *pathname, const char *path)
{
    assert(pathname != NULL && path != NULL);

    char *path_dump = strdup(path);
    char *save_ptr;
    path = strtok_r(path_dump, "/", &save_ptr);
    while (path != NULL) {
        if (strcmp(path, "..") == 0) {
            if (pathname->length > 0) {
                list_remove(pathname, pathname->length - 1);
            }
        } else if (strcmp(path, ".") != 0) {
            list_append(pathname, path, strlen(path) + 1);
        }
        path = strtok_r(save_ptr, "/", &save_ptr);
    }
}

static void find_scope_end(Editor *e)
{
    const char *s = editor_get_line(e);
    size_t slen = strlen(s);

    size_t scope = 1;
    while (scope > 0 && (e->c.x < slen || e->c.y + 1 < e->lines.length)) {
        e->c = editor_move(e, EK_RIGHT, e->c);
        s = editor_get_line(e);

        if      (*strchrnul("{[(", s[e->c.x]) != '\0') scope++;
        else if (*strchrnul("}])", s[e->c.x]) != '\0') scope--;
    }
}

static void line_dealloc(void *line)
{
    line_destroy((Line *) line);
}

static int line_compare(const void *line1, const void *line2)
{
    return strcmp(((Line *) line1)->s, ((Line *) line2)->s);
}


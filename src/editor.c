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
static void editor_delete_char(Editor *e);
static void editor_browsing(Editor *e, EditorKey key);
static void editor_move(Editor *e, EditorKey key);
static void editor_edit(Editor *e, EditorKey key);
static void editor_action(Editor *e, EditorKey key);
static void editor_select(Editor *e, EditorKey key);
static void editor_delete_selection(Editor *e);
static void editor_copy_selection(Editor *e);
static void editor_paste(Editor *e);

const char *get_pathname_cstr(const List *pathname);
static void update_pathname(List *pathname, const char *path);

static void line_dealloc(void *line);
static int line_compare(void *line1, void *line2);

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

    return e;
}

void editor_clear(Editor *e)
{
    e->c.x = 0;
    e->c.y = 0;
    e->mode = EM_EDITING;

    list_clear(&e->lines);
}

static_assert(sizeof(Editor) == 96, "Editor structure has changed");

void editor_process_key(Editor *e, EditorKey key)
{
    switch (e->mode) {
        case EM_SELECTION: 
        case EM_SELECTION_RESOLUTION:
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
                case EK_PAGEDOWN: {
                    if (e->mode == EM_SELECTION) {
                        e->mode = EM_SELECTION_RESOLUTION;
                    }
                    editor_move(e, key);
                    e->mode = EM_EDITING;
                } break;

                case EK_BACKSPACE:
                case EK_DELETE:
                case EK_LINE_BELOW:
                case EK_LINE_ABOVE:
                case EK_REMOVE_LINE:
                case EK_MERGE_LINE:
                case EK_RETURN: // Return effect defaults to BREAK_LINE
                case EK_BREAK_LINE:
                case EK_TAB: {
                    if (e->mode == EM_SELECTION) {
                        editor_delete_selection(e);
                    }
                    editor_edit(e, key);
                    e->mode = EM_EDITING;
                } break;

                case EK_SAVE:
                case EK_COPY:
                case EK_PASTE: {
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
                case EK_SELECT_HOME:
                case EK_SELECT_END:
                case EK_SELECT_ALL: {
                    if (e->mode != EM_SELECTION) {
                        e->cs = e->c;
                        e->mode = EM_SELECTION;
                    }
                    editor_select(e, key);
                } break;

                default:
                    assert(0);
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
    }
}

static_assert(EK_COUNT == 35, "The number of editor keys has changed");

void editor_write(Editor *e, const char *s)
{
    if (e->mode == EM_BROWSING) {
        return;
    }
    if (e->mode == EM_SELECTION) {
        editor_delete_selection(e);
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

        if (e->c.y == e->lines.length) {
            Line line = line_init_n(cstr, strlen(cstr));
            list_insert(&e->lines, &line, sizeof(line), e->c.y);
        } else {
            Line *line = list_get(&e->lines, e->c.y);
            if (e->c.x > line->size) {
                e->c.x = line->size;
            }
            line_write_n(line, cstr, strlen(cstr), e->c.x);
        }
        e->c.x += strlen(cstr);

        if (sv_chunk.data != NULL) {
            editor_edit(e, EK_BREAK_LINE);
        }
    }
}

size_t editor_get_line_size(const Editor *e)
{
    Line *line = list_get(&e->lines, e->c.y);
    return (line == NULL) ? 0: line->size;
}

const char *editor_get_line_at(const Editor *e, size_t at)
{
    const Line *line = list_get(&e->lines, at);
    return (line == NULL) ? NULL : line->data;
}

const char *editor_get_line(const Editor *e)
{
    return editor_get_line_at(e, e->c.y);
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

    da_create_zero(selectbuf, char);

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

    selectbuf.data[selectbuf.size] = '\0';

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
    assert (at + 1 < e->lines.length);

    Line *line       = list_get(&e->lines, at);
    Line *line_after = list_get(&e->lines, at + 1);
    line_write(line, line_after->data, line->size);

    editor_remove_line_at(e, at + 1);
}

void editor_break_line_at(Editor *e, size_t at) // todo change size_t to vec2ui
{
    assert(at < e->lines.length);

    Line *line = list_get(&e->lines, at);
    Line new_line = line_init(&line->data[e->c.x]);
    list_insert(&e->lines, &new_line, sizeof(new_line), at + 1);

    while (e->c.x < line->size) {
        line_delete_char(line, e->c.x);
    }
}

void editor_new_line_at(Editor *e, const char *s, size_t at)
{
    Line line = line_init(s);
    list_insert(&e->lines, &line, sizeof(line), at);
}

/* Editor Operations */

void editor_delete_char_at(Editor *e, Vec2ui at)
{
    Line *line = list_get(&e->lines, at.y);
    if (at.y + 1 >= e->lines.length &&
        at.x >= line->size) 
    {
        return;
    }

    if (at.x == line->size) {
        editor_merge_line(e);
    } else {
        line_delete_char(line, at.x);
    }
}

void editor_delete_char(Editor *e)
{
    editor_delete_char_at(e, e->c);
}

static void editor_move(Editor *e, EditorKey key)
{
    switch (key) {
        case EK_LEFT: {
            if (e->mode == EM_SELECTION_RESOLUTION) {
                int diff = vec2ui_cmp_yx(e->c, e->cs);
                if (diff > 0) {
                    e->c = e->cs;
                }
                e->mode = EM_EDITING;
                return;
            }

            if (e->c.x > 0) {
                size_t line_size = editor_get_line_size(e);
                if (e->c.x > line_size) {
                    e->c.x = line_size;
                    editor_move(e, EK_LEFT);
                } else {
                    e->c.x--;
                }
            } else if (e->c.y > 0) {
                editor_move(e, EK_UP);
                editor_move(e, EK_LINE_END);
            }
        } break;

        case EK_RIGHT: {
            if (e->mode == EM_SELECTION_RESOLUTION) {
                int diff = vec2ui_cmp_yx(e->c, e->cs);
                if (diff < 0) {
                    e->c = e->cs;
                }
                e->mode = EM_EDITING;
                return;
            }

            if (e->c.x < editor_get_line_size(e)) {
                e->c.x++;
            } else if (e->c.y + 1 < e->lines.length) {
                editor_move(e, EK_DOWN);
                editor_move(e, EK_LINE_HOME);
            }
        } break;

        case EK_UP: {
            if (e->c.y > 0) {
                e->c.y--;
            }
        } break;

        case EK_DOWN: {
            if (e->c.y + 1 < e->lines.length) {
                e->c.y++;
            }
        } break;

        case EK_LEFTW: {
            editor_move(e, EK_LEFT);

            const char *s = editor_get_line(e);
            while (s[e->c.x] == ' ') {
                s = editor_get_line(e);
                editor_move(e, EK_LEFT);
            }
            while (s[e->c.x] != ' ' && e->c.x > 0) {
                s = editor_get_line(e);
                editor_move(e, EK_LEFT);
            }
            if (e->c.x != 0) {
                editor_move(e, EK_RIGHT);
            }
        } break;

        case EK_RIGHTW: {
            editor_move(e, EK_RIGHT);

            const char *s = editor_get_line(e);
            while (s[e->c.x] == ' ') {
                s = editor_get_line(e);
                editor_move(e, EK_RIGHT);
            }
            while (s[e->c.x] != ' ' && e->c.x < strlen(s)) {
                s = editor_get_line(e);
                editor_move(e, EK_RIGHT);
            }
        } break;

        case EK_LINE_HOME: {
            e->c.x = 0;
        } break;

        case EK_LINE_END: {
            e->c.x = editor_get_line_size(e);
        } break;

        case EK_HOME: {
            e->c.x = 0;
            e->c.y = 0;
        } break;

        case EK_END: {
            e->c.y = (e->lines.length > 0) ? e->lines.length - 1 : 0;
            e->c.x = editor_get_line_size(e);
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
}

static void editor_edit(Editor *e, EditorKey key)
{
    switch (key) {
        case EK_BACKSPACE: {
            if (e->mode == EM_SELECTION) return;
            if (e->c.y == 0 && e->c.x == 0) return;
            editor_move(e, EK_LEFT);
            editor_delete_char(e);
        } break;

        case EK_DELETE: {
            if (e->mode == EM_SELECTION) return;
            editor_delete_char(e);
        } break;

        case EK_TAB: {
            editor_write(e, "    ");
        } break;

        case EK_LINE_BELOW: {
            editor_new_line_at(e, "", e->c.y + 1);
            editor_move(e, EK_DOWN);
        } break;

        case EK_LINE_ABOVE: {
            editor_new_line(e, "");
        } break;

        case EK_REMOVE_LINE: {
            editor_remove_line(e);
            editor_move(e, EK_UP);
        } break;

        case EK_MERGE_LINE: {
            editor_merge_line(e);
        } break;

        case EK_RETURN:
        case EK_BREAK_LINE: {
            editor_break_line(e);
            editor_move(e, EK_DOWN);
            editor_move(e, EK_LINE_HOME);
        } break;

        default:
            assert(0);
    }
}

static void editor_action(Editor *e, EditorKey key)
{
    switch (key) {
        case EK_SAVE: {
            save_file(e);
        } break;

        case EK_COPY: {
            editor_copy_selection(e);
        } break;

        case EK_PASTE: {
            editor_paste(e);
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
            editor_move(e, EK_UP);
        } break;
            
        case EK_RIGHT:
        case EK_DOWN: {
            editor_move(e, EK_DOWN);
        } break;

        case EK_PAGEUP: {
            // TODO
        } break;

        case EK_PAGEDOWN: {
            // TODO
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
            editor_move(e, EK_LEFT);
        } break;

        case EK_SELECT_RIGHT: {
            editor_move(e, EK_RIGHT);
        } break;

        case EK_SELECT_UP: {
            editor_move(e, EK_UP);
        } break;

        case EK_SELECT_DOWN: {
            editor_move(e, EK_DOWN);
        } break;

        case EK_SELECT_LEFTW: {
            editor_move(e, EK_LEFTW);
        } break;

        case EK_SELECT_RIGHTW: {
            editor_move(e, EK_RIGHTW);
        } break;

        case EK_SELECT_LINE_HOME: {
            editor_move(e, EK_LINE_HOME);
        } break;

        case EK_SELECT_LINE_END: {
            editor_move(e, EK_LINE_END);
        } break;

        case EK_SELECT_HOME: {
            editor_move(e, EK_HOME);
        } break;

        case EK_SELECT_END: {
            editor_move(e, EK_END);
        } break;

        case EK_SELECT_ALL: {
            e->cs = vec2uis(0);
            editor_move(e, EK_END);
        } break;

        default:
            assert(0);
    }
}

static_assert(EK_COUNT == 35, "The number of editor keys has changed");

static void editor_delete_selection(Editor *e)
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
        e->c = csbegin;
    }
}

static void editor_copy_selection(Editor *e)
{
    if (e->clipboard != NULL) {
        free(e->clipboard);
        e->clipboard = NULL;
    }
    e->clipboard = editor_retrieve_selection(e);
}

static void editor_paste(Editor *e)
{
    editor_write(e, e->clipboard);
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
        if (errno == ENOENT) return; // File does not exist, it's fine
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
        fwrite(line->data, 1, line->size, file);
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

const char *get_pathname_cstr(const List *pathname)
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
    return buffer;
}

static void update_pathname(List *pathname, const char *path)
{
    assert(pathname != NULL && path != NULL);
    if (strcmp(path, "..") == 0) {
        if (pathname->length > 0) {
            list_remove(pathname, pathname->length - 1);
        }
    } else if (strcmp(path, ".") != 0) {
        list_append(pathname, path, strlen(path) + 1);
    }
}

static void line_dealloc(void *line)
{
    line_destroy((Line *) line);
}

static int line_compare(void *line1, void *line2)
{
    return strcmp(((Line *) line1)->data, ((Line *) line2)->data);
}










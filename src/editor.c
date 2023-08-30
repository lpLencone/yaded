#define _DEFAULT_SOURCE

#include "editor.h"

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

// File I/O
static void save_file(const Editor *e);
static void open_file(Editor *e, const char *filename);
static void open_dir(Editor *e, const char *dirname);

// Editor Operations
static void editor_delete_char(Editor *e);
static void editor_browsing(Editor *e, EditorKey key);

const char *get_pathname_cstr(const List *pathname);
static void update_pathname(List *pathname, const char *path);

static void line_dealloc(void *line);
static int line_compare(void *line1, void *line2);

Editor editor_init(const char *pathname)
{
    Editor e = {0};

    e.pathname = list_init(NULL, NULL);

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
    e.mode = EDITOR_MODE_EDITING;

    editor_open(&e, pathname);

    return e;
}

void editor_clear(Editor *e)
{
    e->cx = 0;
    e->cy = 0;
    e->mode = EDITOR_MODE_EDITING;

    list_clear(&e->lines);
}

void editor_process_key(Editor *e, EditorKey key)
{
    switch (e->mode) {
        case EDITOR_MODE_EDITING: {
            switch (key) {
                case EDITOR_KEY_LEFT:
                case EDITOR_KEY_RIGHT:
                case EDITOR_KEY_UP:
                case EDITOR_KEY_DOWN:
                case EDITOR_KEY_LEFTW:
                case EDITOR_KEY_RIGHTW:
                case EDITOR_KEY_LINE_HOME:
                case EDITOR_KEY_LINE_END:
                case EDITOR_KEY_HOME:
                case EDITOR_KEY_END:
                case EDITOR_KEY_PAGEUP:
                case EDITOR_KEY_PAGEDOWN: {
                    editor_move(e, key);
                } break;

                case EDITOR_KEY_BACKSPACE:
                case EDITOR_KEY_DELETE:
                case EDITOR_KEY_RETURN:
                case EDITOR_KEY_LINE_BELOW:
                case EDITOR_KEY_LINE_ABOVE:
                case EDITOR_KEY_REMOVE_LINE:
                case EDITOR_KEY_MERGE_LINE:
                case EDITOR_KEY_BREAK_LINE:
                case EDITOR_KEY_TAB: {
                    editor_edit(e, key);
                } break;

                case EDITOR_KEY_SAVE:
                case EDITOR_KEY_BROWSE: {
                    editor_action(e, key);
                } break;

                default:
                    assert(0);
            }
        } break;

        case EDITOR_MODE_BROWSING: {
            switch (key) {
                case EDITOR_KEY_LEFT:
                case EDITOR_KEY_RIGHT:
                case EDITOR_KEY_UP:
                case EDITOR_KEY_DOWN:
                case EDITOR_KEY_PAGEUP:
                case EDITOR_KEY_PAGEDOWN: 
                case EDITOR_KEY_RETURN: {
                    editor_browsing(e, key);
                } break;

                default:
                    assert(0);
            }
        } break;

        case EDITOR_MODE_SELECTION: {
            switch (key) {
                default:
                    assert(0);
            }
        } break;
    }
}

static_assert(EDITOR_KEY_COUNT == 23, "The number of editor keys has changed");

void editor_write(Editor *e, const char *s)
{
    if (e->mode == EDITOR_MODE_BROWSING) {
        return;
    }

    if (e->cy == e->lines.length) {
        Line line = line_init(s);
        list_insert(&e->lines, &line, sizeof(line), e->cy);
    } else {
        Line *line = list_get(&e->lines, e->cy);
        if (e->cx > line->size) {
            e->cx = line->size;
        }
        line_write(line, s, e->cx);
    }

    e->cx += strlen(s);
}

size_t editor_get_line_size(const Editor *e)
{
    Line *line = list_get(&e->lines, e->cy);
    return (line == NULL) ? 0: line->size;
}

const char *editor_get_line_at(const Editor *e, size_t at)
{
    const Line *line = list_get(&e->lines, at);
    return (line == NULL) ? NULL : line->s;
}

const char *editor_get_line(const Editor *e)
{
    return editor_get_line_at(e, e->cy);
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
    line_write(line, line_after->s, line->size);

    editor_remove_line_at(e, at + 1);
}

void editor_break_line_at(Editor *e, size_t at)
{
    Line *line = list_get(&e->lines, at);
    Line new_line = line_init(&line->s[e->cx]);
    list_insert(&e->lines, &new_line, sizeof(new_line), at + 1);

    while (e->cx < line->size) {
        line_delete_char(line, e->cx);
    }
}

void editor_new_line_at(Editor *e, const char *s, size_t at)
{   
    Line line = line_init(s);
    list_insert(&e->lines, &line, sizeof(line), at);
}

/* Editor Operations */

void editor_delete_char(Editor *e)
{
    if (e->cy + 1 >= e->lines.length &&
        e->cx >= editor_get_line_size(e)) 
    {
        return;
    }

    Line *line = list_get(&e->lines, e->cy);

    if (e->cx > line->size) e->cx = line->size;
    if (e->cx == line->size) {
        editor_merge_line(e);
    } else {
        line_delete_char(line, e->cx);
    }
}

void editor_move(Editor *e, EditorKey key)
{
    switch (key) {
        case EDITOR_KEY_LEFT: {
            if (e->cx > 0) {
                size_t line_size = editor_get_line_size(e);
                if (e->cx > line_size) {
                    e->cx = line_size;
                    editor_move(e, EDITOR_KEY_LEFT);
                } else {
                    e->cx--;
                }
            } else if (e->cy > 0) {
                editor_move(e, EDITOR_KEY_UP);
                editor_move(e, EDITOR_KEY_LINE_END);
            }
        } break;

        case EDITOR_KEY_RIGHT: {
            if (e->cx < editor_get_line_size(e)) {
                e->cx++;
            } else if (e->cy + 1 < e->lines.length) {
                editor_move(e, EDITOR_KEY_DOWN);
                editor_move(e, EDITOR_KEY_LINE_HOME);
            }
        } break;

        case EDITOR_KEY_UP: {
            if (e->cy > 0) {
                e->cy--;
            }
        } break;

        case EDITOR_KEY_DOWN: {
            if (e->cy + 1 < e->lines.length) {
                e->cy++;
            }
        } break;

        case EDITOR_KEY_LEFTW: {
            editor_move(e, EDITOR_KEY_LEFT);

            const char *s = editor_get_line(e);
            while (s[e->cx] == ' ') {
                s = editor_get_line(e);
                editor_move(e, EDITOR_KEY_LEFT);
            }
            while (s[e->cx] != ' ' && e->cx > 0) {
                s = editor_get_line(e);
                editor_move(e, EDITOR_KEY_LEFT);
            }
            if (e->cx != 0) {
                editor_move(e, EDITOR_KEY_RIGHT);
            }
        } break;

        case EDITOR_KEY_RIGHTW: {
            editor_move(e, EDITOR_KEY_RIGHT);

            const char *s = editor_get_line(e);
            while (s[e->cx] == ' ') {
                s = editor_get_line(e);
                editor_move(e, EDITOR_KEY_RIGHT);
            }
            while (s[e->cx] != ' ' && e->cx < strlen(s)) {
                s = editor_get_line(e);
                editor_move(e, EDITOR_KEY_RIGHT);
            }
        } break;

        case EDITOR_KEY_LINE_HOME: {
            e->cx = 0;
        } break;

        case EDITOR_KEY_LINE_END: {
            e->cx = editor_get_line_size(e);
        } break;

        case EDITOR_KEY_HOME: {
            e->cx = 0;
            e->cy = 0;
        } break;

        case EDITOR_KEY_END: {
            e->cy = (e->lines.length > 0) ? e->lines.length - 1 : 0;
            e->cx = editor_get_line_size(e);
        } break;

        case EDITOR_KEY_PAGEUP: {
            // TODO
        } break;

        case EDITOR_KEY_PAGEDOWN: {
            // TODO
        } break;

        default:
            assert(0);
    }
}

void editor_edit(Editor *e, EditorKey key)
{
    switch (key) {
        case EDITOR_KEY_BACKSPACE: {
            if (e->cx == 0 && e->cy == 0) return;
            editor_move(e, EDITOR_KEY_LEFT);
            editor_delete_char(e);
        } break;

        case EDITOR_KEY_DELETE: {
            editor_delete_char(e);
        } break;

        case EDITOR_KEY_RETURN: {
            editor_break_line(e);
            editor_move(e, EDITOR_KEY_DOWN);
        } break;

        case EDITOR_KEY_TAB: {
            // TODO
        } break;

        case EDITOR_KEY_LINE_BELOW: {
            editor_new_line_at(e, "", e->cy + 1);
            editor_move(e, EDITOR_KEY_DOWN);
        } break;

        case EDITOR_KEY_LINE_ABOVE: {
            editor_new_line(e, "");
        } break;

        case EDITOR_KEY_REMOVE_LINE: {
            editor_remove_line(e);
            editor_move(e, EDITOR_KEY_UP);
        } break;

        case EDITOR_KEY_MERGE_LINE: {
            editor_merge_line(e);
        } break;

        case EDITOR_KEY_BREAK_LINE: {
            editor_break_line(e);
            editor_move(e, EDITOR_KEY_DOWN);
            editor_move(e, EDITOR_KEY_LINE_HOME);
        } break;

        default:
            assert(0);
    }
}

void editor_action(Editor *e, EditorKey key)
{
    switch (key) {
        case EDITOR_KEY_SAVE: {
            save_file(e);
        } break;

        default:
            assert(0);
    }
}

static void editor_browsing(Editor *e, EditorKey key)
{
    switch (key) {
        case EDITOR_KEY_LEFT:
        case EDITOR_KEY_UP: {
            editor_move(e, EDITOR_KEY_UP);
        } break;
            
        case EDITOR_KEY_RIGHT:
        case EDITOR_KEY_DOWN: {
            editor_move(e, EDITOR_KEY_DOWN);
        } break;

        case EDITOR_KEY_PAGEUP: {
            // TODO
        } break;

        case EDITOR_KEY_PAGEDOWN: {
            // TODO
        } break;

        case EDITOR_KEY_RETURN: {
            editor_open(e, editor_get_line(e));
        } break;
        
        default:
            assert(0);
    }
}

static_assert(EDITOR_KEY_COUNT == 23, "The number of editor keys has changed");

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
        e->mode = EDITOR_MODE_BROWSING;
    } else if (mode == S_IFREG) { // Regular file
        open_file(e, pathname);
        e->mode = EDITOR_MODE_EDITING;
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

#define sv_c_str(c_chunk, sv_chunk)                     \
    c_chunk = malloc(sv_chunk.count + 1);               \
    memcpy(c_chunk, sv_chunk.data, sv_chunk.count);     \
    c_chunk[sv_chunk.count] = '\0';                       

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
    return strcmp(((Line *) line1)->s, ((Line *) line2)->s);
}










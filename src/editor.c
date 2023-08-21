#define _DEFAULT_SOURCE

#include "editor.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


#define SV_IMPLEMENTATION
#include "sv.h"

// Line Operations
static void remove_line(Editor *e);
static void merge_line(Editor *e);
static void break_line(Editor *e);

// File I/O
static void save_file(Editor *e);
static void open_file(Editor *e);

// Editor Operations
static void editor_edit(Editor *e, EditorKey key);
static void editor_move(Editor *e, EditorKey key);
static void editor_action(Editor *e, EditorKey key);


Editor editor_init(const char *filename)
{
    Editor e;
    e.cx = 0;
    e.cy = 0;
    e.filename = filename;
    
    e.lines = list_init();
    Line line = line_init();
    list_append(&e.lines, &line, sizeof(line));

    struct stat buffer;

    if (e.filename != NULL && stat(filename, &buffer) == 0) {
        open_file(&e);
    }

    return e;
}

void editor_process_key(Editor *e, EditorKey key)
{
    switch (key) {
        case EDITOR_LEFT:
        case EDITOR_RIGHT:
        case EDITOR_UP:
        case EDITOR_DOWN:
        case EDITOR_HOME:
        case EDITOR_END:
        case EDITOR_PAGEUP:
        case EDITOR_PAGEDOWN: {
            editor_move(e, key);
        } break;

        case EDITOR_BACKSPACE:
        case EDITOR_DELETE:
        case EDITOR_RETURN:
        case EDITOR_TAB: {
            editor_edit(e, key);
        } break;

        case EDITOR_SAVE: {
            editor_action(e, key);
        } break;

        default:
            assert(0);
    }
}

void editor_click(Editor *e, size_t x, size_t y)
{
    e->cx = x;
    if (y < e->lines.length) {
        e->cy = y;
    }
}

void editor_insert_text(Editor *e, const char *s)
{
    Line *line = list_get(&e->lines, e->cy);

    if (e->cx > line->size) {
        e->cx = line->size;
    }

    line_insert_text(line, s, e->cx);
    e->cx += strlen(s);
}

void editor_delete_char(Editor *e)
{
    if (e->cx == 0 && e->cy == 0) {
        return;
    }

    editor_move(e, EDITOR_LEFT);

    Line *line = list_get(&e->lines, e->cy);
    if (e->cx == line->size) {
        merge_line(e);
    } else {
        line_delete_char(line, e->cx);
    }
}

size_t editor_get_line_size(const Editor *e)
{
    Line *line = list_get(&e->lines, e->cy);
    return (line == NULL) ? 0: line->size;
}

const char *editor_get_line_at(const Editor *e, size_t at)
{
    const Line *line = list_get(&e->lines, at);
    return (line == NULL) ? NULL: line->s;
}

const char *editor_get_line(const Editor *e)
{
    return editor_get_line_at(e, e->cy);
}

/* Line operations */

static void remove_line(Editor *e)
{
    if (e->cy == e->lines.length) {
        return;
    }
    list_remove(&e->lines, e->cy);
    editor_move(e, EDITOR_UP);
}

static void merge_line(Editor *e)
{
    Line *line_before = list_get(&e->lines, e->cy);
    Line *line_after = list_get(&e->lines, e->cy + 1);
    line_insert_text(line_before, line_after->s, line_before->size);

    editor_move(e, EDITOR_DOWN);
    remove_line(e);
}

static void break_line(Editor *e)
{
    Line *line = list_get(&e->lines, e->cy);
    Line *new_line = list_get(&e->lines, e->cy + 1);

    line_insert_text(new_line, &line->s[e->cx], new_line->size);

    while (e->cx < line->size) {
        line_delete_char(line, e->cx);
    }
}

static void new_line(Editor *e)
{
    Line line = line_init();
    list_insert(&e->lines, &line, sizeof(line), e->cy + 1);
    break_line(e);
    editor_move(e, EDITOR_RIGHT);
}

/* Editor Operations */

static void editor_move(Editor *e, EditorKey key)
{
    size_t line_size = editor_get_line_size(e);
    switch (key) {
        case EDITOR_LEFT: {
            if (e->cx > 0) {
                if (e->cx > line_size) {
                    e->cx = line_size;
                    editor_move(e, EDITOR_LEFT);
                } else {
                    e->cx--;
                }
            } else if (e->cy > 0) {
                editor_move(e, EDITOR_UP);
                editor_move(e, EDITOR_END);
            }
        } break;

        case EDITOR_RIGHT: {
            if (e->cx < line_size) {
                e->cx++;
            } else if (e->cy + 1 < e->lines.length) {
                editor_move(e, EDITOR_DOWN);
                editor_move(e, EDITOR_HOME);
            }
        } break;

        case EDITOR_UP: {
            if (e->cy > 0) {
                e->cy--;
            }
        } break;

        case EDITOR_DOWN: {
            if (e->cy < e->lines.length - 1) {
                e->cy++;
            }
        } break;

        case EDITOR_HOME: {
            e->cx = 0;
        } break;

        case EDITOR_END: {
            e->cx = line_size;
        } break;

        case EDITOR_PAGEUP: {
            // TODO
        } break;

        case EDITOR_PAGEDOWN: {
            // TODO
        } break;

        default:
            assert(0);
    }
}

static void editor_edit(Editor *e, EditorKey key)
{
    switch (key) {
        case EDITOR_BACKSPACE: {
            editor_delete_char(e);
        } break;

        case EDITOR_DELETE: {
            if (e->cy + 1 < e->lines.length ||
                e->cx < editor_get_line_size(e)) 
            {
                editor_move(e, EDITOR_RIGHT);
                editor_delete_char(e);
            }
        } break;

        case EDITOR_RETURN: {
            new_line(e);
        } break;

        case EDITOR_TAB: {
            // TODO
        } break;

        default:
            assert(0);
    }
}

static void editor_action(Editor *e, EditorKey key)
{
    switch (key) {
        case EDITOR_SAVE: {
            save_file(e);
        } break;

        default:
            assert(0);
    }
}

/* File I/O */

static void save_file(Editor *e)
{
    // TODO: change that shoot for a decent prompt
    if (e->filename == NULL) {
        e->filename = "outputtt";
    }

    FILE *file = fopen(e->filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Could not open file `%s`: %s\n", e->filename, strerror(errno));
        exit(1);
    }

    for (size_t i = 0; i < e->lines.length; i++) {
        Line *line = list_get(&e->lines, i);
        fwrite(line->s, 1, line->size, file);
        fputc('\n', file);
    }
    fclose(file);
}

#define sv_c_str(c_chunk, sv_chunk)                         \
    c_chunk = malloc(chunk_line.count + 1);                 \
    memcpy(c_chunk, chunk_line.data, chunk_line.count);     \
    c_chunk[chunk_line.count] = '\0';                       \

static void open_file(Editor *e) 
{   
    FILE *file = fopen(e->filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file `%s`: %s\n", e->filename, strerror(errno));
        exit(1);
    }

    static char chunk[64 * 1024];

    while (!feof(file)) {
        size_t n_bytes = fread(chunk, 1, sizeof(chunk), file);

        String_View chunk_sv = {
            .data = chunk,
            .count = n_bytes
        };
        
        while (chunk_sv.count > 0) {
            String_View chunk_line = {0};
            char *c_chunk;
            
            if (sv_try_chop_by_delim(&chunk_sv, '\n', &chunk_line)) {
                sv_c_str(c_chunk, chunk_line);

                editor_insert_text(e, c_chunk);
                new_line(e);
            } else {
                sv_c_str(c_chunk, chunk_line);
                
                editor_insert_text(e, c_chunk);
                chunk_sv = SV_NULL;
            }

            free(c_chunk);
        }
    }

    e->cy = 0;
}













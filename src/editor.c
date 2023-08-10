#include "editor.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void remove_line(Editor *e);
static void merge_line(Editor *e);
static void break_line(Editor *e);
static void save_file(Editor *e, const char *filename);

Editor editor_init(void)
{
    Editor e;
    e.cx = 0;
    e.cy = 0;
    
    e.lines = list_init();
    Line line = line_init();
    list_append(&e.lines, &line, sizeof(line));

    return e;
}

void editor_edit(Editor *e, EditorEditKeys key)
{
    switch(key) {
        case EDITOR_BACKSPACE: {
            editor_delete_char(e);
        } break;

        case EDITOR_DELETE: {
            if (e->cy + 1 < e->lines.length ||
                e->cx < get_line_length(e)) 
            {
                editor_move(e, EDITOR_RIGHT);
                editor_delete_char(e);
            }
        } break;

        case EDITOR_RETURN: {
            editor_new_line(e);
        } break;

        case EDITOR_TAB: {
            // TODO
        } break;

        case EDITOR_SAVE: {
            save_file(e, "outputtt");
        } break;

        default:
            assert(0);
    }
}

void editor_move(Editor *e, EditorMoveKeys key)
{
    size_t line_size = get_line_length(e);

    switch (key) {
        case EDITOR_LEFT: {
            if (e->cx > 0) {
                if (e->cx > line_size) {
                    e->cx = line_size;
                    editor_move(e, EDITOR_LEFT);
                } else {
                    e->cx--;
                }
            } else {
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
            if (e->cy + 1 < e->lines.length) {
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
    }
}

void editor_insert_char(Editor *e, char c)
{
    assert(e->cy < e->lines.length);

    Line *line = list_get(&e->lines, e->cy);

    if (e->cx > line->size) {
        e->cx = line->size;
    }

    line_insert_char(line, c, e->cx);
    e->cx++;
}

void editor_delete_char(Editor *e)
{
    if (e->cx == 0 && e->cy == 0) {
        return;
    }

    Line *line = list_get(&e->lines, e->cy);
    
    if (e->cx > 0) {
        if (e->cx > line->size) {
            e->cx = line->size;
        }
        e->cx--;
        line_delete_char(line, e->cx);
    } else {
        editor_move(e, EDITOR_LEFT);
        merge_line(e);
    }
}

void editor_new_line(Editor *e)
{
    Line line = line_init();
    list_insert(&e->lines, &line, sizeof(line), e->cy + 1);
    break_line(e);
    editor_move(e, EDITOR_RIGHT);
}

size_t get_line_length(Editor *e)
{
    Line *line = list_get(&e->lines, e->cy);
    return (line == NULL) ? 0: line->size;
}

void save_file(Editor *e, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Could not open file `%s`: %s\n", filename, strerror(errno));
        exit(1);
    }

    for (size_t i = 0; i < e->lines.length; i++) {
        Line *line = list_get(&e->lines, i);
        fwrite(line->s, 1, line->size, file);
        fputc('\n', file);
    }
    fclose(file);
}

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
    char *s = line_after->s;

    for (size_t i = 0; s[i] != '\0'; i++) {
        line_insert_char(line_before, s[i], line_before->size);
    }

    editor_move(e, EDITOR_DOWN);
    remove_line(e);
}

static void break_line(Editor *e)
{
    Line *line = list_get(&e->lines, e->cy);
    Line *new_line = list_get(&e->lines, e->cy + 1);

    while (e->cx < line->size) {
        line_insert_char(new_line, line->s[e->cx], new_line->size);
        line_delete_char(line, e->cx);
    }
}
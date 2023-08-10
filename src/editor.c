#include "editor.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

static void remove_line(Editor *e);
static void merge_line(Editor *e);

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

void editor_move(Editor *e, EditorMoveKey key)
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
            } else {
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
            if (e->cy < e->lines.length) {
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
    assert(e->cy <= e->lines.length);

    if (e->cy == e->lines.length) {
        editor_new_line(e);
    }

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
    } else if (e->cy == e->lines.length) {
        editor_move(e, EDITOR_LEFT);
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
    list_insert(&e->lines, &line, sizeof(line), e->cy);
}

size_t get_line_length(Editor *e)
{
    Line *line = list_get(&e->lines, e->cy);
    return (line == NULL) ? 0: line->size;
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

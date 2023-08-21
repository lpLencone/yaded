#ifndef YADED_EDITOR_H_
#define YADED_EDITOR_H_

#include "ds/list.h"
#include "line.h"

#define line_debug printf("line size: %ld\n", get_line_length(e));
#define editor_debug printf("(%ld, %ld)\n", e->cx, e->cy);

typedef enum {
    EDITOR_LEFT,
    EDITOR_RIGHT,
    EDITOR_UP,
    EDITOR_DOWN,
    EDITOR_HOME,
    EDITOR_END,
    EDITOR_PAGEUP,
    EDITOR_PAGEDOWN,
    EDITOR_BACKSPACE,
    EDITOR_DELETE,
    EDITOR_RETURN,
    EDITOR_TAB,
    EDITOR_SAVE,
} EditorKeys;

typedef struct {
    List lines;
    size_t cx, cy;

    const char *filename;
} Editor;

Editor editor_init(const char *filename);
void editor_insert_text(Editor *e, const char *s);
void editor_delete_char(Editor *e);
void editor_process_key(Editor *e, EditorKeys key);
void editor_click(Editor *e, size_t x, size_t y);

size_t editor_get_line_size(Editor *e);
const char *editor_get_line(Editor *e);
const char *editor_get_line_at(Editor *e, size_t at);

#endif // YADED_EDITOR_H_
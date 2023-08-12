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
} EditorMoveKeys;

typedef enum {
    EDITOR_BACKSPACE,
    EDITOR_DELETE,
    EDITOR_RETURN,
    EDITOR_TAB,
    EDITOR_SAVE,
} EditorEditKeys;

typedef struct {
    List lines;
    size_t cx, cy;
    
    const char *filename;
} Editor;

Editor editor_init(const char *filename);
void editor_insert_text(Editor *e, const char *s);
void editor_delete_char(Editor *e);

void editor_edit(Editor *e, EditorEditKeys key);
void editor_move(Editor *e, EditorMoveKeys);

size_t get_line_length(Editor *e);
void editor_new_line(Editor *e);

#endif // YADED_EDITOR_H_
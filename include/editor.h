#ifndef YADED_EDITOR_H_
#define YADED_EDITOR_H_

#include "ds/list.h"
#include "line.h"

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
} Editor;

Editor editor_init(void);
void editor_insert_char(Editor *e, char c);
void editor_delete_char(Editor *e);
void editor_edit(Editor *e, EditorEditKeys key);
void editor_move(Editor *e, EditorMoveKeys);
size_t get_line_length(Editor *e);
void editor_new_line(Editor *e);

#endif // YADED_EDITOR_H_
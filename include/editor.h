#ifndef YADED_EDITOR_H_
#define YADED_EDITOR_H_

#include "ds/list.h"
#include "line.h"

#define line_debug printf("line size: %ld\n", get_line_length(e));
#define editor_debug printf("(%ld, %ld)\n", e->cx, e->cy);

typedef enum {
    EDITOR_MOVE_KEY_START,
    EDITOR_LEFT = EDITOR_MOVE_KEY_START,
    EDITOR_RIGHT,
    EDITOR_UP,
    EDITOR_DOWN,
    EDITOR_HOME,
    EDITOR_END,
    EDITOR_PAGEUP,
    EDITOR_PAGEDOWN,
    EDITOR_MOVE_KEY_END,
    EDITOR_EDIT_KEY_START = EDITOR_MOVE_KEY_END, // tf is this
    EDITOR_BACKSPACE = EDITOR_EDIT_KEY_START,
    EDITOR_DELETE,
    EDITOR_RETURN,
    EDITOR_TAB,
    EDITOR_EDIT_KEY_END,
    EDITOR_ACTION_KEY_START = EDITOR_EDIT_KEY_END,
    EDITOR_SAVE = EDITOR_ACTION_KEY_START,
    EDITOR_ACTION_KEY_END,
} EditorKey; 

typedef struct {
    List lines;
    size_t cx, cy;

    const char *filename;
} Editor;

Editor editor_init(const char *filename);

void editor_insert_s(Editor *e, const char *s);
void editor_process_key(Editor *e, EditorKey key);

size_t editor_get_line_size(const Editor *e);
const char *editor_get_line(const Editor *e);
const char *editor_get_line_at(const Editor *e, size_t at);

#endif // YADED_EDITOR_H_
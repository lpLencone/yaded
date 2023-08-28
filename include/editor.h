#ifndef YADED_EDITOR_H_
#define YADED_EDITOR_H_

#include "ds/list.h"
#include "line.h"

#define line_debug printf("line size: %ld\n", editor_get_line_size(e));
#define editor_debug printf("(%ld, %ld)\n", e->cx, e->cy);

typedef enum {
    EDITOR_LEFT,
    EDITOR_RIGHT,
    EDITOR_UP,
    EDITOR_DOWN,
    EDITOR_LEFTW,
    EDITOR_RIGHTW,
    EDITOR_LINE_HOME,
    EDITOR_LINE_END,
    EDITOR_HOME,
    EDITOR_END,
    EDITOR_PAGEUP,
    EDITOR_PAGEDOWN,
    EDITOR_BACKSPACE,
    EDITOR_DELETE,
    EDITOR_TAB,
    EDITOR_RETURN,
    EDITOR_LINE_BELOW,
    EDITOR_LINE_ABOVE,
    EDITOR_SAVE,
    EDITOR_KEY_COUNT,
} EditorKey;

typedef struct {
    List lines;
    size_t cx, cy;

    const char *filename;
} Editor;

Editor editor_init(const char *filename);

void editor_insert_s(Editor *e, const char *s);
void editor_process_key(Editor *e, EditorKey key);

void editor_new_line(Editor *e);
void editor_break_line(Editor *e);
void editor_new_line_at(Editor *e, size_t at);

size_t editor_get_line_size(const Editor *e);
const char *editor_get_line(const Editor *e);
const char *editor_get_line_at(const Editor *e, size_t at);

#endif // YADED_EDITOR_H_
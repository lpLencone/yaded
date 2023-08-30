#ifndef YADED_EDITOR_H_
#define YADED_EDITOR_H_

#include "ds/list.h"
#include "line.h"
#include "la.h"

#define line_debug debug_print printf("line size: %ld\n", editor_get_line_size(e));
#define editor_debug debug_print printf("(%ld, %ld)\n", e->cx, e->cy);

typedef enum {
    EDITOR_KEY_LEFT,
    EDITOR_KEY_RIGHT,
    EDITOR_KEY_UP,
    EDITOR_KEY_DOWN,
    EDITOR_KEY_LEFTW,
    EDITOR_KEY_RIGHTW,
    EDITOR_KEY_LINE_HOME,
    EDITOR_KEY_LINE_END,
    EDITOR_KEY_HOME,
    EDITOR_KEY_END,
    EDITOR_KEY_PAGEUP,
    EDITOR_KEY_PAGEDOWN,
    EDITOR_KEY_BACKSPACE,
    EDITOR_KEY_DELETE,
    EDITOR_KEY_TAB,
    EDITOR_KEY_RETURN,
    EDITOR_KEY_LINE_BELOW,
    EDITOR_KEY_LINE_ABOVE,
    EDITOR_KEY_REMOVE_LINE,
    EDITOR_KEY_MERGE_LINE,
    EDITOR_KEY_BREAK_LINE,
    EDITOR_KEY_SAVE,
    EDITOR_KEY_BROWSE,
    EDITOR_KEY_COUNT,
} EditorKey;

typedef enum {
    EDITOR_MODE_EDITING,
    EDITOR_MODE_BROWSING,
    EDITOR_MODE_SELECTION,
} EditorMode;

typedef struct {
    List lines;
    size_t cx, cy;
    Vec2f cs; // Cursor selection

    EditorMode mode;
    List pathname;
} Editor;

Editor editor_init(const char *pathname);
void editor_clear(Editor *e);

void editor_process_key(Editor *e, EditorKey key);

void editor_write(Editor *e, const char *s);

#define editor_new_line(e, s) editor_new_line_at(e, s, (e)->cy)
#define editor_remove_line(e) editor_remove_line_at(e, (e)->cy)
#define editor_merge_line(e) editor_merge_line_at(e, (e)->cy)
#define editor_break_line(e) editor_break_line_at(e, (e)->cy)
void editor_new_line_at(Editor *e, const char *s, size_t at);
void editor_remove_line_at(Editor *e, size_t at);
void editor_merge_line_at(Editor *e, size_t at);
void editor_break_line_at(Editor *e, size_t at);

size_t editor_get_line_size(const Editor *e);
const char *editor_get_line(const Editor *e);
const char *editor_get_line_at(const Editor *e, size_t at);

void editor_move(Editor *e, EditorKey key);
void editor_edit(Editor *e, EditorKey key);
void editor_action(Editor *e, EditorKey key);

void editor_open(Editor *e, const char *pathname);

#endif // YADED_EDITOR_H_
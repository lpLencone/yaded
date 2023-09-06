#ifndef YADED_EDITOR_H_
#define YADED_EDITOR_H_

#include "ds/list.h"
#include "line.h"
#include "la.h"

#define line_debug debug_print printf("line size: %ld\n", editor_get_line_size(e));
#define editor_debug debug_print printf("(%u, %u)\n", e->c.x, e->c.y);

typedef enum {
    EK_LEFT,
    EK_RIGHT,
    EK_UP,
    EK_DOWN,
    EK_LEFTW,
    EK_RIGHTW,
    EK_LINE_HOME,
    EK_LINE_END,
    EK_HOME,
    EK_END,
    EK_PAGEUP,
    EK_PAGEDOWN,
    EK_NEXT_EMPTY_LINE,
    EK_PREV_EMPTY_LINE,
    EK_BACKSPACE,
    // EK_BACKSAPCEW,
    EK_DELETE,
    // EK_DELETEW
    EK_TAB,
    EK_RETURN,
    EK_LINE_BELOW,
    EK_LINE_ABOVE,
    EK_REMOVE_LINE,
    EK_MERGE_LINE,
    EK_BREAK_LINE,
    EK_SELECT_LEFT,
    EK_SELECT_LEFTW,
    EK_SELECT_RIGHT,
    EK_SELECT_RIGHTW,
    EK_SELECT_UP,
    EK_SELECT_DOWN,
    EK_SELECT_LINE_HOME,
    EK_SELECT_LINE_END,
    EK_SELECT_WORD,
    EK_SELECT_LINE,
    EK_SELECT_OUTER_BLOCK,
    EK_SELECT_NEXT_BLOCK,
    EK_SELECT_HOME,
    EK_SELECT_END,
    EK_SELECT_NEXT_EMPTY_LINE,
    EK_SELECT_PREV_EMPTY_LINE,
    EK_SELECT_ALL,
    EK_SAVE,
    EK_COPY,
    EK_PASTE,
    EK_CUT,
    EK_COUNT,
} EditorKey;

typedef enum {
    EM_EDITING,
    EM_SELECTION,
    EM_SELECTION_RESOLUTION,
    EM_BROWSING,
} EditorMode;

typedef struct {
    List lines;
    Vec2ui c; // Cursor
    Vec2ui cs; // Selection cursor

    char *clipboard;

    EditorMode mode;
    List pathname;
} Editor;

Editor editor_init(const char *pathname);
void editor_clear(Editor *e);

void editor_process_key(Editor *e, EditorKey key);

void editor_write(Editor *e, const char *s);

#define editor_new_line(e, s) editor_new_line_at(e, s, (e)->c.y)
#define editor_remove_line(e) editor_remove_line_at(e, (e)->c.y)
#define editor_merge_line(e) editor_merge_line_at(e, (e)->c.y)
#define editor_break_line(e) editor_break_line_at(e, (e)->c.y)
void editor_new_line_at(Editor *e, const char *s, size_t at);
void editor_remove_line_at(Editor *e, size_t at);
void editor_merge_line_at(Editor *e, size_t at);
void editor_break_line_at(Editor *e, size_t at);

size_t editor_get_line_size(const Editor *e);
#define editor_get_line(e) editor_get_line_at(e, (e)->c.y)
const char *editor_get_line_at(const Editor *e, size_t at);

char *editor_retrieve_selection(const Editor *e);

void editor_open(Editor *e, const char *pathname);

#endif // YADED_EDITOR_H_
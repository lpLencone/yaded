#ifndef MEDO_EDITOR_H_
#define MEDO_EDITOR_H_

#include "ds/list.h"
#include "ds/string_builder.h"
#include "line.h"
#include "la.h"

#define EDITOR_
#include "be/basic_editor.h"

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
    EK_BACKSPACEW,
    EK_DELETE,
    EK_DELETEW,
    EK_TAB,
    EK_INDENT,
    EK_UNINDENT,
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
    EK_SEARCH_START,
    EK_SEARCH_NEXT,
    EK_SEARCH_PREV,
    EK_COPY,
    EK_PASTE,
    EK_CUT,
    EK_ESC,
    EK_COUNT,
} EditorKey;

typedef enum {
    EM_EDITING,
    EM_SELECTION,
    EM_SEARCHING,
    EM_BROWSING,
} EditorMode;

typedef struct {
    List lines;
    Vec2ui c; // Cursor
    Vec2ui cs; // Selection cursor

    size_t select_cur;

    Basic_Editor be;

    char *clipboard;

    char searchbuf[64];
    Vec2i match;

    EditorMode mode;
    
    da_var(pathname, char *);
} Editor;

Editor editor_init(const char *pathname);
void editor_clear(Editor *e);

void editor_process_key(Editor *e, EditorKey key);
Vec2ui editor_move(Editor *e, EditorKey key, Vec2ui pos);
Vec2ui editor_edit(Editor *e, EditorKey key, Vec2ui pos);

size_t editor_write_at(Editor *e, const char *s, size_t at);

#define editor_new_line(e, s) editor_new_line_at(e, s, (e)->c.y)
#define editor_remove_line(e) editor_remove_line_at(e, (e)->c.y)
#define editor_merge_line(e) editor_merge_line_at(e, (e)->c.y)
#define editor_break_line(e) editor_break_line_at(e, (e)->c)
void editor_new_line_at(Editor *e, const char *s, size_t at);
void editor_remove_line_at(Editor *e, size_t at);

#define editor_get_line(e) editor_get_line_at(e, (e)->c.y)
const char *editor_get_line_at(const Editor *e, size_t at);
size_t editor_get_line_size(const Editor *e);

char *editor_retrieve_selection(const Editor *e);

void editor_open(Editor *e, const char *pathname);

void editor_selection_start(Editor *e, Vec2ui pos);
void editor_selection_stop(Editor *e);

#endif // MEDO_EDITOR_H_



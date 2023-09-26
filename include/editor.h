#ifndef MEDO_EDITOR_H_
#define MEDO_EDITOR_H_

#include "ds/string_builder.h"
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
    EK_LINE_HOME, // change line_home to home and home to file_home
    EK_LINE_END,
    EK_HOME,
    EK_END,
    EK_PAGEUP,
    EK_PAGEDOWN,
    EK_NEXT_PARAGRAPH,
    EK_PREV_PARAGRAPH,
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
    EK_SELECT_NEXT_PARAGRAPH,
    EK_SELECT_PREV_PARAGRAPH,
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
    size_t select_cur;

    Basic_Editor be;

    char *clipboard;

    char searchbuf[64];
    int match;

    EditorMode mode;
    
    String_Builder pathname;
} Editor;

Editor editor_init(void);
void editor_clear(Editor *e);

void editor_process_key(Editor *e, EditorKey key);
size_t editor_move(Editor *e, EditorKey key, size_t cur);
size_t editor_edit(Editor *e, EditorKey key, size_t cur);

size_t editor_write_at(Editor *e, const char *s, size_t at);

void editor_open(Editor *e, const char *path, size_t pathlen);

#endif // MEDO_EDITOR_H_



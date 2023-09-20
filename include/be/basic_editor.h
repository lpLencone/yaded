#ifndef EDITOR_H_
#define EDITOR_H_

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%15s : %4d : %-20s ", __FILE__, __LINE__, __func__);
#endif
#ifndef debug_println
#define debug_println debug_print puts("");
#endif

#include "ds/dynamic_array.h"
#include "be/common.h"
#include "simple_renderer.h"
#include "lexer.h"

#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

typedef struct {
    size_t begin;
    size_t end;
} Line_;

da_Type(Lines, Line_);

da_Type(Tokens, Token);

typedef struct {
    String_Builder data;
    Lines lines;
    Tokens tokens;
    String_Builder file_path;

    bool selection;
    size_t cursor;

    String_Builder clipboard;
} Basic_Editor;

Errno be_save_as(Basic_Editor *be, const char *file_path);
Errno be_save(const Basic_Editor *be);
Errno be_load_from_file(Basic_Editor *be, const char *file_path);

void be_backspace(Basic_Editor *be);
void be_delete(Basic_Editor *be);
size_t be_cursor_row(const Basic_Editor *be, size_t cur);

void be_move_line_up(Basic_Editor *be);
void be_move_line_down(Basic_Editor *be);
void be_move_line_start(Basic_Editor *be);
void be_move_line_end(Basic_Editor *be);
void be_move_char_left(Basic_Editor *be);
void be_move_char_right(Basic_Editor *be);
void be_move_word_left(Basic_Editor *be);
void be_move_word_right(Basic_Editor *be);

Line_ be_get_line_at_(const Basic_Editor *be, size_t cur);

#define be_insert_s(be, s) be_insert_sn(be, s, strlen(s))
#define be_insert_sn(be, s, n) be_insert_sn_at(be, s, n, (be)->cursor)
#define be_insert_s_at(be, s, at) be_insert_sn_at(be, s, strlen(s), at)
size_t be_insert_sn_at(Basic_Editor *be, const char *s, size_t n, size_t at);

void be_delete_sn_from(Basic_Editor *be, size_t n, size_t from);

void be_clipboard_copy(Basic_Editor *be);
void be_clipboard_paste(Basic_Editor *be);


#endif // EDITOR_H_

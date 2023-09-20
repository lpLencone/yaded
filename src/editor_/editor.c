#include "be/basic_editor.h"
#include "lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static void be_recompute_lines(Basic_Editor *be);

void be_backspace(Basic_Editor *be)
{
    if (be->cursor > be->data.size) {
        be->cursor = be->data.size;
    }
    if (be->cursor == 0) return;
    be->cursor--;

    da_remove_from(&be->data, be->cursor);
    be_recompute_lines(be);
}

void be_delete(Basic_Editor *be)
{
    if (be->cursor >= be->data.size) return;
    da_remove_from(&be->data, be->cursor);

    be_recompute_lines(be);
}

Errno be_save_as(Basic_Editor *be, const char *file_path)
{
    printf("Saving as %s...\n", file_path);
    Errno err = write_entire_file(file_path, be->data.data, be->data.size);
    if (err != 0) return err;
    be->file_path.size = 0;
    sb_append_cstr(&be->file_path, file_path);
    sb_append_null(&be->file_path);
    return 0;
}

Errno be_save(const Basic_Editor *be)
{
    assert(be->file_path.size > 0);
    printf("Saving as %s...\n", be->file_path.data);
    return write_entire_file(be->file_path.data, be->data.data, be->data.size);
}

Errno be_load_from_file(Basic_Editor *be, const char *file_path)
{
    printf("Loading %s\n", file_path);

    be->data.size = 0;
    Errno err = read_entire_file(file_path, &be->data);
    if (err != 0) return err;

    be->cursor = 0;

    be_recompute_lines(be);

    be->file_path.size = 0;
    sb_append_cstr(&be->file_path, file_path);
    sb_append_null(&be->file_path);

    return 0;
}

size_t be_cursor_row(const Basic_Editor *be, size_t cur)
{
    assert(be->lines.size > 0);
    for (size_t row = 0; row < be->lines.size; row++) {
        Line_ line = be->lines.data[row];
        if (line.begin <= cur && cur <= line.end) {
            return row;
        }
    }
    return be->lines.size - 1;
}

void be_move_line_up(Basic_Editor *be)
{
    size_t row = be_cursor_row(be, be->cursor);
    size_t col = be->cursor - be->lines.data[row].begin;

    if (row > 0) {
        Line_ next_line = be->lines.data[row - 1];
        size_t next_line_size = next_line.end - next_line.begin;
        if (col > next_line_size) col = next_line_size;
        be->cursor = next_line.begin + col;
    }
}

void be_move_line_down(Basic_Editor *be)
{
    size_t row = be_cursor_row(be, be->cursor);
    size_t col = be->cursor - be->lines.data[row].begin;
    if (row + 1 < be->lines.size) {
        Line_ next_line = be->lines.data[row + 1];
        size_t next_line_size = next_line.end - next_line.begin;
        if (col > next_line_size) col = next_line_size;
        be->cursor = next_line.begin + col;
    }
}

void be_move_char_left(Basic_Editor *be)
{
    if (be->cursor > 0) be->cursor -= 1;
}

void be_move_char_right(Basic_Editor *be)
{
    if (be->cursor < be->data.size) be->cursor += 1;
}

#define issymbol(c) (isalnum(c) || c == '_')
void be_move_word_left(Basic_Editor *be)
{
    if (be->cursor > 0) {
        be->cursor--;
    }
    
    if (be->cursor > 0 && !issymbol(be->data.data[be->cursor])) {
        while (be->cursor > 0 && !issymbol(be->data.data[be->cursor])) {
            be->cursor--;
        }
    } else {
        while (be->cursor > 0 && issymbol(be->data.data[be->cursor])) {
            be->cursor--;
        }
    }

    if (be->cursor > 0) {
        be->cursor++; // maybe should check against be->data.size
    }
}

void be_move_word_right(Basic_Editor *be)
{
    if (be->cursor < be->data.size) {
        be->cursor++;
    }
    
    if (be->cursor < be->data.size && !issymbol(be->data.data[be->cursor])) {
        while (be->cursor < be->data.size && !issymbol(be->data.data[be->cursor])) {
            be->cursor++;
        }
    } else {
        while (be->cursor < be->data.size && issymbol(be->data.data[be->cursor])) {
            be->cursor++;
        }
    }
}
#undef issymbol

void be_move_line_start(Basic_Editor *be)
{
    Line_ line = be_get_line_at_(be, be->cursor);
    be->cursor = line.begin;
}

void be_move_line_end(Basic_Editor *be)
{
    size_t row = be_cursor_row(be, be->cursor);
    Line_ line = be->lines.data[row];
    be->cursor = line.end;
}

size_t be_insert_sn_at(Basic_Editor *be, const char *s, size_t n, size_t at)
{
    if (at > be->data.size) {
        at = be->data.size;
    }

    da_insert_n(&be->data, s, n, at);

    be_recompute_lines(be);

    at += n;
    return at;
}

void be_delete_sn_from(Basic_Editor *be, size_t n, size_t from)
{
    da_remove_n_from(&be->data, n, from);
    be_recompute_lines(be);
}

static void be_recompute_lines(Basic_Editor *be)
{
    // Lines
    {
        be->lines.size = 0;

        Line_ line;
        line.begin = 0;

        for (size_t i = 0; i < be->data.size; i++) {
            if (be->data.data[i] == '\n') {
                line.end = i;
                da_append(&be->lines, &line);
                line.begin = i + 1;
            }
        }

        line.end = be->data.size;
        da_append(&be->lines, &line);
    }

    // Syntax Highlighting
    {
        da_clear(&be->tokens);
        Lexer l = lexer_init(be->data.data, be->data.size, NULL);
        Token t = lexer_next(&l);
        while (t.kind != TOKEN_END) {
            da_append(&be->tokens, &t);
            t = lexer_next(&l);
        }
    }
}

Line_ be_get_line_at_(const Basic_Editor *be, size_t cur)
{
    assert(cur < be->data.size);
    return be->lines.data[be_cursor_row(be, cur)];
}

#if 0
void be_clipboard_copy(Basic_Editor *be)
{
    if (be->selection) {
        size_t begin = be->select_begin;
        size_t end = be->cursor;
        if (begin > end) SWAP(size_t, begin, end);

        be->clipboard.size = 0;
        sb_append_n(&be->clipboard, &be->data.data[begin], end - begin + 1);
        sb_append_null(&be->clipboard);

        if (SDL_SetClipboardText(be->clipboard.data) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }
    }
}
#endif

void be_clipboard_paste(Basic_Editor *be)
{
    char *text = SDL_GetClipboardText();
    size_t text_len = strlen(text);
    if (text_len > 0) {
        be_insert_sn(be, text, text_len);
    } else {
        fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
    }
    SDL_free(text);
}


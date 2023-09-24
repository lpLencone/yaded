#include "be/basic_editor.h"
#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void be_recompute_lines(Basic_Editor *be);

void be_backspace(Basic_Editor *be)
{
    if (be->cur > be->data.size) {
        be->cur = be->data.size;
    }
    if (be->cur == 0) return;
    be->cur--;

    da_remove_from(&be->data, be->cur);
    be_recompute_lines(be);
}

void be_delete(Basic_Editor *be)
{
    if (be->cur >= be->data.size) return;
    da_remove_from(&be->data, be->cur);
    be_recompute_lines(be);
}

Errno be_save(const Basic_Editor *be)
{
    assert(be->filename.size > 0);
    printf("Saving as %s...\n", be->filename.data);
    return write_entire_file(be->filename.data, be->data.data, be->data.size);
}

Errno be_load_from_file(Basic_Editor *be, const char *filename)
{
    printf("Loading %s\n", filename);

    be->data.size = 0;
    Errno err = read_entire_file(filename, &be->data);
    if (err != 0) return err;

    be->cur = 0;

    be_recompute_lines(be);

    be->filename.size = 0;
    sb_append_cstr(&be->filename, filename);
    sb_append_null(&be->filename);

    return 0;
}

size_t be_cursor_row(const Basic_Editor *be, size_t cur)
{
    assert(be->lines.size > 0);
    for (size_t row = 0; row < be->lines.size; row++) {
        Line_ line = be->lines.data[row];
        if (line.home <= cur && cur <= line.end) {
            return row;
        }
    }
    return be->lines.size - 1;
}

size_t be_move_up(Basic_Editor *be, size_t cur)
{
    size_t row = be_cursor_row(be, cur);
    size_t col = cur - be->lines.data[row].home;

    if (row > 0) {
        Line_ next_line = be->lines.data[row - 1];
        size_t next_line_size = next_line.end - next_line.home;
        if (col > next_line_size) col = next_line_size;
        cur = next_line.home + col;
    }
    return cur;
}

size_t be_move_down(Basic_Editor *be, size_t cur)
{
    size_t row = be_cursor_row(be, cur);
    size_t col = cur - be->lines.data[row].home;
    if (row + 1 < be->lines.size) {
        Line_ next_line = be->lines.data[row + 1];
        size_t next_line_size = next_line.end - next_line.home;
        if (col > next_line_size) col = next_line_size;
        cur = next_line.home + col;
    }
    return cur;
}

#define issymbol(c) (isalnum(c) || c == '_')
size_t be_move_leftw(Basic_Editor *be, size_t cur)
{
    if (cur > 0) {
        cur--;
    }
    
    if (cur > 0 && !issymbol(be->data.data[cur])) {
        while (cur > 0 && !issymbol(be->data.data[cur])) {
            cur--;
        }
        if (cur > 0 && issymbol(be->data.data[cur])) {
            cur++; // maybe should check against be->data.size
        }
    } else {
        while (cur > 0 && issymbol(be->data.data[cur])) {
            cur--;
        }
        if (cur > 0 && !issymbol(be->data.data[cur])) {
            cur++; // maybe should check against be->data.size
        }
    }

    return cur;
}

size_t be_move_rightw(Basic_Editor *be, size_t cur)
{
    if (cur < be->data.size) {
        cur++;
    }
    
    if (cur < be->data.size && !issymbol(be->data.data[cur])) {
        while (cur < be->data.size && !issymbol(be->data.data[cur])) {
            cur++;
        }
    } else {
        while (cur < be->data.size && issymbol(be->data.data[cur])) {
            cur++;
        }
    }
    return cur;
}
#undef issymbol

size_t be_move_home(Basic_Editor *be, size_t cur)
{
    Line_ line = be_get_line_at_(be, cur);
    cur = line.home;
    return cur;
}

size_t be_move_end(Basic_Editor *be, size_t cur)
{
    size_t row = be_cursor_row(be, cur);
    Line_ line = be->lines.data[row];
    cur = line.end;
    return cur;
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

size_t be_delete_sn_from(Basic_Editor *be, size_t n, size_t from)
{
    da_remove_n_from(&be->data, n, from);
    be_recompute_lines(be);
    return from - n;
}

static void be_recompute_lines(Basic_Editor *be)
{
    be->lines.size = 0;
    Line_ line;
    line.home = 0;
    for (size_t i = 0; i < be->data.size; i++) {
        if (be->data.data[i] == '\n') {
            line.end = i;
            da_append(&be->lines, &line);
            line.home = i + 1;
        }
    }
    line.end = be->data.size;
    da_append(&be->lines, &line);
}

Line_ be_get_line_at_(const Basic_Editor *be, size_t cur)
{
    assert(cur < be->data.size);
    return be->lines.data[be_cursor_row(be, cur)];
}


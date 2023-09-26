#include "be/basic_editor.h"
#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void be_recompute_lines(Basic_Editor *be);

Errno be_load_from_file(Basic_Editor *be, const char *filename)
{
    be->data.size = 0;
    Errno err = read_entire_file(filename, &be->data);
    if (err != 0) return err;

    be->cur = 0;
    be_recompute_lines(be);

    return 0;
}

// Get

Line be_get_line(const Basic_Editor *be, size_t cur)
{
    assert(cur < be->data.size);
    return be->lines.data[be_cursor_row(be, cur)];
}

size_t be_cursor_row(const Basic_Editor *be, size_t cur)
{
    assert(be->lines.size > 0);
    for (size_t row = 0; row < be->lines.size; row++) {
        Line line = be->lines.data[row];
        if (line.home <= cur && cur <= line.end) {
            return row;
        }
    }
    return be->lines.size - 1;
}

// Move

size_t be_move_up(Basic_Editor *be, size_t cur)
{
    size_t row = be_cursor_row(be, cur);
    size_t col = cur - be->lines.data[row].home;

    if (row > 0) {
        Line next_line = be->lines.data[row - 1];
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
        Line next_line = be->lines.data[row + 1];
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
    Line line = be_get_line(be, cur);
    cur = line.home;
    return cur;
}

size_t be_move_end(Basic_Editor *be, size_t cur)
{
    size_t row = be_cursor_row(be, cur);
    Line line = be->lines.data[row];
    cur = line.end;
    return cur;
}

size_t be_move_next_paragraph(Basic_Editor *be, size_t cur)
{
    size_t row = 0;
    Line line = {0};
    do {
        cur = be_move_down(be, cur);
        row = be_cursor_row(be, cur);
        line = be->lines.data[row];
    } while (row < be->lines.size && line.home != line.end);
    return cur;
}

size_t be_move_prev_paragraph(Basic_Editor *be, size_t cur)
{
    size_t row = 0;
    Line line = {0};
    do {
        cur = be_move_up(be, cur);
        row = be_cursor_row(be, cur);
        line = be->lines.data[row];
    } while (row > 0 && line.home != line.end);
    return cur;
}

// Manipulation

size_t be_backspace(Basic_Editor *be)
{
    if (be->cur > 0) {
        be_delete_n_from(be, 1, --be->cur);
    }
    return be->cur;
}

void be_delete(Basic_Editor *be)
{
    if (be->cur >= be->data.size) return;
    be_delete_n_from(be, 1, be->cur);
}

size_t be_insert_sn_at(Basic_Editor *be, const char *s, size_t n, size_t at)
{
    if (at > be->data.size) {
        at = be->data.size;
    }
    da_insert_n(&be->data, s, n, at);
    be_recompute_lines(be);
    return at + n;
}

void be_delete_n_from(Basic_Editor *be, size_t n, size_t from)
{
    da_remove_n_from(&be->data, n, from);
    be_recompute_lines(be);
}

size_t be_insert_line_above(Basic_Editor *be, size_t cur)
{
    Line line = be_get_line(be, cur);
    be_insert_sn_at(be, "\n", 1, line.home);
    return line.home;
}

size_t be_insert_line_below(Basic_Editor *be, size_t cur)
{
    Line line = be_get_line(be, cur);
    return be_insert_sn_at(be, "\n", 1, line.end);
}

// Maintenance

static void be_recompute_lines(Basic_Editor *be)
{
    be->lines.size = 0;
    Line line;
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

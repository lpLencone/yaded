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
#include "ds/string_builder.h"
#include "be/common.h"
#include "simple_renderer.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t home;
    size_t end;
} Line;

da_Type(Lines, Line);

typedef struct {
    String_Builder data;
    Lines lines;

    bool selection;
    size_t cur;
} Basic_Editor;

void be_load_from_file(Basic_Editor *be, const char *filename);
void be_destroy(Basic_Editor *be);

size_t be_cursor_row(const Basic_Editor *be, size_t cur);
Line be_get_line(const Basic_Editor *be, size_t cur);
// TODO: move n
size_t be_move_up(Basic_Editor *be, size_t cur);
size_t be_move_down(Basic_Editor *be, size_t cur);
size_t be_move_home(Basic_Editor *be, size_t cur);
size_t be_move_end(Basic_Editor *be, size_t cur);
size_t be_move_leftw(Basic_Editor *be, size_t cur);
size_t be_move_rightw(Basic_Editor *be, size_t cur);
size_t be_move_next_paragraph(Basic_Editor *be, size_t cur);
size_t be_move_prev_paragraph(Basic_Editor *be, size_t cur);

// Manipulation
#define be_insert_s(be, s) be_insert_sn(be, s, strlen(s))
#define be_insert_sn(be, s, n) be_insert_sn_at(be, s, n, (be)->cur)
#define be_insert_s_at(be, s, at) be_insert_sn_at(be, s, strlen(s), at)
size_t be_backspace(Basic_Editor *be);
void be_delete(Basic_Editor *be);
size_t be_insert_sn_at(Basic_Editor *be, const char *s, size_t n, size_t at);
void be_delete_n_from(Basic_Editor *be, size_t n, size_t from);
size_t be_insert_line_above(Basic_Editor *be, size_t cur);
size_t be_insert_line_below(Basic_Editor *be, size_t cur);


#endif // EDITOR_H_

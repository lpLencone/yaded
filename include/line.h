#ifndef YADED_LINE_H_
#define YADED_LINE_H_

#include <stddef.h>

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%20s : %4d : %-20s\n", __FILE__, __LINE__, __func__);
#endif

typedef struct {
    size_t capacity;
    size_t size;
    char *s;
} Line;

#define line_init(s) line_init_n(s, strlen(s))
Line line_init_n(const char *s, size_t n);

void line_destroy(Line *line);
void line_clear(Line *line);

#define line_write(line, s, at) line_write_n(line, s, strlen(s), at)
void line_write_n(Line *line, const char *s, size_t n, size_t at);

#define line_append(line, s) line_append_n(line, s, strlen(s))
#define line_append_n(line, s, n) line_write_n(line, s, strlen(s))

void line_delete_char(Line *line, size_t at);

#endif // YADED_LINE_H_
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

Line line_init(void);

void line_insert_s(Line *line, const char *s, size_t at);
void line_delete_char(Line *line, size_t at);

#endif // YADED_LINE_H_
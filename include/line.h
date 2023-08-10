#ifndef YADED_LINE_H_
#define YADED_LINE_H_

#include <stddef.h>

typedef struct {
    size_t capacity;
    size_t size;
    char *s;
} Line;

Line line_init(void);
void line_insert_char(Line *line, char c, size_t at);
void line_delete_char(Line *line, size_t at);

#endif // YADED_LINE_H_
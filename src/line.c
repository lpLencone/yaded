#define _DEFAULT_SOURCE

#include "line.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

Line line_init_n(const char *s, size_t n)
{
    assert(s != NULL);

    Line line;
    da_init(&line, s, n + 1);
    line.data[n] = '\0';

    return line;
}

void line_destroy(Line *line)
{
    da_end(line);
}

void line_clear(Line *line)
{
    da_clear(line);
}

void line_write_n(Line *line, const char *s, size_t n, size_t at)
{
#if 0
    assert(at <= line->size);

    while (line->capacity < line->size + n + 1) {
        line->data = realloc(line->data, line->capacity * 2);
        line->capacity *= 2;
    }

    memmove(line->data + at + n, line->data + at, line->size - at);
    memcpy(line->data + at, s, n);
    
    line->size += n;
    printf("%zu\n", line->size);
    line->data[line->size] = '\0';
#else
    da_insert_n(line, s, n, at);
#endif
}

void line_append_n(Line *line, const char *s, size_t n)
{
    line_write_n(line, s, n, strlen(s));
}

void line_delete_char(Line *line, size_t at)
{
    assert(at < line->size);

    if (line->size == 0) {
        return;
    }

    memmove(line->data + at, line->data + at + 1, line->size + 1 - at);
    line->size--;

    if (line->size + 1 < line->capacity / 2) {
        line->data = realloc(line->data, line->capacity / 2);
        line->capacity /= 2;
    }
}

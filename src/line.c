#define _DEFAULT_SOURCE

#include "line.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

Line line_init_n(const char *s, size_t n)
{
    assert(s != NULL);

    Line line;
    line.size = n;
    line.capacity = line.size + 1;
    line.s = strndup(s, n);
    line.s[n] = '\0';

    
    return line;
}

void line_destroy(Line *line)
{
    assert(line != NULL && line->s != NULL);

    free(line->s);
    line->s = NULL;

}

void line_clear(Line *line)
{
    assert(line != NULL && line->s != NULL);

    line->s[0] = '\0';
    line->size = 0;
    line->capacity = 1;

}

void line_write_n(Line *line, const char *s, size_t n, size_t at)
{
    assert(at <= line->size);

    while (line->capacity < line->size + n + 1) {
        line->s = realloc(line->s, line->capacity * 2);
        line->capacity *= 2;
    }

    memmove(line->s + at + n, line->s + at, line->size - at);
    memcpy(line->s + at, s, n);
    
    line->size += n;

    line->s[line->size] = '\0';

}

void line_delete_char(Line *line, size_t at)
{
    assert(at < line->size);

    if (line->size == 0) {
        return;
    }

    memmove(line->s + at, line->s + at + 1, line->size + 1 - at);
    line->size--;

    if (line->size + 1 < line->capacity / 2) {
        line->s = realloc(line->s, line->capacity / 2);
        line->capacity /= 2;
    }

}

#define _DEFAULT_SOURCE

#include "line.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

Line line_init(const char *s)
{
    assert(s != NULL);

    Line line;
    line.s = strdup(s);
    line.size = strlen(s);
    line.capacity = line.size + 1;
    
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

void line_write(Line *line, const char *s, size_t at)
{
    assert(at <= line->size);
    
    size_t slen = strlen(s);

    while (line->capacity < line->size + slen + 1) {
        line->s = realloc(line->s, line->capacity * 2);
        line->capacity *= 2;
    }

    memmove(line->s + at + slen, line->s + at, line->size - at);
    memcpy(line->s + at, s, slen);
    
    line->size += slen;
    line->s[line->size] = '\0';
}

void line_append(Line *line, const char *s)
{
    line_write(line, s, strlen(s));
}

void line_delete_char(Line *line, size_t at)
{
    assert(at < line->size);

    if (line->size == 0) {
        return;
    }

    memmove(line->s + at, line->s + at + 1, line->size + 1 - at);
    line->size--;

    if (line->size - 1 < line->capacity / 2) {
        line->s = realloc(line->s, line->capacity / 2);
        line->capacity /= 2;
    }
}

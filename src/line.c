#include "line.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define LINE_INIT_CAPACITY 2

Line line_init(void)
{
    Line line;

    line.capacity = LINE_INIT_CAPACITY;
    line.size = 0;
    line.s = malloc(LINE_INIT_CAPACITY);
    line.s[0] = '\0';

    return line;
}

void line_insert_text(Line *line, const char *s, size_t at)
{
    size_t slen = strlen(s);

    while (line->capacity < line->size + slen + 2) {
        line->s = realloc(line->s, line->capacity * 2);
        line->capacity *= 2;
    }

    memmove(line->s + at + slen, line->s + at, line->size - at);
    memcpy(line->s + at, s, slen);
    
    line->size += slen;
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

    if (line->size - 1 < line->capacity / 4) {
        line->s = realloc(line->s, line->capacity / 2);
        line->capacity /= 2;
    }
}

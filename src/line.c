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

void line_insert_char(Line *line, char c, size_t at)
{
    assert(at <= line->size + 1);

    if (line->capacity < line->size + 2) {
        line->s = realloc(line->s, line->size * 2);
        line->capacity *= 2;
    }

    memmove(line->s + at + 1, line->s + at, line->size - at);
    line->s[at] = c;
    
    line->size++;
    line->s[line->size] = '\0';
}

// Delete char at `at`
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

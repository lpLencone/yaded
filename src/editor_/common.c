#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "be/common.h"

static Errno file_size(FILE *file, size_t *size)
{
    long saved = ftell(file);
    if (saved < 0) return errno;
    if (fseek(file, 0, SEEK_END) < 0) return errno;
    long result = ftell(file);
    if (result < 0) return errno;
    if (fseek(file, saved, SEEK_SET) < 0) return errno;
    *size = (size_t) result;
    return 0;
}

Errno read_entire_file(const char *filename, String_Builder *sb)
{
    Errno result = 0;
    FILE *f = NULL;

    f = fopen(filename, "r");
    if (f == NULL) return_defer(errno);

    size_t size;
    Errno err = file_size(f, &size);
    if (err != 0) return_defer(err);

    if (sb->capacity < size) {
        sb->capacity = size;
        sb->data = realloc(sb->data, sb->capacity*sizeof(*sb->data));
        assert(sb->data != NULL && "Buy more RAM lol");
    }

    fread(sb->data, size, 1, f);
    if (ferror(f)) return_defer(errno);
    sb->size = size;

defer:
    if (f) fclose(f);
    return result;
}

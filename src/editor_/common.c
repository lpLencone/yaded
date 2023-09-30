#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "be/common.h"

#define FILESIZE(fp, size) \
    fseek(fp, 0L, SEEK_END); \
    *size = ftell(fp);       \
    rewind(fp);

char *read_entire_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror("fopen");
        exit(1);
    }

    size_t size;
    FILESIZE(f, &size);
    char *data = malloc(size);
    assert(data != NULL);

    fread(data, size, 1, f);
    if (ferror(f)) {
        perror("fread");
        exit(1);
    }
    fclose(f);
    
    return data;
}

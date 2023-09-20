#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdio.h>

#include "ds/string_builder.h"

typedef int Errno;

#define SWAP(T, a, b) do { T t = a; a = b; b = t; } while (0)

#define return_defer(value) do { result = (value); goto defer; } while (0)

#define DA_INIT_CAP 256

char *temp_strdup(const char *s);
void temp_reset(void);

typedef struct {
    const char **data;
    size_t size;
    size_t capacity;
} Files;

Errno read_entire_file(const char *file_path, String_Builder *sb);
Errno write_entire_file(const char *file_path, const char *buf, size_t buf_size);
Errno read_entire_dir(const char *dir_path, Files *files);

#endif // COMMON_H_

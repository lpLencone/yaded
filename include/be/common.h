#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdio.h>

#include "ds/string_builder.h"

typedef int Errno;

#define SWAP(T, a, b) do { T t = a; a = b; b = t; } while (0)

#define return_defer(value) do { result = (value); goto defer; } while (0)

Errno read_entire_file(const char *file_path, String_Builder *sb);

#endif // COMMON_H_

#ifndef MEDO_COMMON_H_
#define MEDO_COMMON_H_

#include <stdlib.h>
#include <stdio.h>

#define SWAP(T, a, b) do { T t = a; a = b; b = t; } while (0)

char *read_entire_file(const char *filename);

#endif // MEDO_COMMON_H_

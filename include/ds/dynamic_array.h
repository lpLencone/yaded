#ifndef MEDO_DS_DYNAMIC_ARRAY_H_
#define MEDO_DS_DYNAMIC_ARRAY_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%15s : %4d : %-20s ", __FILE__, __LINE__, __func__);
#endif
#ifndef debug_println
#define debug_println debug_print puts("");
#endif

#ifndef DA_INIT_CAPACITY
#  define DA_INIT_CAPACITY  1
#endif // DA_INIT_CAPACITY

#define TYPESIZE(da) sizeof(*(da)->data)

#define da_var(da, t) \
    struct { \
        size_t size; \
        size_t capacity; \
        t *data; \
    } da

#define da_Type(da, t) \
    typedef da_var(da, t)

#define da_zero(da) \
    do { \
        (da)->size = 0; \
        (da)->capacity = 0; \
        (da)->data = NULL; \
    } while (0)

#define da_init(da, d, n) \
    do { \
        (da)->size = n; \
        (da)->capacity = n; \
        (da)->data = malloc(n * TYPESIZE(da)); \
        memcpy((da)->data, d, n * TYPESIZE(da)); \
    } while (0) \

#define da_var_zero(da, t) \
    da_var(da, t); \
    da_zero(&da)

#define da_var_init(da, t, d, n) \
    da_var(da, t); \
    da_init(&da, d, n)

#define da_end(da) \
    do { \
        assert(da != NULL && (da)->data != NULL); \
        free((da)->data); \
        (da)->data = NULL; \
        (da)->size = 0; \
        (da)->capacity = 0; \
    } while (0)


#define da_insert_n(da, d, n, at) \
    do { \
        assert((da) != NULL && d != NULL); \
        assert(at <= (da)->size); \
 \
        if ((da)->capacity == 0) { \
            (da)->capacity = DA_INIT_CAPACITY; \
            (da)->data = calloc(DA_INIT_CAPACITY, TYPESIZE(da)); \
        } \
 \
        while ((da)->capacity < (da)->size + n) { \
            (da)->capacity *= 2; \
            (da)->data = realloc((da)->data, (da)->capacity * TYPESIZE(da)); \
            assert((da)->data != NULL); \
        } \
 \
        memmove( \
            (da)->data + (at + n), \
            (da)->data + at, \
            ((da)->size - at) * TYPESIZE(da) \
        ); \
        memcpy((da)->data + at, d, (n * TYPESIZE(da))); \
 \
        (da)->size += n; \
    } while (0)

#define da_insert(da, d, at) da_insert_n(da, d, 1, at)

#define da_append_n(da, d, n) da_insert_n(da, d, n, (da)->size)
#define da_append(da, d) da_append_n(da, d, 1)

#define da_remove_n(da, from, n) \
    do { \
        assert(from + n < (da)->size + 1);  \
 \
        if (from + n == (da)->size) { \
            memset((da)->data + from, 0, n * TYPESIZE(da)); \
        } else { \
            memmove( \
                (da)->data + from,  \
                (da)->data + (from + n),  \
                ((da)->size + 1 - n) * TYPESIZE(da) \
            ); \
        } \
        (da)->size -= n; \
 \
        if ((da)->size < (da)->capacity / 2) { \
            (da)->data = realloc((da)->data, (da)->capacity / 2); \
            assert((da)->data != NULL); \
            (da)->capacity /= 2; \
        } \
    } while (0)
#define da_remove_at(da, at) da_remove_n(da, at, 1)

#define da_get_copy_n(da, copybuf, from, n) \
    do { \
        copybuf = calloc(n, TYPESIZE(da)); \
        memcpy(copybuf, &((da)->data[from]), n * TYPESIZE(da)); \
    } while (0)
    
#define da_get_copy(da, copybuf) da_get_copy_n(da, copybuf, 0, (da)->size)

#define da_peek(da, peekp, from) \
    do { \
        peekp = &(da)->data[from]; \
    } while (0)

#endif // MEDO_DS_DYNAMIC_ARRAY_H_

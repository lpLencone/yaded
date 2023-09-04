#ifndef YADED_DS_DYNAMIC_ARRAY_H_
#define YADED_DS_DYNAMIC_ARRAY_H_

#define da_make(da, t) \
    struct { \
        size_t tsize; \
        size_t size; \
        size_t capacity; \
        t *data; \
    } da

#define da_init(da, t) \
    do { \
        (da)->tsize = sizeof(t); \
        (da)->size = 0; \
        (da)->capacity = 2; \
        (da)->data = calloc(2, (da)->tsize); \
    } while (0)

#define da_create(da, t) \
    struct { \
        size_t tsize; \
        size_t size; \
        size_t capacity; \
        t *data; \
    } da = { \
        .tsize = sizeof(t), \
        .size = 0, \
        .capacity = 2, \
        .data = calloc(2, sizeof(t)) \
    }

#define da_end(da) \
    do { \
        free((da)->data); \
        (da)->data = NULL; \
    } while (0)

#define da_clear(da) \
    do { \
        if ((da)->capacity != 0) { \
            (da)->size = 0; \
            (da)->capacity = 1; \
            (da)->data = realloc((da)->data, (da)->capacity * (da)->tsize); \
            memset((da)->data, 0, 1); \
        } \
    } while (0)

#define da_insert_n(da, d, n, at) \
    do { \
        assert((da) != NULL && d != NULL); \
        assert(at <= (da)->size); \
 \
        while ((da)->capacity < (da)->size + n + 1) { \
            (da)->capacity *= 2; \
            (da)->data = realloc((da)->data, (da)->capacity * (da)->tsize); \
            assert((da)->data != NULL); \
        } \
 \
        memmove( \
            (da)->data + (at + n), \
            (da)->data + at, \
            ((da)->size - at) * (da)->tsize \
        ); \
        memcpy((da)->data + at, d, (n * (da)->tsize)); \
 \
        (da)->size += n; \
    } while (0)

#define da_insert(da, d, at) da_insert_n(da, d, 1, at)

#define da_append_n(da, d, n) da_insert_n(da, d, n, (da)->size)
#define da_append(da, d) da_append_n(da, d, 1)

#define da_remove_n(da, from, n) \
    do { \
        assert(0 < from && from + n <= (da)->size);  \
 \
        memmove( \
            (da)->data + (da)->tsize * (from + n),  \
            (da)->data + (da)->tsize * from,  \
            (da)->size - (da)->tsize * n \
        ); \
        (da)->size -= n; \
 \
        if ((da)->size < (da)->capacity / 2) { \
            (da)->data = realloc((da)->data, (da)->capacity / 2); \
            (da)->capacity /= 2; \
        } \
    } while (0)
#define da_remove(da, at) da_remove(da, at, 1)

#define da_get_copy_n(da, copybuf, from, n) \
    do { \
        copybuf = calloc(from + n, (da)->tsize); \
        memcpy(copybuf, &((da)->data[from]), n * (da)->tsize); \
    } while (0)
#define da_get_copy(da, copybuf) da_get_copy_n(da, copybuf, 0, (da)->size)

#define da_peek(da, peekp, from) \
    do { \
        peekp = &da->data[from] \
    } while (0)

#endif // YADED_DS_DYNAMIC_ARRAY_H_
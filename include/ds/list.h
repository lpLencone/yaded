#ifndef YADED_DS_LIST_H_
#define YADED_DS_LIST_H_

#include "ds/node.h"

typedef void (*DeallocFunc)(void *data);
typedef int  (*CompareFunc)(void *data1, void *data2);

typedef struct {
    Node *head;
    size_t length;
    DeallocFunc dealloc;
    CompareFunc compare;
} List;

List list_init(DeallocFunc dealloc, CompareFunc compare);

void list_clear(List *list);
void list_insert(List *list, const void *data, size_t size, size_t at);
void list_append(List *list, const void *data, size_t size);
void list_swap(List *list, size_t at1, size_t at2);
void list_remove(List *list, size_t at);
void list_quicksort(List *list);

void *list_get(const List *list, size_t at);

#endif // YADED_DS_LIST_H_


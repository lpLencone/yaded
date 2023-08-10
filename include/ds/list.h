#ifndef YADED_DS_LIST_H_
#define YADED_DS_LIST_H_

#include "ds/node.h"

typedef struct {
    Node *head;
    size_t length;
} List;

List list_init(void);
void list_end(List *list);

void list_insert(List *list, const void *data, size_t size, size_t at);
void list_append(List *list, const void *data, size_t size);
void list_swap(List *list, size_t at1, size_t at2);
void list_remove(List *list, size_t at);

void *list_get(List *list, size_t at);

#endif // YADED_DS_LIST_H_

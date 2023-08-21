#include "ds/list.h"

#include <assert.h>

static Node *get_node_at(const List *list, size_t at);

List list_init(void)
{
    List list;
    list.head = NULL;
    list.length = 0;
    return list;
}

void list_destroy(List *list)
{
    while (list->length > 0) {
        list_remove(list, 0);
    }
}

void list_insert(List *list, const void *data, size_t size, size_t at)
{
    assert(list != NULL && at <= list->length);

    Node *node = node_init(data, size);

    if (at == 0) {
        node->next = list->head;
        list->head = node;
    }
    else {
        Node *cursor = list->head;
        for (size_t i = 0; i < at - 1; i++, cursor = cursor->next);

        node->next = cursor->next;
        cursor->next = node;
    }

    list->length++;
}

void list_append(List *list, const void *data, size_t size)
{
    list_insert(list, data, size, list->length);
}

void list_swap(List *list, size_t at1, size_t at2)
{
    assert(list != NULL && at1 < list->length && at2 < list->length);

    Node *cursor = list->head;
    Node *n1, *n2;

    size_t at = (at1 > at2) ? at1 : at2;
    for (size_t i = 0; i <= at; i++) {
        if (i == at1) {
            n1 = cursor;
        }
        if (i == at2) {
            n2 = cursor;
        }
    }

    void *temp = n1->data;
    n1->data = n2->data;
    n2->data = temp;    
}

void list_remove(List *list, size_t at)
{
    assert(list != NULL && at < list->length);
    

    if (at == 0) {
        Node *temp = list->head;
        list->head = temp->next;
        node_end(temp);
    } else {
        Node *cursor = list->head;
        for (size_t i = 0; i < at - 1; i++) {
            cursor = cursor->next;
        }

        Node *temp = cursor->next;
        cursor->next = cursor->next->next;
        node_end(temp);
    }

    list->length--;
}

void *list_get(const List *list, size_t at)
{
    if (at >= list->length) {
        return NULL;
    }
    Node *node = get_node_at(list, at);
    return node->data;
}


static Node *get_node_at(const List *list, size_t at)
{
    Node *cursor = list->head;
    for (size_t i = 0; i < at; i++, cursor = cursor->next);
    return cursor;
}

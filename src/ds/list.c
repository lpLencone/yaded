#include "ds/list.h"

#include <assert.h>

static Node *get_node_at(const List *list, size_t at);
static void node_swap(Node *a, Node *b);
static void quicksort_recursive(Node *start, Node *end, CompareFunc compare);

List list_init(DeallocFunc dealloc, CompareFunc compare)
{
    List list;
    list.head = NULL;
    list.length = 0;
    list.dealloc = dealloc;
    list.compare = compare;
    return list;
}

void list_clear(List *list)
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

    node_swap(n1, n2); 
}

void list_remove(List *list, size_t at)
{
    assert(list != NULL && at < list->length);

    if (at == 0) {
        Node *temp = list->head;
        list->head = temp->next;

        if (list->dealloc) list->dealloc(temp->data);
        node_end(temp);
    } else {
        Node *cursor = list->head;
        for (size_t i = 0; i < at - 1; i++) {
            cursor = cursor->next;
        }
        Node *temp = cursor->next;
        cursor->next = temp->next;

        if (list->dealloc) list->dealloc(temp->data);
        node_end(temp);
    }

    list->length--;
}

void *list_get(const List *list, size_t at)
{
    assert(at < list->length);
    
    Node *node = get_node_at(list, at);
    return node->data;
}

void list_quicksort(List *list)
{
    if (list->length > 0) {
        Node *tail = get_node_at(list, list->length - 1);
        quicksort_recursive(list->head, tail, list->compare);
    }
}


static Node *get_node_at(const List *list, size_t at)
{
    Node *cursor = list->head;
    for (size_t i = 0; i < at; i++, cursor = cursor->next);
    return cursor;
}

static void node_swap(Node *a, Node *b)
{
    void *temp = a->data;
    a->data = b->data;
    b->data = temp;
}

static Node *partition(Node *start, Node *end, CompareFunc compare)
{
    Node *pivot = start;
    Node *cursor_i = start;
    Node *cursor_j = start;

    while (cursor_j != NULL && cursor_j != end) {
        if (compare(cursor_j->data, end->data) < 0) {
            pivot = cursor_i;
            node_swap(cursor_i, cursor_j);
            cursor_i = cursor_i->next;
        }
        cursor_j = cursor_j->next;
    }
    node_swap(cursor_i, end);
    return pivot;
}

static void quicksort_recursive(Node *start, Node *end, CompareFunc compare)
{
    if (start == end) {
        return;
    }
    Node *pivot = partition(start, end, compare);
    if (pivot != NULL && start != pivot) {
        quicksort_recursive(start, pivot, compare);
    }
    if (pivot != NULL && pivot->next != NULL) {
        quicksort_recursive(pivot->next, end, compare);
    }
}
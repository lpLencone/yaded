#ifndef YADED_DS_NODE_H_
#define YADED_DS_NODE_H_

#include <stdio.h>
#define debug_print printf("%20s : %4d : %-20s\n", __FILE__, __LINE__, __func__);

#include <stddef.h>

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

Node   *node_init(const void *data, size_t size);
void    node_end(Node *node);

#endif // YADED_DS_NODE_H_
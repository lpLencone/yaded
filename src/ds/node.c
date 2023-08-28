#include "ds/node.h"

#include <stdlib.h>
#include <string.h>

Node *node_init(const void *data, size_t size)
{
    Node *node = malloc(sizeof(Node));
    node->data = malloc(size);
    memcpy(node->data, data, size);
    node->next = NULL;
    return node;
}

void node_end(Node *node)
{
    free(node->data);
    free(node);
}

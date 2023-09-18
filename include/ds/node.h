#ifndef MEDO_DS_NODE_H_
#define MEDO_DS_NODE_H_

#ifndef debug_print
#include <stdio.h>
#define debug_print printf("%15s : %4d : %-20s ", __FILE__, __LINE__, __func__);
#endif
#ifndef debug_println
#define debug_println debug_print puts("");
#endif

#include <stddef.h>

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

Node   *node_init(const void *data, size_t size);
void    node_end(Node *node);

#endif // MEDO_DS_NODE_H_
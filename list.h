#ifndef LIST_H
#define LIST_H
struct tlb;

typedef struct node {
    struct node *prev;
    struct node *next;
    struct tcb *t; // the tlb
} node;

void insert_before(node *n, node *new);
void insert_after(node *n, node *new);
void remove_node(node *n);
#endif /* !LIST_H */

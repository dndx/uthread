#include "list.h"

// a simple linked list implementation

void insert_before(node *n, node *new)
{
    new->prev = n->prev;
    new->next = n;
    if (n->prev)
    {
        n->prev->next = new;
    }
    n->prev = new;
}

void insert_after(node *n, node *new)
{
    new->prev = n;
    new->next = n->next;
    if (n->next)
    {
        n->next->prev = new;
    }
    n->next = new;
}

void remove_node(node *n)
{
    if (n->prev)
    {
        n->prev->next = n->next;
    }
    if (n->next)
    {
        n->next->prev = n->prev;
    }
}

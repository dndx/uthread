#ifndef UTHREAD_H
#define UTHREAD_H
#define _GNU_SOURCE
#include <ucontext.h>
#include "list.h"

typedef struct tcb {
    ucontext_t con;
    void *stack;
    unsigned long start_cpu_time;
    // cpu time on this kernel thread before user thread started to run, in microseconds
    unsigned long cpu_time; // total cpu time used so far, in microseconds
    int terminated;
} tcb; // thread control block

void system_init(int max_number_of_klt);
int uthread_create(void (*func)());
void uthread_yield(void);
void uthread_exit(void);

#endif /* !UTHREAD_H */

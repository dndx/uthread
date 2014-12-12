#include "uthread.h"
#include <pthread.h>
#include <stdlib.h>
#include <sched.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/syscall.h>

#define gettid() syscall(SYS_gettid)

#define STACK_SIZE 8388608 // default thread stack size
typedef struct rcb {
    int tid;
    ucontext_t con; // currently running coroutine
    tcb *t;
} rcb; // runner control block

static int max_kernel_th;
static node *all_head, *ready_head;
// all_head points to the head node of the "all process" linked list
// ready_head points to the head node of the "ready process" linked list
static pthread_mutexattr_t mutex_shared_attr;
static pthread_condattr_t cond_shared_attr;
static pthread_mutex_t mutex; // global mutex
static pthread_cond_t ready_queue_changed;
static rcb *runner_control; // runner contexts for all runners

static int runner(void *);
static unsigned long timeval_to_msec(struct timeval *t); // helper method to convert a struct timeval to integer msec

void system_init(int max_number_of_klt)
{
    max_kernel_th = max_number_of_klt > 1 ? max_number_of_klt : 1; // in case input is malformed, we just use 1 runner
#ifndef NDEBUG
    printf("requested kernel threads: %d\n", max_kernel_th);
#endif

    // initialize shared mutex and conditional variable
    if (pthread_mutexattr_init(&mutex_shared_attr))
    {
        perror("pthread_mutexattr_init()");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutexattr_setpshared(&mutex_shared_attr, PTHREAD_PROCESS_SHARED))
    {
        perror("pthread_mutexattr_setpshared()");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&mutex, &mutex_shared_attr))
    {
        perror("pthread_mutex_init()");
        exit(EXIT_FAILURE);
    }

    if (pthread_condattr_init(&cond_shared_attr))
    {
        perror("pthread_condattr_init()");
        exit(EXIT_FAILURE);
    }
    if (pthread_condattr_setpshared(&cond_shared_attr, PTHREAD_PROCESS_SHARED))
    {
        perror("pthread_condattr_setpshared()");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&ready_queue_changed, &cond_shared_attr))
    {
        perror("pthread_cond_init()");
        exit(EXIT_FAILURE);
    }

    runner_control = malloc(sizeof(rcb) * max_kernel_th); // allocate rcbs according to runner requested

    for (long i=0; i<max_kernel_th; i++)
    {
        void *stack = malloc(STACK_SIZE); // allocate stacks for each runner, this is only used by the uthread library and not the user thread
        if (!stack)
        {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
        if (getcontext(&runner_control[i].con) == -1) // initialize runner context
        {
            perror("system_init()->getcontext()");
            exit(EXIT_FAILURE);
        }
        runner_control[i].con.uc_link = NULL; // initialize runner context
        runner_control[i].con.uc_stack.ss_sp = stack + STACK_SIZE - 1;
        runner_control[i].con.uc_stack.ss_size = STACK_SIZE;
        runner_control[i].tid = clone(runner, runner_control[i].con.uc_stack.ss_sp, CLONE_VM|CLONE_FILES|CLONE_SIGHAND, (void *) i); // kick off runner
        if (runner_control[i].tid == -1)
        {
            perror("clone()");
            exit(EXIT_FAILURE);
        }
    }        
}

int uthread_create(void (*func)(void))
{
    tcb *t = calloc(1, sizeof(tcb)); // allocate TCB for user thread
    if (!t)
    {
        return -1; // unable to allocate memory
    }

    t->stack = malloc(STACK_SIZE); // allocate stack for the thread
    if (!t->stack)
    {
        return -1;
    }
    if (getcontext(&t->con) == -1) // initialize the user thread context
    {
        perror("uthread_create()->getcontext()");
        return -1;
    }
    t->con.uc_link = NULL;
    t->con.uc_stack.ss_sp = t->stack + STACK_SIZE - 1;
    t->con.uc_stack.ss_size = STACK_SIZE;
    makecontext(&t->con, func, 0); // makecontext() ensures the user thread starts from func once scheduled to run on a kernel thread

    node *an = malloc(sizeof(node)); // node for the all process linked list
    node *rn = malloc(sizeof(node)); // node for the ready process linked list
    if (!an || !rn)
    {
        return -1; // memory allocation failure
    }
    an->t = rn->t = t; // both nodes pointing to the TCB

    pthread_mutex_lock(&mutex); // manipulating shared variable
    if (!all_head)
    {
        all_head = an;
        an->next = NULL;
        an->prev = NULL;
    }
    else
    {
        insert_before(all_head, an);
        all_head = an;
    }

    if (!ready_head)
    {
        ready_head = rn;
        rn->next = NULL;
        rn->prev = NULL;
    }
    else
    {
        insert_before(ready_head, rn);
        ready_head = rn;
    }
    pthread_cond_signal(&ready_queue_changed); // kick off idle runners, if any
    pthread_mutex_unlock(&mutex);

    return 0;
}

void uthread_yield(void)
{
    /*
     * here, to find out the corresponding RCB the kernel thread belongs to, we walk through
     * the runner_control array and find the RCB with the same tid as current kernel thread.
     * This is an O(n) operation, but I don't see any more efficient way than this.
     */
    int tid = gettid();
    for (int i=0; i<max_kernel_th; i++)
    {
        if (runner_control[i].tid == tid)
        {
#ifndef NDEBUG
    printf("found rcb: %d\n", i);
#endif
            if (swapcontext(&runner_control[i].t->con, &runner_control[i].con) == -1) // save the current context and return to the runner function
            {
                perror("swapcontext()");
            }
        }
    }
}

static int runner(void *id)
{
    long rid = (long) id; // here, we are "piggybacked" the runner id into the pointer, so that no heap mamory was needed.
#ifndef NDEBUG
    printf("runner id is: %lu\n", rid);
#endif
    while (1) // the runner loop
    {
        pthread_mutex_lock(&mutex);
        if (ready_head) // if there are anything in the ready queue
        {
            tcb *t = ready_head->t;
            node *old_head = ready_head;
            ready_head = old_head->next;
            remove_node(old_head); // dequeue
            pthread_mutex_unlock(&mutex); // no need to hold the lock anymore

            struct rusage usage;
            if (getrusage(RUSAGE_THREAD, &usage) == -1) // find the resource usage so far
            {
                perror("getrusage()");
                return -1;
            }
            t->start_cpu_time = timeval_to_msec(&usage.ru_utime) + timeval_to_msec(&usage.ru_stime); // record the resource usage before swapping into user thread
            runner_control[rid].t = t; // keep track of who is running right now
            if (swapcontext(&runner_control[rid].con, &t->con) == -1) // save current context and swap into the user thread
            {
                perror("swapcontext()");
            }
#ifndef NDEBUG
           printf("runner is back!\n");
#endif
            if (getrusage(RUSAGE_THREAD, &usage) == -1) // get resource usage again
            {
                perror("getrusage()");
                return -1;
            }
            t->cpu_time += timeval_to_msec(&usage.ru_utime) + timeval_to_msec(&usage.ru_stime) - t->start_cpu_time; // calculate the CPU time

            pthread_mutex_lock(&mutex); // manipulating on global data
            if (!t->terminated) // user thread didn't call uthread_exit()
            {
#ifndef NDEBUG
                printf("requeueing thread\n");
#endif
                if (!ready_head)
                {
                    ready_head = old_head;
                    ready_head->next = ready_head->prev = NULL;
                }
                else
                {
                    node *n = ready_head;
                    int updated = 0;
                    while (1) // find the location to insert thread according to CPU usage
                    {
                        if (t->cpu_time < n->t->cpu_time)
                        {
                            insert_before(n, old_head);
                            if (ready_head == n)
                            {
                                ready_head = old_head;
                            }
                            updated = 1;
                            break;
                        }

                        if (n->next)
                        {
                            n = n->next;
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (!updated) // used more CPU than everything else, n is the tail node of the ready queue
                    {
                        insert_after(n, old_head);
                    }
                }
            }
#ifndef NDEBUG
            else
            {
                printf("process terminated, finding something else..\n");
                free(t->stack);
                free(t); // no memory leak please
            }
#endif
        }
        else // ready queue is empty
        {
#ifndef NDEBUG
            printf("nothing to do, waiting on the cond variable..\n");
#endif
            pthread_cond_wait(&ready_queue_changed, &mutex);
        }
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}

void uthread_exit(void) // simply marks corresponding TCB's terminated field as 1 and runner will take care of it
{
    int tid = gettid();
    for (int i=0; i<max_kernel_th; i++)
    {
        if (runner_control[i].tid == tid)
        {
            runner_control[i].t->terminated = 1;
        }
    }
    uthread_yield(); // trap back to the runner
}

static unsigned long timeval_to_msec(struct timeval *t)
{
    return t->tv_sec * 1000000 + t->tv_usec;
}

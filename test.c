#include "uthread.h" //the header file containing the definitions of data structures and functions for the uthread library
#include <math.h>
#include <semaphore.h>
#include <stdio.h>

int n_threads=1;
int myid=0;
sem_t mutex;

void do_something()
{
        int id,i,j;

        sem_wait(&mutex);

        id=myid;
        myid++;
        printf("This is ult %d\n", id);

        if(n_threads<6){
                n_threads++;
                sem_post(&mutex);

                uthread_create(do_something);
        }else sem_post(&mutex);

        if(id%2==0)
                for(j=0;j<100;j++)
                        for(i=0;i<1000000;i++) sqrt(i*119.89);

        printf("This is ult %d again\n", id);

        uthread_yield();

        printf("This is ult %d once more\n", id);

        if(id%2==0)
                for(j=0;j<100;j++)
                        for(i=0;i<1000000;i++) sqrt(i*119.89);

        printf("This is ult %d exit\n", id);

        uthread_exit();
}

main()
{
        int i;
        system_init(1);
        setbuf(stdout,NULL);
        sem_init(&mutex,0,1);
        uthread_create(do_something);
        uthread_exit();
}


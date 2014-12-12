
ComS 352 Fall 2014 Project 1 (Due: Sunday, November 2, 11:59pm)

Development of uthread: a User-level Thread Library

The purpose of this project is to deepen your understadning of the basic OS concepts of kernel threads, user-level threads, running context of threads, context switch, thread scheduling and synchronization, as well as to familize you with C/C++ system programming.   

1. uthread library functions

In this project, you are required to develop a library of APIs for user-level thread programming. Specifically, the library should expose the following functions:

(1) void system_init(int max_number_of_klt)

This function is called before any other uthread library functions can be called. You may use it to initialize the uthread system (for example, data structures). Parameter max_num_of_klt specifies the maximum number of kernel threads that can be used to run the user-level threads created by this library. Argument max_number_of_klt should be no less than 1. 
 
(2) int uthread_create(void (*func)())

This function creates a new user-level thread which runs func(). It returns 0 if succeeds, or -1 otherwise (if the library is unable to create the thread due to certain reason such as lack of resource). 

(3) void uthread_yield()

The calling thread requests to yield the kernel thread that it is currently running on to one of other user threads which has run for the shortest period of time. If no any other thread has shorter running time than this calling thread, the calling thread should proceed to run on the kernel thread. 
   
(4) void uthread_exit()

The calling user-level thread ends its execution. You can assume that user application code always call this function before a user thread ends. 


2. Scheduling of user-level threads in uthread library

Recall that user-level threads should be mapped (scheduled) to kernel threads to run on the CPU(s). The uthread library should implement "many to many mapping" method to map user threads to up to max_number_of_klt kernel threads. 

The uthread library should implement a customized non-preemptive, cooperative, fair algorithm when scheduling user threads to the kernel thread, which is described as follows: when a kernel thread becomes available (for example, because yielding or ending of a user thread) and several user threads are ready to run, the thread that has run on the CPU for the smallest amount of time should be picked to run on the kernel thread. If there are multiple such ready threads of the same running time, the FCFS algorithm is used to break the tie. 

You may use getusage(int, struct usage *) system call to measure the time a user thread has run. Note that, the measurement of CPU time consumed by a thread may not be very accurate; you can assume each user thread will run a pretty long time (e.g., go through a heavy loop) before each calling of yield.  


3. Other Instructions 

Submit your library code together with a readme file to describe how to integrate your library and application code, including how to compile. If your library source code are in multiple files, compressed the files into a single ZIP file and submit. 

Your code should compile successfully in pyrite, the platform used for grading. Otherwise, you may receive no points.  

Your code should avoid memory leakage. Failure to handle such problems may cause loss of up to 10% of your grade. 

Write comments in your code to help debugging, testing and grading. The readability of your code will contribute to up to 10% of your grade if the code runs correctly; in the case that your code is not completely right, good readability will allow the TA to identify (and evaluate) the problematic portion of your code and hence be able to give partial credits.  

Well test your code before submission.

If submitted late, the same late policy as weekly assignments will be applied.

The TA may contact you for code debugging and evaluation (in the case that you code does not run correctly). So be alert on emails after you submit your code. 

Start as early as possible!
    


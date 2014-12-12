uthread
=============

Note
=============
See `project-1-2014f.txt` for original project specification.

**This project is not production quality and isn't as efficient as it should be,
use at your own risk!**

Use
=============
To use uthread, simply include the header file "uthread.h" in your application code,
use the provided system_init(), uthread_create(), uthread_yield() and uthread_exit()
as described in the project specification.

Compile
=============
To compile uthread, you need to compile two object files list.o and uthread.o and link
them with your application. Additionally, you will need to link libmath and libpthread
while generating the final executable, for example, if your code lives in test.c:

$ gcc -Wall -std=gnu11 -O0 -g -DNDEBUG -c uthread.c
$ gcc -Wall -std=gnu11 -O0 -g -DNDEBUG -c list.c
$ gcc -Wall -std=gnu11 -O0 -g -DNDEBUG -c test.c
$ gcc -Wall -std=gnu11 -O0 -g -DNDEBUG -o test uthread.o list.o test.o -lpthread -lm

Please note that -DNDEBUG is always required, otherwise uthread will be outputting verbose
debug information.

A sample Makefile is provided with the submission, to compile the test code posted on
Blackboard, simply type:

$ make
$ ./test

To clean the directory:

$ make clean

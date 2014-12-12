CFLAGS=-Wall -std=gnu11 -O0 -g -DNDEBUG
all: test

test: test.o uthread.o list.o
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lm

test1.o: test1.c
	$(CC) -c $(CFLAGS) $<

test.o: test.c
	$(CC) -c $(CFLAGS) $<

uthread.o: uthread.c uthread.h
	$(CC) -c $(CFLAGS) $<

list.o: list.c list.h
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f test *.o

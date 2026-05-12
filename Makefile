CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Iinclude -g
SRCS    = src/scheduler.c src/mutex.c src/semaphore.c src/cond.c
OBJS    = $(SRCS:.c=.o)
LIB     = libadaptos.a

.PHONY: all clean test

all: $(LIB) demo/producer_consumer demo/philosophers

$(LIB): $(OBJS)
	ar rcs $@ $^

demo/producer_consumer: demo/producer_consumer.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -ladaptos

demo/philosophers: demo/philosophers.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -ladaptos

test: $(LIB)
	$(CC) $(CFLAGS) -o tests/run_tests tests/posix_compat.c -L. -ladaptos
	./tests/run_tests

clean:
	rm -f $(OBJS) $(LIB) demo/producer_consumer demo/philosophers tests/run_tests

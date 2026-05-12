/**
 * producer_consumer.c — Classic bounded-buffer demo using AdaptOS.
 *
 * 3 producer threads and 2 consumer threads share a ring buffer of size 8.
 * Synchronised with a mutex + two condition variables (not_full, not_empty).
 */

#include <stdio.h>
#include <stdlib.h>
#include "adaptOS.h"

#define BUFFER_SIZE  8
#define N_PRODUCERS  3
#define N_CONSUMERS  2
#define ITEMS_EACH   10

static int     buffer[BUFFER_SIZE];
static int     head = 0, tail = 0, count = 0;
static amutex_t lock;
static acond_t  not_full;
static acond_t  not_empty;

static void *producer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < ITEMS_EACH; i++) {
        int item = id * 100 + i;
        amutex_lock(&lock);
        while (count == BUFFER_SIZE)
            acond_wait(&not_full, &lock);
        buffer[tail] = item;
        tail = (tail + 1) % BUFFER_SIZE;
        count++;
        printf("[Producer %d] produced %d  (buf=%d)\n", id, item, count);
        acond_signal(&not_empty);
        amutex_unlock(&lock);
    }
    return NULL;
}

static void *consumer(void *arg) {
    int id = *(int *)arg;
    int total = (N_PRODUCERS * ITEMS_EACH) / N_CONSUMERS;
    for (int i = 0; i < total; i++) {
        amutex_lock(&lock);
        while (count == 0)
            acond_wait(&not_empty, &lock);
        int item = buffer[head];
        head = (head + 1) % BUFFER_SIZE;
        count--;
        printf("[Consumer %d] consumed %d  (buf=%d)\n", id, item, count);
        acond_signal(&not_full);
        amutex_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    amutex_init(&lock);
    acond_init(&not_full);
    acond_init(&not_empty);

    athread_t ptids[N_PRODUCERS], ctids[N_CONSUMERS];
    int pids[N_PRODUCERS], cids[N_CONSUMERS];

    for (int i = 0; i < N_PRODUCERS; i++) {
        pids[i] = i + 1;
        athread_create(&ptids[i], producer, &pids[i]);
    }
    for (int i = 0; i < N_CONSUMERS; i++) {
        cids[i] = i + 1;
        athread_create(&ctids[i], consumer, &cids[i]);
    }
    for (int i = 0; i < N_PRODUCERS; i++) athread_join(ptids[i], NULL);
    for (int i = 0; i < N_CONSUMERS; i++) athread_join(ctids[i], NULL);

    printf("All done.\n");
    amutex_destroy(&lock);
    acond_destroy(&not_full);
    acond_destroy(&not_empty);
    return 0;
}

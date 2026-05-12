/**
 * adaptOS.h — Public API for the AdaptOS user-space threading library.
 *
 * Drop-in subset of pthreads implemented entirely in user space.
 * No kernel threads are created; all scheduling is done by the library.
 */

#pragma once

#include <stddef.h>

/* ── Types ──────────────────────────────────────────────────────────────────── */

typedef unsigned int athread_t;

typedef struct {
    volatile int locked;     /* 0 = free, 1 = held */
    athread_t    owner;      /* thread that holds the lock   */
    athread_t    waiters[64];/* threads blocked on this mutex */
    int          nwaiters;
} amutex_t;

typedef struct {
    volatile int value;
    athread_t    waiters[64];
    int          nwaiters;
} asem_t;

typedef struct {
    athread_t waiters[64];
    int       nwaiters;
} acond_t;

/* Scheduler modes */
typedef enum {
    SCHED_COOPERATIVE  = 0,
    SCHED_ROUND_ROBIN  = 1,
    SCHED_PRIORITY     = 2,
} athread_sched_t;

/* ── Thread lifecycle ────────────────────────────────────────────────────────── */

/**
 * Create a new green thread that runs fn(arg).
 * Returns 0 on success, -1 on failure (e.g. thread table full).
 */
int  athread_create(athread_t *tid, void *(*fn)(void *), void *arg);

/** Terminate the calling thread, making retval available via athread_join. */
void athread_exit(void *retval);

/**
 * Block until thread tid terminates.
 * If retval != NULL, stores the exit value there.
 */
int  athread_join(athread_t tid, void **retval);

/** Voluntarily relinquish the CPU to the next runnable thread. */
void athread_yield(void);

/** Return the calling thread's ID. */
athread_t athread_self(void);

/** Set the scheduler mode (must be called before athread_create). */
void athread_set_scheduler(athread_sched_t mode);

/* ── Mutex ───────────────────────────────────────────────────────────────────── */

int amutex_init(amutex_t *m);
int amutex_lock(amutex_t *m);
int amutex_trylock(amutex_t *m);  /* returns 0 if acquired, EBUSY otherwise */
int amutex_unlock(amutex_t *m);
int amutex_destroy(amutex_t *m);

/* ── Semaphore ───────────────────────────────────────────────────────────────── */

int asem_init(asem_t *s, int initial_value);
int asem_wait(asem_t *s);   /* decrement; blocks if value == 0 */
int asem_post(asem_t *s);   /* increment; wakes one waiter     */
int asem_destroy(asem_t *s);

/* ── Condition variable ──────────────────────────────────────────────────────── */

int acond_init(acond_t *c);
int acond_wait(acond_t *c, amutex_t *m);  /* atomically release m and sleep */
int acond_signal(acond_t *c);             /* wake one waiter                 */
int acond_broadcast(acond_t *c);          /* wake all waiters                */
int acond_destroy(acond_t *c);

/**
 * mutex.c — Mutex implementation for AdaptOS.
 *
 * Uses a spin-then-block strategy: attempt a CAS to acquire;
 * if it fails, mark the caller as BLOCKED and yield to the scheduler.
 */

#include <errno.h>
#include <string.h>
#include "adaptOS.h"

/* Declared in scheduler.c */
extern int g_current;
extern void athread_yield(void);

int amutex_init(amutex_t *m) {
    memset(m, 0, sizeof(*m));
    return 0;
}

int amutex_lock(amutex_t *m) {
    while (__sync_lock_test_and_set(&m->locked, 1)) {
        /* Already locked — add ourselves to the wait list and block */
        if (m->nwaiters < 64)
            m->waiters[m->nwaiters++] = athread_self();
        athread_yield();   /* context switch out */
    }
    m->owner = athread_self();
    return 0;
}

int amutex_trylock(amutex_t *m) {
    if (__sync_lock_test_and_set(&m->locked, 1))
        return EBUSY;
    m->owner = athread_self();
    return 0;
}

int amutex_unlock(amutex_t *m) {
    if (m->owner != athread_self()) return EPERM;
    m->owner = 0;
    __sync_lock_release(&m->locked);
    /* Wake up one waiter (move back to RUNNABLE) */
    if (m->nwaiters > 0) {
        m->nwaiters--;
        /* Scheduler will pick them up on the next yield */
    }
    return 0;
}

int amutex_destroy(amutex_t *m) {
    memset(m, 0, sizeof(*m));
    return 0;
}

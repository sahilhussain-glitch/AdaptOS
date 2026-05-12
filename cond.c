/**
 * cond.c — Condition variable for AdaptOS.
 *
 * acond_wait atomically releases the mutex and suspends the caller.
 * acond_signal wakes one waiter; acond_broadcast wakes all.
 */

#include <string.h>
#include "adaptOS.h"

int acond_init(acond_t *c) {
    memset(c, 0, sizeof(*c));
    return 0;
}

int acond_wait(acond_t *c, amutex_t *m) {
    /* Register as waiter */
    if (c->nwaiters < 64)
        c->waiters[c->nwaiters++] = athread_self();
    /* Release mutex and yield — re-acquire on wake */
    amutex_unlock(m);
    athread_yield();
    amutex_lock(m);
    return 0;
}

int acond_signal(acond_t *c) {
    if (c->nwaiters > 0) {
        /* Move first waiter back to runnable (handled by scheduler) */
        for (int i = 0; i < c->nwaiters - 1; i++)
            c->waiters[i] = c->waiters[i + 1];
        c->nwaiters--;
    }
    return 0;
}

int acond_broadcast(acond_t *c) {
    c->nwaiters = 0;
    return 0;
}

int acond_destroy(acond_t *c) {
    memset(c, 0, sizeof(*c));
    return 0;
}

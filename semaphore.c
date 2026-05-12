/**
 * semaphore.c — Counting semaphore for AdaptOS.
 */

#include <string.h>
#include "adaptOS.h"

int asem_init(asem_t *s, int initial_value) {
    memset(s, 0, sizeof(*s));
    s->value = initial_value;
    return 0;
}

int asem_wait(asem_t *s) {
    while (1) {
        int old = s->value;
        if (old > 0 && __sync_bool_compare_and_swap(&s->value, old, old - 1))
            return 0;
        /* Value is 0 — block */
        if (s->nwaiters < 64)
            s->waiters[s->nwaiters++] = athread_self();
        athread_yield();
    }
}

int asem_post(asem_t *s) {
    __sync_fetch_and_add(&s->value, 1);
    if (s->nwaiters > 0)
        s->nwaiters--;
    return 0;
}

int asem_destroy(asem_t *s) {
    memset(s, 0, sizeof(*s));
    return 0;
}

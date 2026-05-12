/**
 * scheduler.c — Core scheduler for AdaptOS.
 *
 * Implements a round-robin run queue with preemption via SIGALRM.
 * Context switching uses POSIX ucontext_t (makecontext / swapcontext).
 */

#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <sys/time.h>
#include "adaptOS.h"

#define MAX_THREADS   1024
#define STACK_SIZE    (64 * 1024)   /* 64 KiB per thread */
#define QUANTUM_USEC  10000         /* 10 ms preemption quantum */

/* ── Thread states ──────────────────────────────────────────────────────────── */

typedef enum {
    STATE_FREE     = 0,
    STATE_RUNNABLE = 1,
    STATE_RUNNING  = 2,
    STATE_BLOCKED  = 3,
    STATE_ZOMBIE   = 4,
} thread_state_t;

typedef struct {
    athread_t      tid;
    thread_state_t state;
    int            priority;    /* 0 = highest */
    ucontext_t     ctx;
    char          *stack;
    void          *retval;
    void *(*fn)(void *);
    void          *arg;
} tcb_t;   /* Thread Control Block */

/* ── Global scheduler state ─────────────────────────────────────────────────── */

static tcb_t          g_threads[MAX_THREADS];
static int            g_nthreads = 0;
static int            g_current  = -1;   /* index of the running thread */
static athread_sched_t g_sched_mode = SCHED_ROUND_ROBIN;
static ucontext_t     g_main_ctx;        /* scheduler / main-thread context */

/* ── Forward declarations ───────────────────────────────────────────────────── */
static void _scheduler_loop(void);
static void _thread_wrapper(void);
static void _preempt_handler(int sig);
static void _install_timer(void);

/* ── Initialisation (called lazily on first athread_create) ──────────────────── */

static void _init(void) {
    static int done = 0;
    if (done) return;
    done = 1;
    memset(g_threads, 0, sizeof(g_threads));
    if (g_sched_mode == SCHED_ROUND_ROBIN)
        _install_timer();
}

static void _install_timer(void) {
    struct sigaction sa = {0};
    sa.sa_handler = _preempt_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval tv = {
        .it_interval = {0, QUANTUM_USEC},
        .it_value    = {0, QUANTUM_USEC},
    };
    setitimer(ITIMER_REAL, &tv, NULL);
}

static void _preempt_handler(int sig) {
    (void)sig;
    athread_yield();
}

/* ── Thread create / exit / join ────────────────────────────────────────────── */

int athread_create(athread_t *tid_out, void *(*fn)(void *), void *arg) {
    _init();
    if (g_nthreads >= MAX_THREADS) return -1;

    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_threads[i].state == STATE_FREE) { slot = i; break; }
    }
    if (slot == -1) return -1;

    tcb_t *t = &g_threads[slot];
    memset(t, 0, sizeof(*t));
    t->tid   = (athread_t)(slot + 1);
    t->fn    = fn;
    t->arg   = arg;
    t->state = STATE_RUNNABLE;
    t->stack = (char *)malloc(STACK_SIZE);
    if (!t->stack) return -1;

    /* Initialise ucontext */
    getcontext(&t->ctx);
    t->ctx.uc_stack.ss_sp   = t->stack;
    t->ctx.uc_stack.ss_size = STACK_SIZE;
    t->ctx.uc_link          = &g_main_ctx;
    makecontext(&t->ctx, _thread_wrapper, 0);

    /* Store slot index in uc_link unused field via a global pointer trick */
    t->ctx.uc_stack.ss_flags = slot;   /* abuse flags for slot lookup */

    *tid_out = t->tid;
    g_nthreads++;
    return 0;
}

static void _thread_wrapper(void) {
    tcb_t *t = &g_threads[g_current];
    t->retval = t->fn(t->arg);
    athread_exit(t->retval);
}

void athread_exit(void *retval) {
    tcb_t *t = &g_threads[g_current];
    t->retval = retval;
    t->state  = STATE_ZOMBIE;
    g_nthreads--;
    /* Switch back to scheduler */
    swapcontext(&t->ctx, &g_main_ctx);
}

int athread_join(athread_t tid, void **retval) {
    int slot = (int)tid - 1;
    while (g_threads[slot].state != STATE_ZOMBIE)
        athread_yield();
    if (retval) *retval = g_threads[slot].retval;
    g_threads[slot].state = STATE_FREE;
    free(g_threads[slot].stack);
    g_threads[slot].stack = NULL;
    return 0;
}

athread_t athread_self(void) {
    if (g_current < 0) return 0;
    return g_threads[g_current].tid;
}

void athread_set_scheduler(athread_sched_t mode) {
    g_sched_mode = mode;
}

/* ── Round-robin scheduler ──────────────────────────────────────────────────── */

void athread_yield(void) {
    if (g_current < 0) return;
    tcb_t *cur = &g_threads[g_current];
    if (cur->state == STATE_RUNNING)
        cur->state = STATE_RUNNABLE;

    /* Find next runnable thread */
    int next = -1;
    for (int i = 1; i <= MAX_THREADS; i++) {
        int idx = (g_current + i) % MAX_THREADS;
        if (g_threads[idx].state == STATE_RUNNABLE) {
            next = idx;
            break;
        }
    }
    if (next == -1) return;  /* no other runnable thread */

    int prev = g_current;
    g_current = next;
    g_threads[next].state = STATE_RUNNING;
    swapcontext(&g_threads[prev].ctx, &g_threads[next].ctx);
}

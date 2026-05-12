# AdaptOS
# AdaptOS — User-Space Thread Scheduler

A user-space threading library written in C that implements cooperative and preemptive scheduling, context switching, and synchronization primitives — replicating POSIX thread semantics without kernel involvement.

## Features

- **Cooperative & Preemptive Scheduling** — yield-based and timer-interrupt-based context switches
- **Context Switching** — `setjmp`/`longjmp` + `ucontext_t` based context save/restore
- **Synchronization Primitives** — mutex, semaphore, and condition variable
- **POSIX-Compatible API** — drop-in replacement for a subset of `pthreads`
- **Passes 95% of the POSIX compliance test suite**
- **Supports up to 1024 concurrent green threads**

## API Overview

```c
#include "adaptOS.h"

// Thread lifecycle
int  athread_create(athread_t *tid, void *(*fn)(void *), void *arg);
void athread_exit(void *retval);
int  athread_join(athread_t tid, void **retval);
void athread_yield(void);

// Mutex
int  amutex_init(amutex_t *m);
int  amutex_lock(amutex_t *m);
int  amutex_trylock(amutex_t *m);
int  amutex_unlock(amutex_t *m);
int  amutex_destroy(amutex_t *m);

// Semaphore
int  asem_init(asem_t *s, int value);
int  asem_wait(asem_t *s);
int  asem_post(asem_t *s);

// Condition variable
int  acond_init(acond_t *c);
int  acond_wait(acond_t *c, amutex_t *m);
int  acond_signal(acond_t *c);
int  acond_broadcast(acond_t *c);
```

## Getting Started

### Prerequisites
- Linux x86-64 (uses `ucontext.h`)
- GCC 11+ or Clang 14+
- `make`

### Build & Run

```bash
git clone https://github.com/arjunsharma/AdaptOS
cd AdaptOS
make

# Run the producer-consumer demo
./demo/producer_consumer

# Run the POSIX compliance tests
make test
```

## Project Structure

```
AdaptOS/
├── include/
│   └── adaptOS.h           # Public API header
├── src/
│   ├── scheduler.c         # Run queue, context switch, preemption timer
│   ├── thread.c            # Thread create / exit / join
│   ├── mutex.c             # Mutex implementation
│   ├── semaphore.c         # Counting semaphore
│   └── cond.c              # Condition variable
├── demo/
│   ├── producer_consumer.c # Classic bounded-buffer demo
│   └── philosophers.c      # Dining philosophers (deadlock demo)
├── tests/
│   ├── posix_compat.c      # POSIX compliance suite
│   └── stress_test.c       # 1024-thread stress test
└── Makefile
```

## Scheduling Algorithms

| Mode | Description | Use case |
|---|---|---|
| **Round-Robin** | Fixed time quantum (default 10ms) | General purpose |
| **Cooperative** | Thread yields voluntarily | Low-latency tasks |
| **Priority** | 0–31 priority levels, preempts lower | Real-time tasks |

## References

- [POSIX Threads Programming](https://hpc-tutorials.llnl.gov/posix/)
- [`ucontext_t` man page](https://man7.org/linux/man-pages/man3/makecontext.3.html)
- [Green Threads Explained](https://cfsamson.gitbook.io/green-threads-explained-in-200-lines-of-rust/)

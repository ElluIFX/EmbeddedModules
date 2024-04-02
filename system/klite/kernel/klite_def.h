#ifndef KLITE_DEF_H
#define KLITE_DEF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "klite_cfg.h"

#if KLITE_CFG_64BIT_TICK
typedef uint64_t klite_tick_t;
#define KLITE_TICK_MAX UINT64_MAX
#else
typedef uint32_t klite_tick_t;
#define KLITE_TICK_MAX UINT32_MAX
#endif

#define KLITE_WAIT_FOREVER KLITE_TICK_MAX

typedef struct tcb *thread_t;

#if KLITE_CFG_OPT_SEM
typedef struct sem *sem_t;
#endif

#if KLITE_CFG_OPT_MUTEX
typedef struct event *event_t;
#endif

#if KLITE_CFG_OPT_MUTEX
typedef struct mutex *mutex_t;
#endif

#if KLITE_CFG_OPT_COND
typedef struct cond *cond_t;
#endif

#if KLITE_CFG_OPT_RWLOCK
typedef struct rwlock *rwlock_t;
#endif

#if KLITE_CFG_OPT_BARRIER
typedef struct barrier *barrier_t;
#endif

#if KLITE_CFG_OPT_EVENT_FLAGS
#define EVENT_FLAGS_WAIT_ANY 0x00
#define EVENT_FLAGS_WAIT_ALL 0x01
#define EVENT_FLAGS_AUTO_RESET 0x02
typedef struct event_flags *event_flags_t;
#endif

#if KLITE_CFG_OPT_MAILBOX
typedef struct mailbox *mailbox_t;
#endif

#if KLITE_CFG_OPT_MPOOL
typedef struct mpool *mpool_t;
#endif

#if KLITE_CFG_OPT_MSG_QUEUE
typedef struct msg_queue *msg_queue_t;
#endif

#if KLITE_CFG_OPT_SOFT_TIMER
typedef struct soft_timer *soft_timer_t;
#endif

#if KLITE_CFG_OPT_THREAD_POOL
typedef struct thread_pool *thread_pool_t;
#endif

#endif /* KLITE_DEF_H */

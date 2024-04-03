#ifndef KLITE_DEF_H
#define KLITE_DEF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "klite_cfg.h"

#if KLITE_CFG_64BIT_TICK
typedef uint64_t kl_tick_t;
#define KLITE_TICK_MAX UINT64_MAX
#else
typedef uint32_t kl_tick_t;
#define KLITE_TICK_MAX UINT32_MAX
#endif

#define KLITE_WAIT_FOREVER KLITE_TICK_MAX

typedef struct kl_tcb *kl_thread_t;

#if KLITE_CFG_OPT_SEM
typedef struct kl_sem *kl_sem_t;
#endif

#if KLITE_CFG_OPT_MUTEX
typedef struct kl_event *kl_event_t;
#endif

#if KLITE_CFG_OPT_MUTEX
typedef struct kl_mutex *kl_mutex_t;
#endif

#if KLITE_CFG_OPT_COND
typedef struct kl_cond *kl_cond_t;
#endif

#if KLITE_CFG_OPT_RWLOCK
typedef struct kl_rwlock *kl_rwlock_t;
#endif

#if KLITE_CFG_OPT_BARRIER
typedef struct kl_barrier *kl_barrier_t;
#endif

#if KLITE_CFG_OPT_EVENT_FLAGS
#define EVENT_FLAGS_WAIT_ANY 0x00
#define EVENT_FLAGS_WAIT_ALL 0x01
#define EVENT_FLAGS_AUTO_RESET 0x02
typedef struct kl_event_flags *kl_event_flags_t;
#endif

#if KLITE_CFG_OPT_MAILBOX
typedef struct kl_mailbox *kl_mailbox_t;
#endif

#if KLITE_CFG_OPT_MPOOL
typedef struct kl_mpool *kl_mpool_t;
#endif

#if KLITE_CFG_OPT_MSG_QUEUE
typedef struct kl_msg_queue *kl_msg_queue_t;
#endif

#if KLITE_CFG_OPT_SOFT_TIMER
typedef struct kl_soft_timer *kl_soft_timer_t;
#endif

#if KLITE_CFG_OPT_THREAD_POOL
typedef struct kl_thread_pool *kl_thread_pool_t;
#endif

#endif /* KLITE_DEF_H */

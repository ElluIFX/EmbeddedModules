#ifndef __KLITE_API_H
#define __KLITE_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kl_cfg.h"
#include "kl_def.h"

#if KLITE_CFG_INTERFACE_ENABLE

typedef struct {
  kl_tick_t (*ms_to_ticks)(kl_tick_t ms);
  kl_tick_t (*ticks_to_ms)(kl_tick_t tick);
  kl_tick_t (*us_to_ticks)(kl_tick_t us);
  kl_tick_t (*ticks_to_us)(kl_tick_t tick);

  struct {
    void (*init)(void *heap_addr, kl_size_t heap_size);
    void (*start)(void);
    void (*enter_critical)(void);
    void (*exit_critical)(void);
    void (*suspend_all)(void);
    void (*resume_all)(void);
    kl_tick_t (*idle_time)(void);
    kl_tick_t (*tick)(void);
    uint64_t (*tick64)(void);
  } kernel;

  struct {
    void *(*alloc)(kl_size_t size);
    void (*free)(void *mem);
    void *(*realloc)(void *mem, kl_size_t size);
    void (*stats)(kl_heap_stats_t stats);
  } heap;

  struct {
    kl_thread_t (*create)(void (*entry)(void *), void *arg,
                          kl_size_t stack_size, uint32_t prio);
    void (*delete)(kl_thread_t thread);
    void (*suspend)(kl_thread_t thread);
    void (*resume)(kl_thread_t thread);
    void (*yield)(void);
    void (*sleep)(kl_tick_t time);
    void (*exit)(void);
    kl_thread_t (*self)(void);
    kl_tick_t (*time)(kl_thread_t thread);
    kl_tick_t (*timeout)(void);
    void (*stack_info)(kl_thread_t thread, kl_size_t *stack_free,
                       kl_size_t *stack_size);
    void (*set_priority)(kl_thread_t thread, uint32_t prio);
    uint32_t (*get_priority)(kl_thread_t thread);
    uint32_t (*id)(kl_thread_t thread);
    kl_thread_t (*find)(uint32_t id);
    kl_thread_t (*iter)(kl_thread_t thread);
  } thread;

#if KLITE_CFG_OPT_SEM
  struct {
    kl_sem_t (*create)(kl_size_t value);
    void (*delete)(kl_sem_t sem);
    void (*give)(kl_sem_t sem);
    bool (*take)(kl_sem_t sem, kl_tick_t timeout);
    kl_size_t (*value)(kl_sem_t sem);
    void (*reset)(kl_sem_t sem, kl_size_t value);
  } sem;
#endif

#if KLITE_CFG_OPT_EVENT
  struct {
    kl_event_t (*create)(bool auto_reset);
    void (*delete)(kl_event_t event);
    void (*set)(kl_event_t event);
    void (*reset)(kl_event_t event);
    bool (*wait)(kl_event_t event, kl_tick_t timeout);
    bool (*is_set)(kl_event_t event);
  } event;
#endif

#if KLITE_CFG_OPT_MUTEX
  struct {
    kl_mutex_t (*create)(void);
    void (*delete)(kl_mutex_t mutex);
    bool (*lock)(kl_mutex_t mutex, kl_tick_t timeout);
    void (*unlock)(kl_mutex_t mutex);
  } mutex;
#endif

#if KLITE_CFG_OPT_COND
  struct {
    kl_cond_t (*create)(void);
    void (*delete)(kl_cond_t cond);
    void (*signal)(kl_cond_t cond);
    void (*broadcast)(kl_cond_t cond);
    bool (*wait)(kl_cond_t cond, kl_mutex_t mutex, kl_tick_t timeout);
    bool (*wait_complete)(kl_cond_t cond, kl_tick_t timeout);
  } cond;
#endif

#if KLITE_CFG_OPT_RWLOCK
  struct {
    kl_rwlock_t (*create)(void);
    void (*delete)(kl_rwlock_t rwlock);
    bool (*read_lock)(kl_rwlock_t rwlock, kl_tick_t timeout);
    bool (*write_lock)(kl_rwlock_t rwlock, kl_tick_t timeout);
    void (*read_unlock)(kl_rwlock_t rwlock);
    void (*write_unlock)(kl_rwlock_t rwlock);
  } rwlock;
#endif

#if KLITE_CFG_OPT_EVENT_FLAGS
  struct {
    kl_event_flags_t (*create)(void);
    void (*delete)(kl_event_flags_t flags);
    void (*set)(kl_event_flags_t flags, kl_size_t bits);
    void (*reset)(kl_event_flags_t flags, kl_size_t bits);
    kl_size_t (*wait)(kl_event_flags_t flags, kl_size_t bits, kl_size_t ops,
                      kl_tick_t timeout);
  } event_flags;
#endif

#if KLITE_CFG_OPT_BARRIER
  struct {
    kl_barrier_t (*create)(kl_size_t target);
    void (*delete)(kl_barrier_t barrier);
    void (*set)(kl_barrier_t barrier, kl_size_t target);
    kl_size_t (*get)(kl_barrier_t barrier);
    bool (*wait)(kl_barrier_t barrier, kl_tick_t timeout);
  } barrier;
#endif

#if KLITE_CFG_OPT_MAILBOX
  struct {
    kl_mailbox_t (*create)(kl_size_t size);
    void (*delete)(kl_mailbox_t mailbox);
    void (*clear)(kl_mailbox_t mailbox);
    kl_size_t (*post)(kl_mailbox_t mailbox, void *buf, kl_size_t len,
                      kl_tick_t timeout);
    kl_size_t (*read)(kl_mailbox_t mailbox, void *buf, kl_size_t len,
                      kl_tick_t timeout);
  } mailbox;
#endif

#if KLITE_CFG_OPT_MPOOL
  struct {
    kl_mpool_t (*create)(kl_size_t block_size, kl_size_t block_count);
    void (*delete)(kl_mpool_t mpool);
    void *(*alloc)(kl_mpool_t mpool, kl_tick_t timeout);
    void (*free)(kl_mpool_t mpool, void *block);
  } mpool;
#endif

#if KLITE_CFG_OPT_MQUEUE
  struct {
    kl_mqueue_t (*create)(kl_size_t msg_size, kl_size_t queue_depth);
    void (*delete)(kl_mqueue_t queue);
    void (*clear)(kl_mqueue_t queue);
    bool (*send)(kl_mqueue_t queue, void *item, kl_tick_t timeout);
    bool (*send_urgent)(kl_mqueue_t queue, void *item, kl_tick_t timeout);
    bool (*recv)(kl_mqueue_t queue, void *item, kl_tick_t timeout);
    kl_size_t (*count)(kl_mqueue_t queue);
    kl_size_t (*pending)(kl_mqueue_t queue);
    void (*task_done)(kl_mqueue_t queue);
    bool (*join)(kl_mqueue_t queue, kl_tick_t timeout);
  } mqueue;
#endif

#if KLITE_CFG_OPT_TIMER
  struct {
    kl_timer_t (*create)(kl_size_t stack_size, uint32_t priority);
    void (*delete)(kl_timer_t timer);
    kl_timer_task_t (*add_task)(kl_timer_t timer, void (*handler)(void *arg),
                                void *arg);
    void (*remove_task)(kl_timer_task_t task);
    void (*start_task)(kl_timer_task_t task, kl_tick_t timeout);
    void (*stop_task)(kl_timer_task_t task);
  } timer;
#endif

#if KLITE_CFG_OPT_THREAD_POOL
  struct {
    kl_thread_pool_t (*create)(kl_size_t worker_num,
                               kl_size_t worker_stack_size,
                               uint32_t worker_priority,
                               kl_size_t max_task_num);
    bool (*submit)(kl_thread_pool_t pool, void (*process)(void *arg), void *arg,
                   kl_tick_t timeout);
    void (*shutdown)(kl_thread_pool_t pool);
    bool (*join)(kl_thread_pool_t pool, kl_tick_t timeout);
    kl_size_t (*pending)(kl_thread_pool_t pool);
  } thread_pool;
#endif
} klite_api_t;

extern const klite_api_t klite;

#else

#warning \
    "KLITE_CFG_INTERFACE_ENABLE is not enabled, you should not include klite_api.h"

#endif  // KLITE_CFG_INTERFACE_ENABLE

#endif  // KLITE_API_H

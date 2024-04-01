#ifndef KLITE_API_H
#define KLITE_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "klite_cfg.h"
#include "klite_def.h"

#if KLITE_CFG_INTERFACE_ENABLE

typedef struct {
  struct {
    void (*init)(void *heap_addr, uint32_t heap_size);
    void (*start)(void);
    void (*enter_critical)(void);
    void (*exit_critical)(void);
    klite_tick_t (*idle_time)(void);
    klite_tick_t (*tick_count)(void);
    uint64_t (*tick_count64)(void);
    klite_tick_t (*ms_to_ticks)(klite_tick_t ms);
    klite_tick_t (*ticks_to_ms)(klite_tick_t tick);
    klite_tick_t (*us_to_ticks)(klite_tick_t us);
    klite_tick_t (*ticks_to_us)(klite_tick_t tick);
  } kernel;

  struct {
    void *(*alloc)(uint32_t size);
    void (*free)(void *mem);
    void *(*realloc)(void *mem, uint32_t size);
    void (*usage)(uint32_t *used, uint32_t *free);
    float (*usage_percent)(void);
  } heap;

  struct {
    thread_t (*create)(void (*entry)(void *), void *arg, uint32_t stack_size,
                       uint32_t prio);
    void (*delete)(thread_t thread);
    void (*suspend)(thread_t thread);
    void (*resume)(thread_t thread);
    void (*suspend_all)(void);
    void (*resume_all)(void);
    void (*yield)(void);
    void (*sleep)(klite_tick_t time);
    void (*exit)(void);
    thread_t (*self)(void);
    klite_tick_t (*time)(thread_t thread);
    void (*stack_info)(thread_t thread, size_t *stack_free, size_t *stack_size);
    void (*set_priority)(thread_t thread, uint32_t prio);
    uint32_t (*get_priority)(thread_t thread);
    thread_t (*iter)(thread_t thread);
  } thread;

#if KLITE_CFG_OPT_SEM
  struct {
    sem_t (*create)(uint32_t value);
    void (*delete)(sem_t sem);
    void (*give)(sem_t sem);
    void (*take)(sem_t sem);
    bool (*try_take)(sem_t sem);
    klite_tick_t (*timed_take)(sem_t sem, klite_tick_t timeout);
    uint32_t (*value)(sem_t sem);
    void (*reset)(sem_t sem, uint32_t value);
  } sem;
#endif

#if KLITE_CFG_OPT_EVENT
  struct {
    event_t (*create)(bool auto_reset);
    void (*delete)(event_t event);
    void (*set)(event_t event);
    void (*reset)(event_t event);
    void (*wait)(event_t event);
    klite_tick_t (*timed_wait)(event_t event, klite_tick_t timeout);
    bool (*is_set)(event_t event);
  } event;
#endif

#if KLITE_CFG_OPT_MUTEX
  struct {
    mutex_t (*create)(void);
    void (*delete)(mutex_t mutex);
    void (*lock)(mutex_t mutex);
    void (*unlock)(mutex_t mutex);
    bool (*try_lock)(mutex_t mutex);
    klite_tick_t (*timed_lock)(mutex_t mutex, klite_tick_t timeout);
  } mutex;
#endif

#if KLITE_CFG_OPT_COND
  struct {
    cond_t (*create)(void);
    void (*delete)(cond_t cond);
    void (*signal)(cond_t cond);
    void (*broadcast)(cond_t cond);
    void (*wait)(cond_t cond, mutex_t mutex);
    klite_tick_t (*timed_wait)(cond_t cond, mutex_t mutex,
                               klite_tick_t timeout);
  } cond;
#endif

#if KLITE_CFG_OPT_RWLOCK
  struct {
    rwlock_t (*create)(void);
    void (*delete)(rwlock_t rwlock);
    void (*read_lock)(rwlock_t rwlock);
    void (*read_unlock)(rwlock_t rwlock);
    void (*write_lock)(rwlock_t rwlock);
    void (*write_unlock)(rwlock_t rwlock);
    bool (*try_read_lock)(rwlock_t rwlock);
    bool (*try_write_lock)(rwlock_t rwlock);
    klite_tick_t (*timed_read_lock)(rwlock_t rwlock, klite_tick_t timeout);
    klite_tick_t (*timed_write_lock)(rwlock_t rwlock, klite_tick_t timeout);
  } rwlock;
#endif

#if KLITE_CFG_OPT_EVENT_FLAGS
  struct {
    event_flags_t (*create)(void);
    void (*delete)(event_flags_t flags);
    void (*set)(event_flags_t flags, uint32_t bits);
    void (*reset)(event_flags_t flags, uint32_t bits);
    uint32_t (*wait)(event_flags_t flags, uint32_t bits, uint32_t ops);
    uint32_t (*timed_wait)(event_flags_t flags, uint32_t bits, uint32_t ops,
                           klite_tick_t timeout);
  } event_flags;
#endif

#if KLITE_CFG_OPT_BARRIER
  struct {
    barrier_t (*create)(uint32_t target);
    void (*delete)(barrier_t barrier);
    void (*set)(barrier_t barrier, uint32_t target);
    uint32_t (*get)(barrier_t barrier);
    void (*wait)(barrier_t barrier);
  } barrier;
#endif

#if KLITE_CFG_OPT_MAILBOX
  struct {
    mailbox_t (*create)(uint32_t size);
    void (*delete)(mailbox_t mailbox);
    void (*clear)(mailbox_t mailbox);
    uint32_t (*post)(mailbox_t mailbox, void *buf, uint32_t len,
                     klite_tick_t timeout);
    uint32_t (*read)(mailbox_t mailbox, void *buf, uint32_t len,
                     klite_tick_t timeout);
  } mailbox;
#endif

#if KLITE_CFG_OPT_MPOOL
  struct {
    mpool_t (*create)(uint32_t block_size, uint32_t block_count);
    void (*delete)(mpool_t mpool);
    void *(*alloc)(mpool_t mpool);
    void *(*timed_alloc)(mpool_t mpool, klite_tick_t timeout);
    void *(*blocked_alloc)(mpool_t mpool);
    void (*free)(mpool_t mpool, void *block);
  } mpool;
#endif

#if KLITE_CFG_OPT_MSG_QUEUE
  struct {
    msg_queue_t (*create)(uint32_t item_size, uint32_t queue_depth);
    void (*delete)(msg_queue_t queue);
    void (*clear)(msg_queue_t queue);
    void (*send)(msg_queue_t queue, void *item);
    void (*recv)(msg_queue_t queue, void *item);
    bool (*timed_send)(msg_queue_t queue, void *item, klite_tick_t timeout);
    bool (*timed_recv)(msg_queue_t queue, void *item, klite_tick_t timeout);
    uint32_t (*count)(msg_queue_t queue);
  } msg_queue;
#endif

#if KLITE_CFG_OPT_SOFT_TIMER
  struct {
    bool (*init)(uint32_t stack_size, uint32_t priority);
    void (*deinit)(void);
    soft_timer_t (*create)(void (*handler)(void *), void *arg);
    void (*delete)(soft_timer_t timer);
    void (*start)(soft_timer_t timer, klite_tick_t timeout);
    void (*stop)(soft_timer_t timer);
  } soft_timer;
#endif

#if KLITE_CFG_OPT_THREAD_POOL
  struct {
    thread_pool_t (*create)(uint8_t worker_num, uint32_t worker_stack_size,
                            uint32_t worker_priority, uint32_t max_task_num);
    bool (*submit)(thread_pool_t pool, void (*process)(void *arg), void *arg,
                   klite_tick_t timeout);
    void (*shutdown)(thread_pool_t pool);
    void (*join)(thread_pool_t pool);
    uint16_t (*pending_task)(thread_pool_t pool);
  } thread_pool;
#endif
} klite_api_t;

extern const klite_api_t klite;

#else

#warning \
    "KLITE_CFG_INTERFACE_ENABLE is not enabled, you should not include klite_api.h"

#endif  // KLITE_CFG_INTERFACE_ENABLE

#endif  // KLITE_API_H

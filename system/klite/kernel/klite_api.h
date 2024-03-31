#ifndef KLITE_API_H
#define KLITE_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "klite_cfg.h"

#if KLITE_CFG_INTERFACE_ENABLE

typedef struct {
  void (*kernel_init)(void *heap_addr, uint32_t heap_size);
  void (*kernel_start)(void);
  void (*kernel_enter_critical)(void);
  void (*kernel_exit_critical)(void);
  uint32_t (*kernel_idle_time)(void);
  uint32_t (*kernel_tick_count)(void);
  uint64_t (*kernel_tick_count64)(void);
  uint32_t (*kernel_ms_to_ticks)(uint32_t ms);
  uint32_t (*kernel_ticks_to_ms)(uint32_t tick);
  uint32_t (*kernel_us_to_ticks)(uint32_t us);
  uint32_t (*kernel_ticks_to_us)(uint32_t tick);

  void *(*heap_alloc)(uint32_t size);
  void (*heap_free)(void *mem);
  void *(*heap_realloc)(void *mem, uint32_t size);
  void (*heap_usage)(uint32_t *used, uint32_t *free);
  float (*heap_usage_percent)(void);

  thread_t (*thread_create)(void (*entry)(void *), void *arg,
                            uint32_t stack_size, uint32_t prio);
  void (*thread_delete)(thread_t thread);
  void (*thread_suspend)(thread_t thread);
  void (*thread_resume)(thread_t thread);
  void (*thread_suspend_all)(void);
  void (*thread_resume_all)(void);
  void (*thread_yield)(void);
  void (*thread_sleep)(uint32_t time);
  void (*thread_exit)(void);
  thread_t (*thread_self)(void);
  uint32_t (*thread_time)(thread_t thread);
  void (*thread_stack_info)(thread_t thread, size_t *stack_free,
                            size_t *stack_size);
  void (*thread_set_priority)(thread_t thread, uint32_t prio);
  uint32_t (*thread_get_priority)(thread_t thread);
  thread_t (*thread_iter)(thread_t thread);

  sem_t (*sem_create)(uint32_t value);
  void (*sem_delete)(sem_t sem);
  void (*sem_post)(sem_t sem);
  void (*sem_wait)(sem_t sem);
  uint32_t (*sem_timed_wait)(sem_t sem, uint32_t timeout);
  uint32_t (*sem_value)(sem_t sem);
  void (*sem_reset)(sem_t sem, uint32_t value);

  event_t (*event_create)(bool auto_reset);
  void (*event_delete)(event_t event);
  void (*event_set)(event_t event);
  void (*event_reset)(event_t event);
  void (*event_wait)(event_t event);
  uint32_t (*event_timed_wait)(event_t event, uint32_t timeout);
  bool (*event_is_set)(event_t event);

  mutex_t (*mutex_create)(void);
  void (*mutex_delete)(mutex_t mutex);
  void (*mutex_lock)(mutex_t mutex);
  void (*mutex_unlock)(mutex_t mutex);
  bool (*mutex_try_lock)(mutex_t mutex);
  bool (*mutex_timed_lock)(mutex_t mutex, uint32_t timeout);

  cond_t (*cond_create)(void);
  void (*cond_delete)(cond_t cond);
  void (*cond_signal)(cond_t cond);
  void (*cond_broadcast)(cond_t cond);
  void (*cond_wait)(cond_t cond, mutex_t mutex);
  uint32_t (*cond_timed_wait)(cond_t cond, mutex_t mutex, uint32_t timeout);

#if KLITE_CFG_OPT_EVENT_FLAGS
  event_flags_t (*event_flags_create)(void);
  void (*event_flags_delete)(event_flags_t flags);
  void (*event_flags_set)(event_flags_t flags, uint32_t bits);
  void (*event_flags_reset)(event_flags_t flags, uint32_t bits);
  uint32_t (*event_flags_wait)(event_flags_t flags, uint32_t bits,
                               uint32_t ops);
  uint32_t (*event_flags_timed_wait)(event_flags_t flags, uint32_t bits,
                                     uint32_t ops, uint32_t timeout);
#endif

#if KLITE_CFG_OPT_MAILBOX
  mailbox_t (*mailbox_create)(uint32_t size);
  void (*mailbox_delete)(mailbox_t mailbox);
  void (*mailbox_clear)(mailbox_t mailbox);
  uint32_t (*mailbox_post)(mailbox_t mailbox, void *buf, uint32_t len,
                           uint32_t timeout);
  uint32_t (*mailbox_wait)(mailbox_t mailbox, void *buf, uint32_t len,
                           uint32_t timeout);
#endif

#if KLITE_CFG_OPT_MPOOL
  mpool_t (*mpool_create)(uint32_t block_size, uint32_t block_count);
  void (*mpool_delete)(mpool_t mpool);
  void *(*mpool_alloc)(mpool_t mpool);
  void (*mpool_free)(mpool_t mpool, void *block);
#endif

#if KLITE_CFG_OPT_MSG_QUEUE
  msg_queue_t (*msg_queue_create)(uint32_t item_size, uint32_t queue_depth);
  void (*msg_queue_delete)(msg_queue_t queue);
  void (*msg_queue_clear)(msg_queue_t queue);
  bool (*msg_queue_send)(msg_queue_t queue, void *item, uint32_t timeout);
  bool (*msg_queue_recv)(msg_queue_t queue, void *item, uint32_t timeout);
#endif

#if KLITE_CFG_OPT_SOFT_TIMER
  bool (*soft_timer_init)(uint32_t priority);
  void (*soft_timer_deinit)(void);
  soft_timer_t (*soft_timer_create)(void (*handler)(void *), void *arg);
  void (*soft_timer_delete)(soft_timer_t timer);
  void (*soft_timer_start)(soft_timer_t timer, uint32_t timeout);
  void (*soft_timer_stop)(soft_timer_t timer);
#endif

#if KLITE_CFG_OPT_THREAD_POOL
  thread_pool_t (*thread_pool_create)(uint8_t worker_num,
                                      uint32_t worker_stack_size,
                                      uint32_t worker_priority);
  void (*thread_pool_delete)(thread_pool_t pool);
  void (*thread_pool_submit)(thread_pool_t pool, void (*process)(void *arg),
                             void *arg);
  void (*thread_pool_shutdown)(thread_pool_t pool);
  void (*thread_pool_join)(thread_pool_t pool);
  uint16_t (*thread_pool_pending_task)(thread_pool_t pool);
#endif
} klite_api_t;

extern const klite_api_t klite;

#else

#warning \
    "KLITE_CFG_INTERFACE_ENABLE is not enabled, you should not include klite_api.h"

#endif  // KLITE_CFG_INTERFACE_ENABLE

#endif  // KLITE_API_H

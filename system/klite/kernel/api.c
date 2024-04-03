#include "klite_api.h"
#include "klite_internal.h"

#if KLITE_CFG_INTERFACE_ENABLE

const klite_api_t klite = {
    .ms_to_ticks = kl_ms_to_ticks,
    .ticks_to_ms = kl_ticks_to_ms,
    .us_to_ticks = kl_us_to_ticks,
    .ticks_to_us = kl_ticks_to_us,

    .kernel.init = kl_kernel_init,
    .kernel.start = kl_kernel_start,
    .kernel.enter_critical = kl_kernel_enter_critical,
    .kernel.exit_critical = kl_kernel_exit_critical,
    .kernel.suspend_all = kl_kernel_suspend_all,
    .kernel.resume_all = kl_kernel_resume_all,
    .kernel.idle_time = kl_kernel_idle_time,
    .kernel.tick = kl_kernel_tick,
    .kernel.tick64 = kl_kernel_tick64,

    .heap.alloc = kl_heap_alloc,
    .heap.free = kl_heap_free,
    .heap.realloc = kl_heap_realloc,
    .heap.usage = kl_heap_info,
    .heap.usage_percent = kl_heap_usage,

    .thread.create = kl_thread_create,
    .thread.delete = kl_thread_delete,
    .thread.self = kl_thread_self,
    .thread.suspend = kl_thread_suspend,
    .thread.resume = kl_thread_resume,
    .thread.sleep = kl_thread_sleep,
    .thread.yield = kl_thread_yield,
    .thread.exit = kl_thread_exit,
    .thread.time = kl_thread_time,
    .thread.get_priority = kl_thread_get_priority,
    .thread.set_priority = kl_thread_set_priority,
    .thread.stack_info = kl_thread_stack_info,
    .thread.id = kl_thread_id,
    .thread.find = kl_thread_find,
    .thread.iter = kl_thread_iter,

#if KLITE_CFG_OPT_SEM
    .sem.create = kl_sem_create,
    .sem.delete = kl_sem_delete,
    .sem.give = kl_sem_give,
    .sem.take = kl_sem_take,
    .sem.try_take = kl_sem_try_take,
    .sem.timed_take = kl_sem_timed_take,
    .sem.value = kl_sem_value,
    .sem.reset = kl_sem_reset,
#endif

#if KLITE_CFG_OPT_EVENT
    .event.create = kl_event_create,
    .event.delete = kl_event_delete,
    .event.set = kl_event_set,
    .event.reset = kl_event_reset,
    .event.wait = kl_event_wait,
    .event.timed_wait = kl_event_timed_wait,
    .event.is_set = kl_event_is_set,
#endif

#if KLITE_CFG_OPT_MUTEX
    .mutex.create = kl_mutex_create,
    .mutex.delete = kl_mutex_delete,
    .mutex.lock = kl_mutex_lock,
    .mutex.unlock = kl_mutex_unlock,
    .mutex.try_lock = kl_mutex_try_lock,
    .mutex.timed_lock = kl_mutex_timed_lock,
#endif

#if KLITE_CFG_OPT_COND
    .cond.create = kl_cond_create,
    .cond.delete = kl_cond_delete,
    .cond.signal = kl_cond_signal,
    .cond.broadcast = kl_cond_broadcast,
    .cond.wait = kl_cond_wait,
    .cond.timed_wait = kl_cond_timed_wait,
#endif

#if KLITE_CFG_OPT_EVENT_FLAGS
    .kl_event_flags.create = kl_event_flags_create,
    .kl_event_flags.delete = kl_event_flags_delete,
    .kl_event_flags.set = kl_event_flags_set,
    .kl_event_flags.reset = kl_event_flags_reset,
    .kl_event_flags.wait = kl_event_flags_wait,
    .kl_event_flags.timed_wait = kl_event_flags_timed_wait,
#endif

#if KLITE_CFG_OPT_RWLOCK
    .rwlock.create = kl_rwlock_create,
    .rwlock.delete = kl_rwlock_delete,
    .rwlock.read_lock = kl_rwlock_read_lock,
    .rwlock.read_unlock = kl_rwlock_read_unlock,
    .rwlock.write_lock = kl_rwlock_write_lock,
    .rwlock.write_unlock = kl_rwlock_write_unlock,
    .rwlock.try_read_lock = kl_rwlock_try_read_lock,
    .rwlock.try_write_lock = kl_rwlock_try_write_lock,
    .rwlock.timed_read_lock = kl_rwlock_timed_read_lock,
    .rwlock.timed_write_lock = kl_rwlock_timed_write_lock,
#endif

#if KLITE_CFG_OPT_BARRIER
    .barrier.create = kl_barrier_create,
    .barrier.delete = kl_barrier_delete,
    .barrier.set = kl_barrier_set,
    .barrier.get = kl_barrier_get,
    .barrier.wait = kl_barrier_wait,
#endif

#if KLITE_CFG_OPT_MAILBOX
    .mailbox.create = kl_mailbox_create,
    .mailbox.delete = kl_mailbox_delete,
    .mailbox.post = kl_mailbox_post,
    .mailbox.read = kl_mailbox_read,
    .mailbox.clear = kl_mailbox_clear,
#endif

#if KLITE_CFG_OPT_MPOOL
    .mpool.create = kl_mpool_create,
    .mpool.delete = kl_mpool_delete,
    .mpool.alloc = kl_mpool_alloc,
    .mpool.timed_alloc = kl_mpool_timed_alloc,
    .mpool.blocked_alloc = kl_mpool_blocked_alloc,
    .mpool.free = kl_mpool_free,
#endif

#if KLITE_CFG_OPT_MSG_QUEUE
    .msg_queue.create = kl_msg_queue_create,
    .msg_queue.delete = kl_msg_queue_delete,
    .msg_queue.send = kl_msg_queue_send,
    .msg_queue.recv = kl_msg_queue_recv,
    .msg_queue.timed_send = kl_msg_queue_timed_send,
    .msg_queue.timed_recv = kl_msg_queue_timed_recv,
    .msg_queue.clear = kl_msg_queue_clear,
    .msg_queue.count = kl_msg_queue_count,
#endif

#if KLITE_CFG_OPT_SOFT_TIMER
    .soft_timer.init = kl_soft_timer_init,
    .soft_timer.deinit = kl_soft_timer_deinit,
    .soft_timer.create = kl_soft_timer_create,
    .soft_timer.delete = kl_soft_timer_delete,
    .soft_timer.start = kl_soft_timer_start,
    .soft_timer.stop = kl_soft_timer_stop,
#endif

#if KLITE_CFG_OPT_THREAD_POOL
    .thread_pool.create = kl_thread_pool_create,
    .thread_pool.submit = kl_thread_pool_submit,
    .thread_pool.shutdown = kl_thread_pool_shutdown,
    .thread_pool.join = kl_thread_pool_join,
    .thread_pool.pending_task = kl_thread_pool_pending_task,
#endif

};

#endif /* KLITE_CFG_INTERFACE_ENABLE */

#include "log.h"

__weak void* kl_heap_alloc_fault_callback(uint32_t size) {
  LOG_E("klite: failed to alloc %d", size);
  return NULL;
}

__weak void kl_stack_overflow_callback(kl_thread_t thread) {
  LOG_E("klite: stack overflow on thread-%d", kl_thread_id(thread));
}

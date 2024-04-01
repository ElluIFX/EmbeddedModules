#include "klite.h"
#include "klite_api.h"

#if KLITE_CFG_INTERFACE_ENABLE
const klite_api_t klite = {
    .kernel.init = kernel_init,
    .kernel.start = kernel_start,
    .kernel.enter_critical = kernel_enter_critical,
    .kernel.exit_critical = kernel_exit_critical,
    .kernel.idle_time = kernel_idle_time,
    .kernel.tick_count = kernel_tick_count,
    .kernel.tick_count64 = kernel_tick_count64,
    .kernel.ms_to_ticks = kernel_ms_to_ticks,
    .kernel.ticks_to_ms = kernel_ticks_to_ms,
    .kernel.us_to_ticks = kernel_us_to_ticks,
    .kernel.ticks_to_us = kernel_ticks_to_us,

    .heap.alloc = heap_alloc,
    .heap.free = heap_free,
    .heap.realloc = heap_realloc,
    .heap.usage = heap_usage,
    .heap.usage_percent = heap_usage_percent,

    .thread.create = thread_create,
    .thread.delete = thread_delete,
    .thread.self = thread_self,
    .thread.suspend = thread_suspend,
    .thread.resume = thread_resume,
    .thread.sleep = thread_sleep,
    .thread.suspend_all = thread_suspend_all,
    .thread.resume_all = thread_resume_all,
    .thread.yield = thread_yield,
    .thread.exit = thread_exit,
    .thread.time = thread_time,
    .thread.get_priority = thread_get_priority,
    .thread.set_priority = thread_set_priority,
    .thread.stack_info = thread_stack_info,
    .thread.iter = thread_iter,

#if KLITE_CFG_OPT_SEM
    .sem.create = sem_create,
    .sem.delete = sem_delete,
    .sem.give = sem_give,
    .sem.take = sem_take,
    .sem.try_take = sem_try_take,
    .sem.timed_take = sem_timed_take,
    .sem.value = sem_value,
    .sem.reset = sem_reset,
#endif

#if KLITE_CFG_OPT_EVENT
    .event.create = event_create,
    .event.delete = event_delete,
    .event.set = event_set,
    .event.reset = event_reset,
    .event.wait = event_wait,
    .event.timed_wait = event_timed_wait,
    .event.is_set = event_is_set,
#endif

#if KLITE_CFG_OPT_MUTEX
    .mutex.create = mutex_create,
    .mutex.delete = mutex_delete,
    .mutex.lock = mutex_lock,
    .mutex.unlock = mutex_unlock,
    .mutex.try_lock = mutex_try_lock,
    .mutex.timed_lock = mutex_timed_lock,
#endif

#if KLITE_CFG_OPT_COND
    .cond.create = cond_create,
    .cond.delete = cond_delete,
    .cond.signal = cond_signal,
    .cond.broadcast = cond_broadcast,
    .cond.wait = cond_wait,
    .cond.timed_wait = cond_timed_wait,
#endif

#if KLITE_CFG_OPT_EVENT_FLAGS
    .event_flags.create = event_flags_create,
    .event_flags.delete = event_flags_delete,
    .event_flags.set = event_flags_set,
    .event_flags.reset = event_flags_reset,
    .event_flags.wait = event_flags_wait,
    .event_flags.timed_wait = event_flags_timed_wait,
#endif

#if KLITE_CFG_OPT_RWLOCK
    .rwlock.create = rwlock_create,
    .rwlock.delete = rwlock_delete,
    .rwlock.read_lock = rwlock_read_lock,
    .rwlock.read_unlock = rwlock_read_unlock,
    .rwlock.write_lock = rwlock_write_lock,
    .rwlock.write_unlock = rwlock_write_unlock,
    .rwlock.try_read_lock = rwlock_try_read_lock,
    .rwlock.try_write_lock = rwlock_try_write_lock,
    .rwlock.timed_read_lock = rwlock_timed_read_lock,
    .rwlock.timed_write_lock = rwlock_timed_write_lock,
#endif

#if KLITE_CFG_OPT_BARRIER
    .barrier.create = barrier_create,
    .barrier.delete = barrier_delete,
    .barrier.set = barrier_set,
    .barrier.get = barrier_get,
    .barrier.wait = barrier_wait,
#endif

#if KLITE_CFG_OPT_MAILBOX
    .mailbox.create = mailbox_create,
    .mailbox.delete = mailbox_delete,
    .mailbox.post = mailbox_post,
    .mailbox.read = mailbox_read,
    .mailbox.clear = mailbox_clear,
#endif

#if KLITE_CFG_OPT_MPOOL
    .mpool.create = mpool_create,
    .mpool.delete = mpool_delete,
    .mpool.alloc = mpool_alloc,
    .mpool.timed_alloc = mpool_timed_alloc,
    .mpool.blocked_alloc = mpool_blocked_alloc,
    .mpool.free = mpool_free,
#endif

#if KLITE_CFG_OPT_MSG_QUEUE
    .msg_queue.create = msg_queue_create,
    .msg_queue.delete = msg_queue_delete,
    .msg_queue.send = msg_queue_send,
    .msg_queue.recv = msg_queue_recv,
    .msg_queue.timed_send = msg_queue_timed_send,
    .msg_queue.timed_recv = msg_queue_timed_recv,
    .msg_queue.clear = msg_queue_clear,
    .msg_queue.count = msg_queue_count,
#endif

#if KLITE_CFG_OPT_SOFT_TIMER
    .soft_timer.init = soft_timer_init,
    .soft_timer.deinit = soft_timer_deinit,
    .soft_timer.create = soft_timer_create,
    .soft_timer.delete = soft_timer_delete,
    .soft_timer.start = soft_timer_start,
    .soft_timer.stop = soft_timer_stop,
#endif

#if KLITE_CFG_OPT_THREAD_POOL
    .thread_pool.create = thread_pool_create,
    .thread_pool.submit = thread_pool_submit,
    .thread_pool.shutdown = thread_pool_shutdown,
    .thread_pool.join = thread_pool_join,
    .thread_pool.pending_task = thread_pool_pending_task,
#endif

};

#endif /* KLITE_CFG_INTERFACE_ENABLE */

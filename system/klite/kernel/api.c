#include "klite.h"
#include "klite_api.h"

#if KLITE_CFG_INTERFACE_ENABLE
const klite_api_t klite = {
    .kernel_init = kernel_init,
    .kernel_start = kernel_start,
    .kernel_enter_critical = kernel_enter_critical,
    .kernel_exit_critical = kernel_exit_critical,
    .kernel_idle_time = kernel_idle_time,
    .kernel_tick_count = kernel_tick_count,
    .kernel_tick_count64 = kernel_tick_count64,
    .kernel_ms_to_ticks = kernel_ms_to_ticks,
    .kernel_ticks_to_ms = kernel_ticks_to_ms,
    .kernel_us_to_ticks = kernel_us_to_ticks,
    .kernel_ticks_to_us = kernel_ticks_to_us,

    .heap_alloc = heap_alloc,
    .heap_free = heap_free,
    .heap_realloc = heap_realloc,
    .heap_usage = heap_usage,
    .heap_usage_percent = heap_usage_percent,

    .thread_create = thread_create,
    .thread_delete = thread_delete,
    .thread_self = thread_self,
    .thread_suspend = thread_suspend,
    .thread_resume = thread_resume,
    .thread_sleep = thread_sleep,
    .thread_suspend_all = thread_suspend_all,
    .thread_resume_all = thread_resume_all,
    .thread_yield = thread_yield,
    .thread_exit = thread_exit,
    .thread_time = thread_time,
    .thread_get_priority = thread_get_priority,
    .thread_set_priority = thread_set_priority,
    .thread_stack_info = thread_stack_info,
    .thread_iter = thread_iter,

    .sem_create = sem_create,
    .sem_delete = sem_delete,
    .sem_post = sem_post,
    .sem_wait = sem_wait,
    .sem_timed_wait = sem_timed_wait,
    .sem_value = sem_value,
    .sem_reset = sem_reset,

    .event_create = event_create,
    .event_delete = event_delete,
    .event_set = event_set,
    .event_reset = event_reset,
    .event_wait = event_wait,
    .event_timed_wait = event_timed_wait,
    .event_is_set = event_is_set,

    .mutex_create = mutex_create,
    .mutex_delete = mutex_delete,
    .mutex_lock = mutex_lock,
    .mutex_unlock = mutex_unlock,
    .mutex_try_lock = mutex_try_lock,
    .mutex_timed_lock = mutex_timed_lock,

    .cond_create = cond_create,
    .cond_delete = cond_delete,
    .cond_signal = cond_signal,
    .cond_broadcast = cond_broadcast,
    .cond_wait = cond_wait,
    .cond_timed_wait = cond_timed_wait,

#if KLITE_CFG_OPT_EVENT_FLAGS
    .event_flags_create = event_flags_create,
    .event_flags_delete = event_flags_delete,
    .event_flags_set = event_flags_set,
    .event_flags_reset = event_flags_reset,
    .event_flags_wait = event_flags_wait,
    .event_flags_timed_wait = event_flags_timed_wait,
#endif

#if KLITE_CFG_OPT_MAILBOX
    .mailbox_create = mailbox_create,
    .mailbox_delete = mailbox_delete,
    .mailbox_post = mailbox_post,
    .mailbox_wait = mailbox_wait,
    .mailbox_clear = mailbox_clear,
#endif

#if KLITE_CFG_OPT_MPOOL
    .mpool_create = mpool_create,
    .mpool_delete = mpool_delete,
    .mpool_alloc = mpool_alloc,
    .mpool_free = mpool_free,
#endif

#if KLITE_CFG_OPT_MSG_QUEUE
    .msg_queue_create = msg_queue_create,
    .msg_queue_delete = msg_queue_delete,
    .msg_queue_send = msg_queue_send,
    .msg_queue_recv = msg_queue_recv,
    .msg_queue_clear = msg_queue_clear,
#endif

#if KLITE_CFG_OPT_SOFT_TIMER
    .soft_timer_init = soft_timer_init,
    .soft_timer_deinit = soft_timer_deinit,
    .soft_timer_create = soft_timer_create,
    .soft_timer_delete = soft_timer_delete,
    .soft_timer_start = soft_timer_start,
    .soft_timer_stop = soft_timer_stop,
#endif

#if KLITE_CFG_OPT_THREAD_POOL
    .thread_pool_create = thread_pool_create,
    .thread_pool_delete = thread_pool_delete,
    .thread_pool_submit = thread_pool_submit,
    .thread_pool_shutdown = thread_pool_shutdown,
    .thread_pool_join = thread_pool_join,
    .thread_pool_pending_task = thread_pool_pending_task,
#endif

};

#endif /* KLITE_CFG_INTERFACE_ENABLE */

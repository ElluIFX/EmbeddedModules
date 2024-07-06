#include "kl_blist.h"
#include "kl_priv.h"

static struct kl_thread_list m_list_alive;  // 运行中线程列表
static struct kl_thread_list m_list_dead;   // 待删除线程列表
static uint16_t kl_thread_id_counter;       // 线程ID计数器

#define THREAD_INFO_INVALID(tcb)                        \
    (!(tcb) || (tcb)->flags & KL_THREAD_FLAGS_EXITED || \
     (tcb)->magic != KL_THREAD_MAGIC_VALUE)
#define THREAD_OPERATION_INVALID(tcb) \
    (THREAD_INFO_INVALID(tcb) || (tcb)->prio == 0)

kl_thread_t kl_thread_self(void) {
    return (kl_thread_t)kl_sched_tcb_now;
}

kl_thread_t kl_thread_create(void (*entry)(void*), void* arg,
                             kl_size_t stack_size, uint32_t prio) {
    if (!entry) {
        KL_SET_ERRNO(KL_EINVAL);
        return NULL;
    }

    if (prio > KLITE_CFG_MAX_PRIO)
        prio = KLITE_CFG_MAX_PRIO;
    if (!prio && entry != kl_kernel_idle_entry)
        prio = KLITE_CFG_DEFAULT_PRIO;
    if (!stack_size)
        stack_size = KLITE_CFG_DEFAULT_STACK_SIZE;

#if KLITE_CFG_STACK_OVERFLOW_DETECT
    stack_size += 2 * KLITE_CFG_STACKOF_SIZE * sizeof(uint32_t);
#endif

    kl_thread_t tcb;
    uint8_t* stack_base;
    uint32_t* stack_magic;

    tcb = kl_heap_alloc(sizeof(struct kl_thread) + stack_size);
    if (tcb == NULL) {
        KL_SET_ERRNO(KL_ENOMEM);
        return NULL;
    }
    stack_base = (uint8_t*)(tcb + 1);
    stack_magic = (uint32_t*)stack_base;
    memset(tcb, 0, sizeof(struct kl_thread));
    do {
        *stack_magic++ = KL_STACK_MAGIC_VALUE;
    } while ((uint8_t*)stack_magic < stack_base + stack_size);

#if KLITE_CFG_STACK_OVERFLOW_DETECT
    stack_base += KLITE_CFG_STACKOF_SIZE * sizeof(uint32_t);
    stack_size -= 2 * KLITE_CFG_STACKOF_SIZE * sizeof(uint32_t);
#endif

    tcb->stack = kl_port_stack_init(stack_base, stack_base + stack_size,
                                    (void*)entry, arg, (void*)kl_thread_exit);

    tcb->magic = KL_THREAD_MAGIC_VALUE;
    tcb->stack_size = stack_size;
    tcb->prio = prio;
    tcb->entry = entry;
    tcb->node_wait.tcb = tcb;
    tcb->node_sched.tcb = tcb;
    tcb->node_manage.tcb = tcb;
    tcb->tid = kl_thread_id_counter++;

    kl_port_enter_critical();
    kl_blist_append(&m_list_alive, &tcb->node_manage);
    kl_sched_tcb_ready(tcb, false);
    kl_port_leave_critical();

    return (kl_thread_t)tcb;
}

void kl_thread_delete(kl_thread_t thread) {
    if (THREAD_OPERATION_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return;
    }
    if (thread == kl_sched_tcb_now)
        return kl_thread_exit();
    kl_port_enter_critical();
    kl_blist_remove(&m_list_alive, &thread->node_manage);
    kl_sched_tcb_remove(thread);
    kl_port_leave_critical();

#if KLITE_CFG_HEAP_AUTO_FREE
    kl_heap_auto_free(thread);
#endif
    thread->magic = 0;
    kl_heap_free(thread);
}

void kl_thread_suspend(kl_thread_t thread) {
    if (THREAD_OPERATION_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return;
    }
    kl_port_enter_critical();
    kl_sched_tcb_suspend(thread);
    if (thread == kl_sched_tcb_now)
        kl_sched_switch();
    kl_port_leave_critical();
}

void kl_thread_resume(kl_thread_t thread) {
    if (THREAD_OPERATION_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return;
    }
    kl_port_enter_critical();
    kl_sched_tcb_resume(thread);
    kl_sched_preempt(false);
    kl_port_leave_critical();
}

void kl_thread_yield(void) {
    kl_port_enter_critical();
    kl_sched_tcb_ready(kl_sched_tcb_now, false);
    kl_sched_switch();
    kl_port_leave_critical();
}

void kl_thread_sleep(kl_tick_t time) {
    if (!time)
        return kl_thread_yield();
    kl_port_enter_critical();
    kl_sched_tcb_sleep(kl_sched_tcb_now, time);
    kl_sched_switch();
    kl_port_leave_critical();
}

kl_tick_t kl_thread_time(kl_thread_t thread) {
    if (THREAD_INFO_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return KL_INVALID;
    }
    return thread->time;
}

bool kl_thread_join(kl_thread_t thread, kl_tick_t timeout) {
    if (THREAD_OPERATION_INVALID(thread)) {
        return true;  // 可能是已结束的线程
    }
    kl_port_enter_critical();
    kl_sched_tcb_timed_wait(kl_sched_tcb_now, &thread->list_join, timeout);
    kl_sched_switch();
    kl_port_leave_critical();
    KL_RET_CHECK_TIMEOUT();
}

void kl_thread_stack_info(kl_thread_t thread, kl_size_t* stack_free,
                          kl_size_t* stack_size) {
    if (THREAD_INFO_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return;
    }
    kl_size_t free = 0;
    uint32_t* stack_magic = (uint32_t*)(thread + 1);
#if KLITE_CFG_STACK_OVERFLOW_DETECT
    stack_magic += KLITE_CFG_STACKOF_SIZE;
#endif
    while (*stack_magic++ == KL_STACK_MAGIC_VALUE)
        free += 4;
    *stack_free = free;
    *stack_size = thread->stack_size;
}

void kl_thread_set_priority(kl_thread_t thread, uint32_t prio) {
    if (THREAD_INFO_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return;
    }
    if (!prio)
        prio = KLITE_CFG_DEFAULT_PRIO;
    if (prio > KLITE_CFG_MAX_PRIO)
        prio = KLITE_CFG_MAX_PRIO;
    kl_port_enter_critical();
    kl_sched_tcb_reset_prio(thread, prio);
    kl_sched_preempt(false);
    kl_port_leave_critical();
}

uint32_t kl_thread_priority(kl_thread_t thread) {
    if (THREAD_INFO_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return KL_INVALID;
    }
    return thread->prio;
}

void kl_thread_set_slice(kl_thread_t thread, kl_tick_t slice) {
#if KLITE_CFG_ROUND_ROBIN_SLICE
    if (THREAD_OPERATION_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return;
    }
    kl_port_enter_critical();
    // slice=0时实际上会运行1个tick
    thread->slice = slice > 0 ? slice - 1 : 0;
    thread->slice_tick = thread->slice;
    kl_port_leave_critical();
#else
    (void)thread;
    (void)slice;
    KL_SET_ERRNO(KL_ENOTSUP);
#endif  // KLITE_CFG_ROUND_ROBIN_SLICE
}

uint32_t kl_thread_id(kl_thread_t thread) {
    if (thread == NULL) {
        return 0;  // NULL is main thread
    }
    if (THREAD_INFO_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return KL_INVALID;
    }
    return thread->tid;
}

uint32_t kl_thread_flags(kl_thread_t thread) {
    if (THREAD_INFO_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return KL_INVALID;
    }
    return thread->flags;
}

kl_err_t kl_thread_errno(kl_thread_t thread) {
    if (THREAD_INFO_INVALID(thread)) {
        return KL_EINVAL;
    }
    kl_port_enter_critical();
    kl_err_t err = thread->err;
    thread->err = (uint8_t)KL_EOK;
    kl_port_leave_critical();
    return err;
}

void kl_thread_set_errno(kl_thread_t thread, kl_err_t errno) {
    if (THREAD_INFO_INVALID(thread)) {
        if (THREAD_INFO_INVALID(kl_sched_tcb_now)) {
            return;
        }
        thread = kl_sched_tcb_now;
        errno = KL_EINVAL;
    }
    kl_port_enter_critical();
    thread->err = (uint8_t)errno;
    kl_port_leave_critical();
}

kl_tick_t kl_thread_timeout(kl_thread_t thread) {
    if (THREAD_INFO_INVALID(thread)) {
        KL_SET_ERRNO(KL_EINVAL);
        return KL_INVALID;
    }
    return thread->timeout;
}

kl_thread_t kl_thread_find(uint32_t id) {
    kl_port_enter_critical();
    struct kl_thread_node* node = m_list_alive.head;
    while (node) {
        if (((kl_thread_t)node->tcb)->tid == id) {
            break;
        }
        node = node->next;
    }
    kl_port_leave_critical();
    return node ? (kl_thread_t)node->tcb : NULL;
}

kl_thread_t kl_thread_iter(kl_thread_t thread) {
    struct kl_thread_node* node = NULL;
    kl_port_enter_critical();
    if (thread) {
        node = thread->node_manage.next;
    } else {
        node = m_list_alive.head;
    }
    kl_port_leave_critical();
    return node ? (kl_thread_t)node->tcb : NULL;
}

void kl_thread_exit(void) {
    kl_port_enter_critical();
    kl_sched_tcb_remove(kl_sched_tcb_now);
    kl_blist_remove(&m_list_alive, &kl_sched_tcb_now->node_manage);
    kl_blist_append(&m_list_dead, &kl_sched_tcb_now->node_manage);
    kl_sched_switch();
    kl_port_leave_critical();
}

void kl_thread_idle_task(void) {
    struct kl_thread_node* node;
    while (m_list_dead.head) {
        kl_port_enter_critical();
        node = m_list_dead.head;
        kl_blist_remove(&m_list_dead, node);
        kl_port_leave_critical();

#if KLITE_CFG_HEAP_AUTO_FREE
        kl_heap_auto_free(node->tcb);
#endif
        node->tcb->magic = 0;
        kl_heap_free(node->tcb);
    }
}

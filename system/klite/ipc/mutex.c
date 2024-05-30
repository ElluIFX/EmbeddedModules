#include "kl_priv.h"

#if KLITE_CFG_OPT_MUTEX

kl_mutex_t kl_mutex_create(void) {
    kl_mutex_t mutex;
    mutex = kl_heap_alloc(sizeof(struct kl_mutex));
    if (mutex != NULL) {
        memset(mutex, 0, sizeof(struct kl_mutex));
    } else {
        KL_SET_ERRNO(KL_ENOMEM);
    }
    return mutex;
}

void kl_mutex_delete(kl_mutex_t mutex) {
    kl_heap_free(mutex);
}

bool kl_mutex_lock(kl_mutex_t mutex, kl_tick_t timeout) {
    kl_port_enter_critical();
    if (mutex->owner == NULL) {
        mutex->lock++;
        mutex->owner = kl_sched_tcb_now;
        kl_port_leave_critical();
        return true;
    }
    if (mutex->owner == kl_sched_tcb_now) {
        mutex->lock++;
        kl_port_leave_critical();
        return true;
    }
    if (timeout == 0) {
        kl_port_leave_critical();
        KL_SET_ERRNO(KL_ETIMEOUT);
        return false;
    }
    kl_sched_tcb_timed_wait(kl_sched_tcb_now, &mutex->list, timeout);
    kl_sched_switch();
    kl_port_leave_critical();
    if (!kl_sched_tcb_now->timeout) {
        KL_SET_ERRNO(KL_ETIMEOUT);
        return false;
    }
    if (kl_sched_tcb_now != mutex->owner) {
        KL_SET_ERRNO(KL_EPERM);
        return false;
    }
    return true;
}

void kl_mutex_unlock(kl_mutex_t mutex) {
    if (mutex->owner != kl_sched_tcb_now) {
        KL_SET_ERRNO(KL_EPERM);
        return;
    }
    kl_port_enter_critical();
    mutex->lock--;
    if (mutex->lock == 0) {
        mutex->owner = kl_sched_tcb_wake_from(&mutex->list);
        if (mutex->owner != NULL) {
            mutex->lock++;
            kl_sched_preempt(false);
        }
    }
    kl_port_leave_critical();
}

#endif

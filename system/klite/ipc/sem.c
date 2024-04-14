#include "kl_priv.h"

#if KLITE_CFG_OPT_SEM

kl_sem_t kl_sem_create(kl_size_t value) {
    kl_sem_t sem;
    sem = kl_heap_alloc(sizeof(struct kl_sem));
    if (sem != NULL) {
        memset(sem, 0, sizeof(struct kl_sem));
        sem->value = value;
    } else {
        KL_SET_ERRNO(KL_ENOMEM);
    }
    return sem;
}

void kl_sem_delete(kl_sem_t sem) {
    kl_heap_free(sem);
}

void kl_sem_give(kl_sem_t sem) {
    kl_port_enter_critical();
    if (kl_sched_tcb_wake_from(&sem->list)) {
        kl_sched_preempt(false);
        kl_port_leave_critical();
        return;
    }
    sem->value++;
    kl_port_leave_critical();
}

bool kl_sem_take(kl_sem_t sem, kl_tick_t timeout) {
    kl_port_enter_critical();
    if (sem->value > 0) {
        sem->value--;
        kl_port_leave_critical();
        return true;
    }
    if (timeout == 0) {
        kl_port_leave_critical();
        KL_SET_ERRNO(KL_ETIMEOUT);
        return false;
    }
    kl_sched_tcb_timed_wait(kl_sched_tcb_now, &sem->list, timeout);
    kl_sched_switch();
    kl_port_leave_critical();
    if (!kl_sched_tcb_now->timeout) {
        KL_SET_ERRNO(KL_ETIMEOUT);
        return false;
    }
    return true;
}

void kl_sem_reset(kl_sem_t sem, kl_size_t value) {
    kl_port_enter_critical();
    sem->value = value;
    kl_port_leave_critical();
}

kl_size_t kl_sem_value(kl_sem_t sem) {
    return sem->value;
}

#endif

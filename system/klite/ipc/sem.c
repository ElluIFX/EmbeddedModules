#include "kl_priv.h"

#if KLITE_CFG_OPT_SEM

kl_sem_t kl_sem_create(uint32_t value) {
  struct kl_sem *sem;
  sem = kl_heap_alloc(sizeof(struct kl_sem));
  if (sem != NULL) {
    memset(sem, 0, sizeof(struct kl_sem));
    sem->value = value;
  }
  return (kl_sem_t)sem;
}

void kl_sem_delete(kl_sem_t sem) { kl_heap_free(sem); }

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

void kl_sem_take(kl_sem_t sem) {
  kl_port_enter_critical();
  if (sem->value > 0) {
    sem->value--;
    kl_port_leave_critical();
    return;
  }
  kl_sched_tcb_wait(kl_sched_tcb_now, &sem->list);
  kl_sched_switch();
  kl_port_leave_critical();
}

void kl_sem_reset(kl_sem_t sem, uint32_t value) {
  kl_port_enter_critical();
  sem->value = value;
  kl_port_leave_critical();
}

bool kl_sem_try_take(kl_sem_t sem) {
  kl_port_enter_critical();
  if (sem->value > 0) {
    sem->value--;
    kl_port_leave_critical();
    return true;
  }
  kl_port_leave_critical();
  return false;
}

kl_tick_t kl_sem_timed_take(kl_sem_t sem, kl_tick_t timeout) {
  kl_port_enter_critical();
  if (sem->value > 0) {
    sem->value--;
    kl_port_leave_critical();
    return true;
  }
  if (timeout == 0) {
    kl_port_leave_critical();
    return false;
  }
  kl_sched_tcb_timed_wait(kl_sched_tcb_now, &sem->list, timeout);
  kl_sched_switch();
  kl_port_leave_critical();
  return kl_sched_tcb_now->timeout;
}

uint32_t kl_sem_value(kl_sem_t sem) { return sem->value; }

#endif

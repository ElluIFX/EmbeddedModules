#include "kl_priv.h"

#if KLITE_CFG_OPT_COND

kl_cond_t kl_cond_create(void) {
  kl_cond_t cond;
  cond = kl_heap_alloc(sizeof(struct kl_cond));
  if (cond != NULL) {
    memset(cond, 0, sizeof(struct kl_cond));
  } else {
    KL_SET_ERRNO(KL_ENOMEM);
  }
  return cond;
}

void kl_cond_delete(kl_cond_t cond) { kl_heap_free(cond); }

void kl_cond_signal(kl_cond_t cond) {
  kl_port_enter_critical();
  if (kl_sched_tcb_wake_from((struct kl_thread_list *)cond))
    kl_sched_preempt(false);
  kl_port_leave_critical();
}

void kl_cond_broadcast(kl_cond_t cond) {
  bool preempt = false;
  kl_port_enter_critical();
  while (kl_sched_tcb_wake_from((struct kl_thread_list *)cond)) preempt = true;
  if (preempt) kl_sched_preempt(false);
  kl_port_leave_critical();
}

bool kl_cond_wait(kl_cond_t cond, kl_mutex_t mutex, kl_tick_t timeout) {
  if (timeout == 0) {
    return false;
  }
  kl_port_enter_critical();
  kl_sched_tcb_timed_wait(kl_sched_tcb_now, (struct kl_thread_list *)cond,
                          timeout);
  kl_mutex_unlock(mutex);
  kl_sched_switch();
  kl_port_leave_critical();
  kl_mutex_lock(mutex, kl_sched_tcb_now->timeout);
  KL_RET_CHECK_TIMEOUT();
}

bool kl_cond_wait_complete(kl_cond_t cond, kl_tick_t timeout) {
  if (timeout == 0) {
    return false;
  }
  kl_port_enter_critical();
  kl_sched_tcb_timed_wait(kl_sched_tcb_now, (struct kl_thread_list *)cond,
                          timeout);
  kl_sched_switch();
  kl_port_leave_critical();
  KL_RET_CHECK_TIMEOUT();
}

#endif

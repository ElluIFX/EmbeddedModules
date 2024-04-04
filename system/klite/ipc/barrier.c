#include "kl_priv.h"

#if KLITE_CFG_OPT_BARRIER

kl_barrier_t kl_barrier_create(uint32_t target) {
  struct kl_barrier *barrier;
  barrier = kl_heap_alloc(sizeof(struct kl_barrier));
  if (barrier != NULL) {
    memset(barrier, 0, sizeof(struct kl_barrier));
    barrier->target = target;
    barrier->value = 0;
  }
  return (kl_barrier_t)barrier;
}

void kl_barrier_delete(kl_barrier_t barrier) { kl_heap_free(barrier); }

static bool kl_barrier_check(struct kl_barrier *barrier) {
  if (barrier->value >= barrier->target) {
    barrier->value = 0;
    while (kl_sched_tcb_wake_from(&barrier->list))
      ;
    kl_sched_preempt(false);
    return true;
  }
  return false;
}

void kl_barrier_set(kl_barrier_t barrier, uint32_t target) {
  kl_port_enter_critical();
  barrier->target = target;
  kl_barrier_check(barrier);
  kl_port_leave_critical();
}

uint32_t kl_barrier_get(kl_barrier_t barrier) { return barrier->value; }

void kl_barrier_wait(kl_barrier_t barrier) {
  kl_port_enter_critical();
  barrier->value++;
  if (!kl_barrier_check(barrier)) {
    kl_sched_tcb_wait(kl_sched_tcb_now, &barrier->list);
    kl_sched_switch();
  }
  kl_port_leave_critical();
}

#endif

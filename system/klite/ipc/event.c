#include "kl_priv.h"

#if KLITE_CFG_OPT_EVENT

kl_event_t kl_event_create(bool auto_reset) {
  struct kl_event *event;
  event = kl_heap_alloc(sizeof(struct kl_event));
  if (event != NULL) {
    memset(event, 0, sizeof(struct kl_event));
    event->auto_reset = auto_reset;
  }
  return (kl_event_t)event;
}

void kl_event_delete(kl_event_t event) { kl_heap_free(event); }

void kl_event_set(kl_event_t event) {
  kl_port_enter_critical();
  event->state = true;
  if (event->auto_reset) {
    if (kl_sched_tcb_wake_from(&event->list)) {
      event->state = false;
    }
  }
  while (kl_sched_tcb_wake_from(&event->list))
    ;
  kl_sched_preempt(false);
  kl_port_leave_critical();
}

void kl_event_reset(kl_event_t event) {
  kl_port_enter_critical();
  event->state = false;
  kl_port_leave_critical();
}

void kl_event_wait(kl_event_t event) {
  kl_port_enter_critical();
  if (event->state) {
    if (event->auto_reset) {
      event->state = false;
    }
    kl_port_leave_critical();
    return;
  }
  kl_sched_tcb_wait(kl_sched_tcb_now, &event->list);
  kl_sched_switch();
  kl_port_leave_critical();
}

bool kl_event_is_set(kl_event_t event) { return event->state; }

kl_tick_t kl_event_timed_wait(kl_event_t event, kl_tick_t timeout) {
  kl_port_enter_critical();
  if (event->state) {
    if (event->auto_reset) {
      event->state = false;
    }
    kl_port_leave_critical();
    return true;
  }
  if (timeout == 0) {
    kl_port_leave_critical();
    return false;
  }
  kl_sched_tcb_timed_wait(kl_sched_tcb_now, &event->list, timeout);
  kl_sched_switch();
  kl_port_leave_critical();
  return kl_sched_tcb_now->timeout;
}

#endif

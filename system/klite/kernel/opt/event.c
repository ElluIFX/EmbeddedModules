/******************************************************************************
 * Copyright (c) 2015-2023 jiangxiaogang<kerndev@foxmail.com>
 *
 * This file is part of KLite distribution.
 *
 * KLite is free software, you can redistribute it and/or modify it under
 * the MIT Licence.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "klite_internal.h"

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

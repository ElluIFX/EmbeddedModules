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

#if KLITE_CFG_OPT_COND

kl_cond_t kl_cond_create(void) {
  struct kl_cond *cond;
  cond = kl_heap_alloc(sizeof(struct kl_cond));
  if (cond != NULL) {
    memset(cond, 0, sizeof(struct kl_cond));
  }
  return (kl_cond_t)cond;
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

void kl_cond_wait(kl_cond_t cond, kl_mutex_t mutex) {
  kl_port_enter_critical();
  kl_sched_tcb_wait(kl_sched_tcb_now, (struct kl_thread_list *)cond);
  if (mutex) kl_mutex_unlock(mutex);
  kl_sched_switch();
  kl_port_leave_critical();
  if (mutex) kl_mutex_lock(mutex);
}

kl_tick_t kl_cond_timed_wait(kl_cond_t cond, kl_mutex_t mutex,
                             kl_tick_t timeout) {
  kl_port_enter_critical();
  if (timeout == 0) {
    kl_port_leave_critical();
    return false;
  }
  kl_sched_tcb_timed_wait(kl_sched_tcb_now, (struct kl_thread_list *)cond,
                          timeout);
  if (mutex) kl_mutex_unlock(mutex);
  kl_sched_switch();
  kl_port_leave_critical();
  if (mutex) kl_mutex_lock(mutex);
  return kl_sched_tcb_now->timeout;
}

#endif

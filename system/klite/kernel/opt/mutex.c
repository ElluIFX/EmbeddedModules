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

#if KLITE_CFG_OPT_MUTEX

kl_mutex_t kl_mutex_create(void) {
  struct kl_mutex *mutex;
  mutex = kl_heap_alloc(sizeof(struct kl_mutex));
  if (mutex != NULL) {
    memset(mutex, 0, sizeof(struct kl_mutex));
  }
  return (kl_mutex_t)mutex;
}

void kl_mutex_delete(kl_mutex_t mutex) { kl_heap_free(mutex); }

bool kl_mutex_try_lock(kl_mutex_t mutex) {
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
  kl_port_leave_critical();
  return false;
}

kl_tick_t kl_mutex_timed_lock(kl_mutex_t mutex, kl_tick_t timeout) {
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
    return false;
  }
  kl_sched_tcb_timed_wait(kl_sched_tcb_now, &mutex->list, timeout);
  kl_sched_switch();
  kl_port_leave_critical();
  return kl_sched_tcb_now->timeout;
}

void kl_mutex_lock(kl_mutex_t mutex) {
  kl_port_enter_critical();
  if (mutex->owner == NULL) mutex->owner = kl_sched_tcb_now;
  if (mutex->owner == kl_sched_tcb_now) {
    mutex->lock++;
    kl_port_leave_critical();
    return;
  }
  kl_sched_tcb_wait(kl_sched_tcb_now, &mutex->list);
  kl_sched_switch();
  kl_port_leave_critical();
  return;
}

void kl_mutex_unlock(kl_mutex_t mutex) {
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

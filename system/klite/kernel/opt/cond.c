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
#include "klite.h"

#if KLITE_CFG_OPT_COND

#include "klite_internal.h"

struct cond {
  struct tcb_list list;
};

cond_t cond_create(void) {
  struct cond *cond;
  cond = heap_alloc(sizeof(struct cond));
  if (cond != NULL) {
    memset(cond, 0, sizeof(struct cond));
  }
  return (cond_t)cond;
}

void cond_delete(cond_t cond) { heap_free(cond); }

void cond_signal(cond_t cond) {
  cpu_enter_critical();
  if (sched_tcb_wake_from((struct tcb_list *)cond)) sched_preempt(false);
  cpu_leave_critical();
}

void cond_broadcast(cond_t cond) {
  bool preempt = false;
  cpu_enter_critical();
  while (sched_tcb_wake_from((struct tcb_list *)cond)) preempt = true;
  if (preempt) sched_preempt(false);
  cpu_leave_critical();
}

void cond_wait(cond_t cond, mutex_t mutex) {
  cpu_enter_critical();
  sched_tcb_wait(sched_tcb_now, (struct tcb_list *)cond);
  if (mutex) mutex_unlock(mutex);
  sched_switch();
  cpu_leave_critical();
  if (mutex) mutex_lock(mutex);
}

klite_tick_t cond_timed_wait(cond_t cond, mutex_t mutex, klite_tick_t timeout) {
  cpu_enter_critical();
  if (timeout == 0) {
    cpu_leave_critical();
    return false;
  }
  sched_tcb_timed_wait(sched_tcb_now, (struct tcb_list *)cond, timeout);
  if (mutex) mutex_unlock(mutex);
  sched_switch();
  cpu_leave_critical();
  if (mutex) mutex_lock(mutex);
  return sched_tcb_now->timeout;
}

#endif

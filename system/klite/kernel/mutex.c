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
#include "internal.h"
#include "kernel.h"

struct mutex {
  struct tcb_list list;
  struct tcb *owner;
  uint32_t lock;
};

mutex_t mutex_create(void) {
  struct mutex *mutex;
  mutex = heap_alloc(sizeof(struct mutex));
  if (mutex != NULL) {
    memset(mutex, 0, sizeof(struct mutex));
  }
  return (mutex_t)mutex;
}

void mutex_delete(mutex_t mutex) { heap_free(mutex); }

bool mutex_try_lock(mutex_t mutex) {
  cpu_enter_critical();
  if (mutex->owner == NULL) {
    mutex->lock++;
    mutex->owner = sched_tcb_now;
    cpu_leave_critical();
    return true;
  }
  if (mutex->owner == sched_tcb_now) {
    mutex->lock++;
    cpu_leave_critical();
    return true;
  }
  cpu_leave_critical();
  return false;
}

void mutex_lock(mutex_t mutex) {
  cpu_enter_critical();
  if (mutex->owner == NULL) mutex->owner = sched_tcb_now;
  if (mutex->owner == sched_tcb_now) {
    mutex->lock++;
    cpu_leave_critical();
    return;
  }
  sched_tcb_wait(sched_tcb_now, &mutex->list);
  sched_switch();
  cpu_leave_critical();
  return;
}

void mutex_unlock(mutex_t mutex) {
  cpu_enter_critical();
  mutex->lock--;
  if (mutex->lock == 0) {
    mutex->owner = sched_tcb_wake_from(&mutex->list);
    if (mutex->owner != NULL) {
      mutex->lock++;
      sched_preempt(false);
    }
  }
  cpu_leave_critical();
}

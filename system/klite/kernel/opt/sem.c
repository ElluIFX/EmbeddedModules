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

#if KLITE_CFG_OPT_SEM

#include "klite_internal.h"

struct sem {
  struct tcb_list list;
  uint32_t value;
};

sem_t sem_create(uint32_t value) {
  struct sem *sem;
  sem = heap_alloc(sizeof(struct sem));
  if (sem != NULL) {
    memset(sem, 0, sizeof(struct sem));
    sem->value = value;
  }
  return (sem_t)sem;
}

void sem_delete(sem_t sem) { heap_free(sem); }

void sem_give(sem_t sem) {
  cpu_enter_critical();
  if (sched_tcb_wake_from(&sem->list)) {
    sched_preempt(false);
    cpu_leave_critical();
    return;
  }
  sem->value++;
  cpu_leave_critical();
}

void sem_take(sem_t sem) {
  cpu_enter_critical();
  if (sem->value > 0) {
    sem->value--;
    cpu_leave_critical();
    return;
  }
  sched_tcb_wait(sched_tcb_now, &sem->list);
  sched_switch();
  cpu_leave_critical();
}

void sem_reset(sem_t sem, uint32_t value) {
  cpu_enter_critical();
  sem->value = value;
  cpu_leave_critical();
}

bool sem_try_take(sem_t sem) {
  cpu_enter_critical();
  if (sem->value > 0) {
    sem->value--;
    cpu_leave_critical();
    return true;
  }
  cpu_leave_critical();
  return false;
}

klite_tick_t sem_timed_take(sem_t sem, klite_tick_t timeout) {
  cpu_enter_critical();
  if (sem->value > 0) {
    sem->value--;
    cpu_leave_critical();
    return true;
  }
  if (timeout == 0) {
    cpu_leave_critical();
    return false;
  }
  sched_tcb_timed_wait(sched_tcb_now, &sem->list, timeout);
  sched_switch();
  cpu_leave_critical();
  return sched_tcb_now->timeout;
}

uint32_t sem_value(sem_t sem) { return sem->value; }

#endif

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

#if KLITE_CFG_OPT_BARRIER

#include "klite_internal.h"

struct kl_barrier {
  struct kl_tcb_list list;
  uint32_t value;
  uint32_t target;
};

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
    while (sched_tcb_wake_from(&barrier->list))
      ;
    sched_preempt(false);
    return true;
  }
  return false;
}

void kl_barrier_set(kl_barrier_t barrier, uint32_t target) {
  cpu_enter_critical();
  barrier->target = target;
  kl_barrier_check(barrier);
  cpu_leave_critical();
}

uint32_t kl_barrier_get(kl_barrier_t barrier) { return barrier->value; }

void kl_barrier_wait(kl_barrier_t barrier) {
  cpu_enter_critical();
  barrier->value++;
  if (!kl_barrier_check(barrier)) {
    sched_tcb_wait(sched_tcb_now, &barrier->list);
    sched_switch();
  }
  cpu_leave_critical();
}

#endif

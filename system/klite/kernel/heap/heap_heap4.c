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
#if KLITE_CFG_HEAP_USE_HEAP4
#include <string.h>

#include "heap4.h"

volatile static uint8_t heap_lock = 0;
static struct kl_thread_list heap_waitlist;
static void heap_kl_mutex_lock(void) {
  cpu_enter_critical();
  if (!heap_lock) {
    heap_lock = 1;
  } else {
    sched_tcb_wait(sched_tcb_now, &heap_waitlist);
    sched_switch();
  }
  cpu_leave_critical();
}

static void heap_kl_mutex_unlock(void) {
  cpu_enter_critical();
  if (sched_tcb_wake_from(&heap_waitlist)) {
    sched_preempt(false);
  } else {
    heap_lock = 0;
  }
  cpu_leave_critical();
}

void kl_heap_init(void *addr, uint32_t size) {
  prvHeapInit((uint8_t *)addr, size);
}

void *kl_heap_alloc(uint32_t size) {
  heap_kl_mutex_lock();
  void *mem = pvPortMalloc(size);
  if (!mem) mem = kl_heap_alloc_fault_callback(size);
  heap_kl_mutex_unlock();
  return mem;
}

void kl_heap_free(void *mem) {
  heap_kl_mutex_lock();
  vPortFree(mem);
  heap_kl_mutex_unlock();
}

void *kl_heap_realloc(void *mem, uint32_t size) {
  heap_kl_mutex_lock();
  void *new_mem = pvPortRealloc(mem, size);
  if (!new_mem) {
    new_mem = kl_heap_alloc_fault_callback(size);
    if (new_mem) {
      memmove(new_mem, mem, size);
      vPortFree(mem);
    }
  }
  heap_kl_mutex_unlock();
  return new_mem;
}

void kl_heap_info(uint32_t *used, uint32_t *free) {
  heap_kl_mutex_lock();
  *free = xPortGetFreeHeapSize();
  *used = xPortGetTotalHeapSize() - *free;
  heap_kl_mutex_unlock();
}

float kl_heap_usage(void) {
  heap_kl_mutex_lock();
  float usage = (float)(xPortGetTotalHeapSize() - xPortGetFreeHeapSize()) /
                xPortGetTotalHeapSize();
  heap_kl_mutex_unlock();
  return usage;
}

#endif

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
#if KERNEL_CFG_HEAP_USE_HEAP4
#include "heap_4.h"

volatile static uint8_t heap_lock = 0;
static struct tcb_list heap_tcb;
static void heap_mutex_lock(void) {
  cpu_enter_critical();
  if (heap_lock == 0) {
    heap_lock = 1;
  } else {
    sched_tcb_wait(sched_tcb_now, &heap_tcb);
    sched_switch();
  }
  cpu_leave_critical();
}

static void heap_mutex_unlock(void) {
  cpu_enter_critical();
  if (sched_tcb_wake_from(&heap_tcb)) {
    sched_preempt(false);
  } else {
    heap_lock = 0;
  }
  cpu_leave_critical();
}

void heap_create(void *addr, uint32_t size) {
  prvHeapInit((uint8_t *)addr, size);
}

void *heap_alloc(uint32_t size) {
  heap_mutex_lock();
  void *mem = pvPortMalloc(size);
#if KERNEL_CFG_HOOK_ENABLE
  if (mem == NULL) {
    kernel_hook_heap_fault(size);
  } else {
    kernel_hook_heap_operation(mem, NULL, size, KERNEL_HEAP_OP_ALLOC);
  }
#endif
  heap_mutex_unlock();
  return mem;
}

void heap_free(void *mem) {
  heap_mutex_lock();
  vPortFree(mem);
#if KERNEL_CFG_HOOK_ENABLE
  kernel_hook_heap_operation(mem, NULL, 0, KERNEL_HEAP_OP_FREE);
#endif
  heap_mutex_unlock();
}

void *heap_realloc(void *mem, uint32_t size) {
  heap_mutex_lock();
  void *new_mem = pvPortRealloc(mem, size);
#if KERNEL_CFG_HOOK_ENABLE
  if (new_mem == NULL) {
    kernel_hook_heap_fault(size);
  } else {
    kernel_hook_heap_operation(mem, new_mem, size, KERNEL_HEAP_OP_REALLOC);
  }
#endif
  heap_mutex_unlock();
  return new_mem;
}

void heap_usage(uint32_t *used, uint32_t *free) {
  heap_mutex_lock();
  *free = xPortGetFreeHeapSize();
  *used = xPortGetTotalHeapSize() - *free;
  heap_mutex_unlock();
}

float heap_usage_percent(void) {
  heap_mutex_lock();
  float usage = (float)(xPortGetTotalHeapSize() - xPortGetFreeHeapSize()) /
                xPortGetTotalHeapSize();
  heap_mutex_unlock();
  return usage;
}

#endif

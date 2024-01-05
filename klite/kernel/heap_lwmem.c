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
#if HEAP_USE_LWMEM
#include "log.h"
#include "lwmem.h"

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

static lwmem_region_t regions[] = {
    /* Set start address and size of each region */
    {NULL, 0},
    {NULL, 0},
};

void heap_create(void *addr, uint32_t size) {
  regions[0].start_addr = addr;
  regions[0].size = size;
  lwmem_assignmem(regions);
}

__weak void heap_fault_handler(void) { LOG_ERROR("heap fault"); }

void *heap_alloc(uint32_t size) {
  heap_mutex_lock();
  void *mem = lwmem_malloc(size);
  if (mem == NULL) {
    heap_fault_handler();
  }
  heap_mutex_unlock();
  return mem;
}

void heap_free(void *mem) {
  heap_mutex_lock();
  lwmem_free(mem);
  heap_mutex_unlock();
}

void *heap_realloc(void *mem, uint32_t size) {
  heap_mutex_lock();
  void *new_mem = lwmem_realloc(mem, size);
  if (new_mem == NULL) {
    heap_fault_handler();
  }
  heap_mutex_unlock();
  return new_mem;
}
lwmem_stats_t stats;
void heap_usage(uint32_t *used, uint32_t *free) {
  lwmem_get_stats(&stats);
  *free = stats.mem_available_bytes;
  *used = stats.mem_size_bytes - stats.mem_available_bytes;
}

float heap_usage_percent(void) {
  lwmem_get_stats(&stats);
  return (float)(stats.mem_size_bytes - stats.mem_available_bytes) /
         (float)stats.mem_size_bytes;
}

#endif  // HEAP_USE_LWMEM

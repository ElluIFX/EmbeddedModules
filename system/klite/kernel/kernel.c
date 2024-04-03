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
#include "klite_internal.h"

#define MAKE_VERSION_CODE(a, b, c) ((a << 24) | (b << 16) | (c))
#define KERNEL_VERSION_CODE MAKE_VERSION_CODE(5, 1, 0)

static uint64_t m_tick_count;
static kl_thread_t m_idle_thread;

void kl_kernel_init(void* heap_addr, uint32_t heap_size) {
  m_tick_count = 0;
  m_idle_thread = NULL;
  cpu_sys_init();
  sched_init();
  kl_heap_init(heap_addr, heap_size);

  /* 创建idle线程 */
  m_idle_thread = kl_thread_create(kernel_idle_thread, NULL,
                                   KLITE_CFG_IDLE_THREAD_STACK_SIZE, 0);
}

void kl_kernel_start(void) {
  sched_switch();
  cpu_sys_start();
}

void kernel_idle_thread(void* args) {
  (void)args;

  while (1) {
    thread_clean_up();
    cpu_enter_critical();
    sched_idle();
    cpu_leave_critical();
  }
}

kl_tick_t kl_kernel_idle_time(void) {
  return m_idle_thread ? kl_thread_time(m_idle_thread) : 0;
}

void kl_kernel_enter_critical(void) { cpu_enter_critical(); }

void kl_kernel_exit_critical(void) { cpu_leave_critical(); }

void kl_kernel_tick_source(uint32_t time) {
  m_tick_count += time;
  cpu_enter_critical();
  sched_timing(time);
  sched_preempt(true);
  cpu_leave_critical();
}

kl_tick_t kl_kernel_tick_count(void) { return (kl_tick_t)m_tick_count; }

uint64_t kl_kernel_tick_count64(void) { return m_tick_count; }

uint32_t kl_kernel_version(void) { return KERNEL_VERSION_CODE; }

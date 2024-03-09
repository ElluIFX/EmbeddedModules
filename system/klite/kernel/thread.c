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
#include "list.h"

static struct tcb_list m_list_dead;

thread_t thread_self(void) { return (thread_t)sched_tcb_now; }

thread_t thread_create(void (*entry)(void *), void *arg, uint32_t stack_size,
                       uint32_t prio) {
  if (!prio) prio = THREAD_PRIORITY_NORMAL;
  if (prio >= __THREAD_PRIORITY_MAX__) prio = __THREAD_PRIORITY_MAX__ - 1;
  if (!entry) return NULL;
  struct tcb *tcb;
  uint8_t *stack_base;
  stack_size = stack_size ? stack_size : 1024;
  tcb = heap_alloc(sizeof(struct tcb) + stack_size);
  if (tcb == NULL) {
    return NULL;
  }
  stack_base = (uint8_t *)(tcb + 1);
  memset(tcb, 0, sizeof(struct tcb));
  memset(stack_base, 0xCC, stack_size);
  tcb->prio = prio;
  tcb->stack = cpu_contex_init(stack_base, stack_base + stack_size,
                               (void *)entry, arg, (void *)thread_exit);
  tcb->stack_size = stack_size;
  tcb->entry = entry;
  tcb->node_wait.tcb = tcb;
  tcb->node_sched.tcb = tcb;
  cpu_enter_critical();
#if KERNEL_CFG_HOOK_ENABLE
  kernel_hook_thread_create(tcb);
#endif
  sched_tcb_ready(tcb);
  cpu_leave_critical();
  return (thread_t)tcb;
}

void thread_delete(thread_t thread) {
  if (thread == sched_tcb_now) return thread_exit();
  cpu_enter_critical();
#if KERNEL_CFG_HOOK_ENABLE
  kernel_hook_thread_delete(thread);
#endif
  sched_tcb_remove(thread);
  cpu_leave_critical();
  heap_free(thread);
}

void thread_suspend(thread_t thread) {
  cpu_enter_critical();
#if KERNEL_CFG_HOOK_ENABLE
  kernel_hook_thread_suspend(thread);
#endif
  sched_tcb_remove(thread);
  if (thread == sched_tcb_now) sched_switch();
  cpu_leave_critical();
}

void thread_resume(thread_t thread) {
  cpu_enter_critical();
#if KERNEL_CFG_HOOK_ENABLE
  kernel_hook_thread_resume(thread);
#endif
  sched_tcb_ready(thread);
  cpu_leave_critical();
}

void thread_yield(void) {
  cpu_enter_critical();
  sched_switch();
  sched_tcb_ready(sched_tcb_now);
  cpu_leave_critical();
}

void thread_sleep(uint32_t time) {
  cpu_enter_critical();
#if KERNEL_CFG_HOOK_ENABLE
  kernel_hook_thread_sleep(sched_tcb_now, time);
#endif
  sched_tcb_sleep(sched_tcb_now, time);
  sched_switch();
  cpu_leave_critical();
}

uint32_t thread_time(thread_t thread) { return thread->time; }

void thread_stack_info(thread_t thread, size_t *stack_free,
                       size_t *stack_size) {
  uint32_t free = 0;
  uint8_t *stack = (uint8_t *)(thread + 1);
  while (*stack++ == 0xCC) free++;
  *stack_free = free;
  *stack_size = thread->stack_size;
}

void thread_set_priority(thread_t thread, uint32_t prio) {
  if (prio >= __THREAD_PRIORITY_MAX__) prio = __THREAD_PRIORITY_MAX__ - 1;
  cpu_enter_critical();
  sched_tcb_reset(thread, prio);
  thread->prio = prio;
  sched_preempt(false);
#if KERNEL_CFG_HOOK_ENABLE
  kernel_hook_thread_prio_change(thread, prio);
#endif
  cpu_leave_critical();
}

uint32_t thread_get_priority(thread_t thread) { return thread->prio; }

void thread_exit(void) {
  cpu_enter_critical();
#if KERNEL_CFG_HOOK_ENABLE
  kernel_hook_thread_delete(sched_tcb_now);
#endif
  sched_tcb_remove(sched_tcb_now);
  list_append(&m_list_dead, &sched_tcb_now->node_wait);
  sched_switch();
  cpu_leave_critical();
}

void thread_clean_up(void) {
  struct tcb_node *node;
  while (m_list_dead.head) {
    cpu_enter_critical();
    node = m_list_dead.head;
    list_remove(&m_list_dead, node);
    cpu_leave_critical();
    heap_free(node->tcb);
  }
}

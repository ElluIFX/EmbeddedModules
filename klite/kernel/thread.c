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

thread_t thread_create(void (*entry)(void *), void *arg, uint32_t stack_size) {
  struct tcb *tcb;
  uint8_t *stack_base;
  stack_size = stack_size ? stack_size : 1024;
  tcb = heap_alloc( sizeof(struct tcb) + stack_size);
  if (tcb == NULL) {
    return NULL;
  }
  stack_base = (uint8_t *)(tcb + 1);
  memset(tcb, 0, sizeof(struct tcb));
  memset(stack_base, 0xCC, stack_size);
  tcb->prio = THREAD_PRIORITY_NORMAL;
  tcb->stack = cpu_contex_init(stack_base, stack_base + stack_size,
                               (void *)entry, arg, (void *)thread_exit);
  tcb->entry = entry;
  tcb->node_wait.tcb = tcb;
  tcb->node_sched.tcb = tcb;
  cpu_enter_critical();
  sched_tcb_ready(tcb);
  cpu_leave_critical();
  return (thread_t)tcb;
}

void thread_delete(thread_t thread) {
  cpu_enter_critical();
  sched_tcb_remove(thread);
  cpu_leave_critical();
  heap_free( thread);
}

void thread_yield(void) {
  cpu_enter_critical();
  sched_switch();
  sched_tcb_ready(sched_tcb_now);
  cpu_leave_critical();
}

void thread_sleep(uint32_t time) {
  cpu_enter_critical();
  sched_tcb_sleep(sched_tcb_now, time);
  sched_switch();
  cpu_leave_critical();
}

uint32_t thread_time(thread_t thread) { return thread->time; }

void thread_set_priority(thread_t thread, uint32_t prio) {
  cpu_enter_critical();
  sched_tcb_reset(thread, prio);
  thread->prio = prio;
  sched_preempt(false);
  cpu_leave_critical();
}

uint32_t thread_get_priority(thread_t thread) { return thread->prio; }

void thread_exit(void) {
  cpu_enter_critical();
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
    heap_free( node->tcb);
  }
}

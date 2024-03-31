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
#include "klite_internal_list.h"

static struct tcb_list m_list_alive;  // 运行中线程列表
static struct tcb_list m_list_dead;   // 待删除线程列表

thread_t thread_self(void) { return (thread_t)sched_tcb_now; }

thread_t thread_create(void (*entry)(void *), void *arg, uint32_t stack_size,
                       uint32_t prio) {
  if (!entry) return NULL;

  if (prio > KLITE_CFG_MAX_PRIO) prio = KLITE_CFG_MAX_PRIO;
  if (!prio && entry != kernel_idle_thread) prio = KLITE_CFG_DEFAULT_PRIO;
  if (!stack_size) stack_size = KLITE_CFG_DEFAULT_STACK_SIZE;

  struct tcb *tcb;
  uint8_t *stack_base;
  tcb = heap_alloc(sizeof(struct tcb) + stack_size);
  if (tcb == NULL) {
    return NULL;
  }
  stack_base = (uint8_t *)(tcb + 1);
  memset(tcb, 0, sizeof(struct tcb));
  memset(stack_base, STACK_MAGIC_VALUE, stack_size);
  tcb->prio = prio;
  tcb->stack = cpu_contex_init(stack_base, stack_base + stack_size,
                               (void *)entry, arg, (void *)thread_exit);
  tcb->stack_size = stack_size;
  tcb->entry = entry;
  tcb->node_wait.tcb = tcb;
  tcb->node_sched.tcb = tcb;
  tcb->node_manage.tcb = tcb;
  cpu_enter_critical();
#if KLITE_CFG_HOOK_ENABLE
  thread_hook_create(tcb);
#endif
  list_prepend(&m_list_alive, &tcb->node_manage);
  sched_tcb_ready(tcb);
  cpu_leave_critical();
  return (thread_t)tcb;
}

void thread_delete(thread_t thread) {
  if (!thread) thread = sched_tcb_now;
  if (thread == sched_tcb_now) return thread_exit();
  cpu_enter_critical();
#if KLITE_CFG_HOOK_ENABLE
  thread_hook_delete(thread);
#endif
  list_remove(&m_list_alive, &thread->node_manage);
  sched_tcb_remove(thread);
  cpu_leave_critical();
  heap_free(thread);
}

void thread_suspend(thread_t thread) {
  if (!thread) thread = sched_tcb_now;
  cpu_enter_critical();
#if KLITE_CFG_HOOK_ENABLE
  thread_hook_suspend(thread);
#endif
  sched_tcb_remove(thread);
  if (thread == sched_tcb_now) sched_switch();
  cpu_leave_critical();
}

void thread_resume(thread_t thread) {
  if (!thread) thread = sched_tcb_now;  // not possible
  cpu_enter_critical();
#if KLITE_CFG_HOOK_ENABLE
  thread_hook_resume(thread);
#endif
  sched_tcb_ready(thread);
  cpu_leave_critical();
}

void thread_suspend_all(void) {
  cpu_enter_critical();
  sched_susp_nesting++;
  cpu_leave_critical();
}

void thread_resume_all(void) {
  if (sched_susp_nesting == 0) return;
  cpu_enter_critical();
  if (--sched_susp_nesting == 0) {
    sched_preempt(false);
  }
  cpu_leave_critical();
}

void thread_yield(void) {
  cpu_enter_critical();
  sched_switch();
  sched_tcb_ready(sched_tcb_now);
  cpu_leave_critical();
}

void thread_sleep(uint32_t time) {
  if (sched_susp_nesting) {  // 挂起状态直接死等
    uint64_t start = kernel_tick_count64();
    while (kernel_tick_count64() - start < time) {
      cpu_sys_sleep(kernel_tick_count64() - start);
      if (!sched_susp_nesting) {
        return thread_sleep(time - (kernel_tick_count64() - start));
      }
    }
    return;
  }
  if (!time) return thread_yield();
  cpu_enter_critical();
#if KLITE_CFG_HOOK_ENABLE
  thread_hook_sleep(sched_tcb_now, time);
#endif
  sched_tcb_sleep(sched_tcb_now, time);
  sched_switch();
  cpu_leave_critical();
}

uint32_t thread_time(thread_t thread) { return thread->time; }

void thread_stack_info(thread_t thread, size_t *stack_free,
                       size_t *stack_size) {
  if (!thread) thread = sched_tcb_now;
  uint32_t free = 0;
  uint8_t *stack = (uint8_t *)(thread + 1);
  while (*stack++ == STACK_MAGIC_VALUE) free++;
  *stack_free = free;
  *stack_size = thread->stack_size;
}

void thread_set_priority(thread_t thread, uint32_t prio) {
  if (!thread) thread = sched_tcb_now;
  if (!prio) prio = KLITE_CFG_DEFAULT_PRIO;
  if (prio > KLITE_CFG_MAX_PRIO) prio = KLITE_CFG_MAX_PRIO;
  cpu_enter_critical();
  sched_tcb_reset_prio(thread, prio);
  sched_preempt(false);
#if KLITE_CFG_HOOK_ENABLE
  thread_hook_prio_change(thread, prio);
#endif
  cpu_leave_critical();
}

uint32_t thread_get_priority(thread_t thread) {
  if (!thread) thread = sched_tcb_now;
  return thread->prio;
}

thread_t thread_iter(thread_t thread) {
  struct tcb_node *node = NULL;
  cpu_enter_critical();
  if (thread) {
    node = thread->node_manage.next;
  } else {
    node = m_list_alive.head;
  }
  cpu_leave_critical();
  return node ? (thread_t)node->tcb : NULL;
}

void thread_exit(void) {
  cpu_enter_critical();
#if KLITE_CFG_HOOK_ENABLE
  thread_hook_delete(sched_tcb_now);
#endif
  sched_tcb_remove(sched_tcb_now);
  list_remove(&m_list_alive, &sched_tcb_now->node_manage);
  list_append(&m_list_dead, &sched_tcb_now->node_manage);
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

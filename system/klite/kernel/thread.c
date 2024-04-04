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
#include "klite_list.h"

static struct kl_thread_list m_list_manage;  // 运行中线程列表
static struct kl_thread_list m_list_dead;    // 待删除线程列表
static uint32_t kl_thread_id_counter;        // 线程ID计数器

kl_thread_t kl_thread_self(void) { return (kl_thread_t)kl_sched_tcb_now; }

kl_thread_t kl_thread_create(void (*entry)(void *), void *arg,
                             kl_size_t stack_size, uint32_t prio) {
  if (!entry) return NULL;

  if (prio > KLITE_CFG_MAX_PRIO) prio = KLITE_CFG_MAX_PRIO;
  if (!prio && entry != kl_kernel_idle_entry) prio = KLITE_CFG_DEFAULT_PRIO;
  if (!stack_size) stack_size = KLITE_CFG_DEFAULT_STACK_SIZE;

  kl_thread_t tcb;
  uint8_t *stack_base;
  tcb = kl_heap_alloc(sizeof(struct kl_thread) + stack_size);
  if (tcb == NULL) {
    return NULL;
  }
  stack_base = (uint8_t *)(tcb + 1);
  memset(tcb, 0, sizeof(struct kl_thread));
  memset(stack_base, STACK_MAGIC_VALUE, stack_size);
  tcb->prio = prio;
  tcb->stack = kl_port_stack_init(stack_base, stack_base + stack_size,
                                  (void *)entry, arg, (void *)kl_thread_exit);
  tcb->stack_size = stack_size;
  tcb->entry = entry;
  tcb->node_wait.tcb = tcb;
  tcb->node_sched.tcb = tcb;
  tcb->node_manage.tcb = tcb;
  tcb->id_flags = (kl_thread_id_counter++) << 8;  // 高24位
  kl_port_enter_critical();
  list_prepend(&m_list_manage, &tcb->node_manage);
  kl_sched_tcb_ready(tcb);
  kl_port_leave_critical();
  return (kl_thread_t)tcb;
}

void kl_thread_delete(kl_thread_t thread) {
  if (!thread) return;
  if (thread == kl_sched_tcb_now) return kl_thread_exit();
  kl_port_enter_critical();
  list_remove(&m_list_manage, &thread->node_manage);
  kl_sched_tcb_remove(thread);
  kl_port_leave_critical();
  kl_heap_free(thread);
}

void kl_thread_suspend(kl_thread_t thread) {
  if (!thread) return;
  kl_port_enter_critical();
  kl_sched_tcb_suspend(thread);
  if (thread == kl_sched_tcb_now) kl_sched_switch();
  kl_port_leave_critical();
}

void kl_thread_resume(kl_thread_t thread) {
  if (!thread) return;
  kl_port_enter_critical();
  kl_sched_tcb_resume(thread);
  kl_sched_preempt(false);
  kl_port_leave_critical();
}

void kl_thread_yield(void) {
  kl_port_enter_critical();
  kl_sched_switch();
  kl_sched_tcb_ready(kl_sched_tcb_now);
  kl_port_leave_critical();
}

void kl_thread_sleep(kl_tick_t time) {
  if (!time) return kl_thread_yield();
  kl_port_enter_critical();
  kl_sched_tcb_sleep(kl_sched_tcb_now, time);
  kl_sched_switch();
  kl_port_leave_critical();
}

kl_tick_t kl_thread_time(kl_thread_t thread) { return thread->time; }

void kl_thread_stack_info(kl_thread_t thread, kl_size_t *stack_free,
                          kl_size_t *stack_size) {
  if (!thread) return;
  kl_size_t free = 0;
  uint8_t *stack = (uint8_t *)(thread + 1);
  while (*stack++ == STACK_MAGIC_VALUE) free++;
  *stack_free = free;
  *stack_size = thread->stack_size;
}

void kl_thread_set_priority(kl_thread_t thread, uint32_t prio) {
  if (!thread) return;
  if (!prio) prio = KLITE_CFG_DEFAULT_PRIO;
  if (prio > KLITE_CFG_MAX_PRIO) prio = KLITE_CFG_MAX_PRIO;
  kl_port_enter_critical();
  kl_sched_tcb_reset_prio(thread, prio);
  kl_sched_preempt(false);
  kl_port_leave_critical();
}

uint32_t kl_thread_get_priority(kl_thread_t thread) {
  if (!thread) return 0xFFFFFFFF;
  return thread->prio;
}

uint32_t kl_thread_id(kl_thread_t thread) {
  if (!thread) return 0xFFFFFFFF;
  return thread->id_flags >> 8;  // 高24位
}

uint8_t kl_thread_flags(kl_thread_t thread) {
  if (!thread) return 0xFF;
  return thread->id_flags & 0xff;  // 低8位
}

kl_thread_t kl_thread_find(uint32_t id) {
  kl_port_enter_critical();
  struct kl_thread_node *node = m_list_manage.head;
  while (node) {
    if (((kl_thread_t)node->tcb)->id_flags >> 8 == id) {
      break;
    }
    node = node->next;
  }
  kl_port_leave_critical();
  return node ? (kl_thread_t)node->tcb : NULL;
}

kl_thread_t kl_thread_iter(kl_thread_t thread) {
  struct kl_thread_node *node = NULL;
  kl_port_enter_critical();
  if (thread) {
    node = thread->node_manage.next;
  } else {
    node = m_list_manage.head;
  }
  kl_port_leave_critical();
  return node ? (kl_thread_t)node->tcb : NULL;
}

void kl_thread_exit(void) {
  kl_port_enter_critical();
  kl_sched_tcb_remove(kl_sched_tcb_now);
  list_remove(&m_list_manage, &kl_sched_tcb_now->node_manage);
  list_append(&m_list_dead, &kl_sched_tcb_now->node_manage);
  kl_sched_switch();
  kl_port_leave_critical();
}

void kl_thread_clean_up(void) {
  struct kl_thread_node *node;
  while (m_list_dead.head) {
    kl_port_enter_critical();
    node = m_list_dead.head;
    list_remove(&m_list_dead, node);
    kl_port_leave_critical();
    kl_heap_free(node->tcb);
  }
}

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

struct tcb *sched_tcb_now;
struct tcb *sched_tcb_next;
static struct tcb_list m_list_ready[__THREAD_PRIORITY_MAX__];
static struct tcb_list m_list_sleep;
static uint32_t m_idle_timeout;
static uint32_t m_prio_highest;
static uint32_t m_prio_bitmap;

static void list_insert_by_priority(struct tcb_list *list,
                                    struct tcb_node *node) {
  uint32_t prio;
  struct tcb_node *find;
  prio = node->tcb->prio;
  for (find = list->tail; find != NULL; find = find->prev) {
    if (find->tcb->prio >= prio) {
      break;
    }
  }
  list_insert_after(list, find, node);
}

static uint32_t find_highest_priority(uint32_t highest) {
  for (; highest > 0; highest--) {
    if (m_prio_bitmap & (1 << highest)) {
      break;
    }
  }
  return highest;
}

void sched_tcb_remove(struct tcb *tcb) {
  if (tcb->list_wait) {
    list_remove(tcb->list_wait, &tcb->node_wait);
    tcb->list_wait = NULL;
  }
  if (tcb->list_sched) {
    list_remove(tcb->list_sched, &tcb->node_sched);
    if (tcb->list_sched != &m_list_sleep) /* in ready list ? */
    {
      if (tcb->list_sched->head == NULL) {
        m_prio_bitmap &= ~(1 << tcb->prio);
        m_prio_highest = find_highest_priority(m_prio_highest);
      }
    }
    tcb->list_sched = NULL;
  }
}

void sched_tcb_reset(struct tcb *tcb, uint32_t prio) {
  if (tcb->list_wait) {
    list_remove(tcb->list_wait, &tcb->node_wait);
    list_insert_by_priority(tcb->list_wait, &tcb->node_wait);
  }

  if (tcb->list_sched) {
    if (tcb->list_sched != &m_list_sleep) /* in ready list ? */
    {
      /* remove from old list */
      list_remove(tcb->list_sched, &tcb->node_sched);
      if (tcb->list_sched->head == NULL) {
        m_prio_bitmap &= ~(1 << tcb->prio);
        m_prio_highest = find_highest_priority(m_prio_highest);
      }
      /* append to new list */
      tcb->list_sched = &m_list_ready[prio];
      list_append(tcb->list_sched, &tcb->node_sched);
      m_prio_bitmap |= (1 << prio);
      if (m_prio_highest < prio) {
        m_prio_highest = prio;
      }
    }
  }
}

void sched_tcb_ready(struct tcb *tcb) {
  tcb->list_sched = &m_list_ready[tcb->prio];
  list_append(tcb->list_sched, &tcb->node_sched);
  m_prio_bitmap |= (1 << tcb->prio);
  if (m_prio_highest < tcb->prio) {
    m_prio_highest = tcb->prio;
  }
}

void sched_tcb_sleep(struct tcb *tcb, uint32_t timeout) {
  tcb->timeout = timeout;
  tcb->list_sched = &m_list_sleep;
  list_append(tcb->list_sched, &tcb->node_sched);
  if (timeout < m_idle_timeout) {
    m_idle_timeout = timeout;
  }
}

void sched_tcb_wait(struct tcb *tcb, struct tcb_list *list) {
  tcb->list_wait = list;
  list_insert_by_priority(list, &tcb->node_wait);
}

void sched_tcb_timed_wait(struct tcb *tcb, struct tcb_list *list,
                          uint32_t timeout) {
  sched_tcb_wait(tcb, list);
  sched_tcb_sleep(tcb, timeout);
}

static void sched_tcb_wake_up(struct tcb *tcb) {
  if (tcb->list_wait) {
    list_remove(tcb->list_wait, &tcb->node_wait);
    tcb->list_wait = NULL;
  }
  if (tcb->list_sched) {
    list_remove(tcb->list_sched, &tcb->node_sched);
  }
  sched_tcb_ready(tcb);
}

struct tcb *sched_tcb_wake_from(struct tcb_list *list) {
  struct tcb *tcb;
  if (list->head) {
    tcb = list->head->tcb;
    sched_tcb_wake_up(tcb);
    return tcb;
  }
  return NULL;
}

void sched_switch(void) {
  struct tcb *tcb;
  tcb = m_list_ready[m_prio_highest].head->tcb;
  list_remove(tcb->list_sched, &tcb->node_sched);
  if (tcb->list_sched->head == NULL) {
    m_prio_bitmap &= ~(1 << tcb->prio);
    m_prio_highest = find_highest_priority(m_prio_highest);
  }
  tcb->list_sched = NULL;
  sched_tcb_next = tcb;
#if KERNEL_HOOK_ENABLE
  kernel_hook_thread_switch(sched_tcb_now, sched_tcb_next);
#endif
  cpu_contex_switch();
}

void sched_preempt(bool round_robin) {
  if (m_prio_bitmap == 0) /* ready list empty */
  {
    return;
  }
  if (sched_tcb_now != sched_tcb_next) /* last switch was not completed */
  {
    return;
  }
  if ((m_prio_highest + round_robin) > sched_tcb_now->prio) {
    sched_tcb_ready(sched_tcb_now);
    sched_switch();
  }
}

static void sched_timeout(uint32_t elapse) {
  struct tcb *tcb;
  struct tcb_node *node;
  struct tcb_node *next;
  m_idle_timeout = UINT32_MAX;
  for (node = m_list_sleep.head; node != NULL; node = next) {
    next = node->next;
    tcb = node->tcb;
    if (tcb->timeout > elapse) {
      tcb->timeout -= elapse;
      if (tcb->timeout < m_idle_timeout) {
        m_idle_timeout = tcb->timeout;
      }
    } else {
      tcb->timeout = 0;
      sched_tcb_wake_up(tcb);
    }
  }
}

void sched_timing(uint32_t time) {
  static uint32_t elapse;
  elapse += time;
  sched_tcb_now->time += time;
  if (elapse >= m_idle_timeout) {
    sched_timeout(elapse);
    elapse = 0;
  }
}

void sched_idle(void) {
  if (m_prio_bitmap != 0) {
    sched_tcb_ready(sched_tcb_now);
    sched_switch();
  } else {
    cpu_sys_sleep(m_idle_timeout);
  }
}

void sched_init(void) {
  m_idle_timeout = UINT32_MAX;
  m_prio_highest = 0;
  m_prio_bitmap = 0;
  sched_tcb_now = NULL;
  sched_tcb_next =
      sched_tcb_now - 1; /* mark the last switch was not completed */
  memset(m_list_ready, 0, sizeof(m_list_ready));
  memset(&m_list_sleep, 0, sizeof(m_list_sleep));
}

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

struct kl_tcb *sched_tcb_now;
struct kl_tcb *sched_tcb_next;
static struct kl_tcb_list m_list_ready[KLITE_CFG_MAX_PRIO + 1];
static struct kl_tcb_list m_list_sleep;
static kl_tick_t m_idle_elapse;
static kl_tick_t m_idle_timeout;
static uint32_t m_prio_highest;
static uint32_t m_prio_bitmap;

volatile uint32_t sched_susp_nesting = 0;

static inline void list_insert_by_priority(struct kl_tcb_list *list,
                                           struct kl_tcb_node *node) {
  uint32_t prio;
  struct kl_tcb_node *find;
  prio = node->tcb->prio;
  for (find = list->tail; find != NULL; find = find->prev) {
    if (find->tcb->prio >= prio) {
      break;
    }
  }
  list_insert_after(list, find, node);
}

static inline uint32_t find_highest_priority(uint32_t highest) {
  for (; highest > 0; highest--) {
    if (m_prio_bitmap & (1 << highest)) {
      break;
    }
  }
  return highest;
}

static inline void remove_list_wait(struct kl_tcb *tcb) {
  list_remove(tcb->list_wait, &tcb->node_wait);
  tcb->list_wait = NULL;
}

static inline void remove_list_sched(struct kl_tcb *tcb) {
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

void sched_tcb_remove(struct kl_tcb *tcb) {
  if (tcb->list_wait) {
    remove_list_wait(tcb);
  }
  if (tcb->list_sched) {
    remove_list_sched(tcb);
  }
}

void sched_tcb_reset_prio(struct kl_tcb *tcb, uint32_t prio) {
  uint32_t old_prio;
  old_prio = tcb->prio;
  tcb->prio = prio;
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
        m_prio_bitmap &= ~(1 << old_prio);
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

void sched_tcb_ready(struct kl_tcb *tcb) {
  if (tcb->list_sched) {
    remove_list_sched(tcb);
  }
  tcb->list_sched = &m_list_ready[tcb->prio];
  list_append(tcb->list_sched, &tcb->node_sched);
  m_prio_bitmap |= (1 << tcb->prio);
  if (m_prio_highest < tcb->prio) {
    m_prio_highest = tcb->prio;
  }
}

void sched_tcb_sleep(struct kl_tcb *tcb, kl_tick_t timeout) {
  if (tcb->list_sched) {
    remove_list_sched(tcb);
  }
  tcb->timeout = timeout + m_idle_elapse;
  tcb->list_sched = &m_list_sleep;
  list_append(tcb->list_sched, &tcb->node_sched);
  if (tcb->timeout < m_idle_timeout) {
    m_idle_timeout = tcb->timeout;
  }
}

void sched_tcb_wait(struct kl_tcb *tcb, struct kl_tcb_list *list) {
  if (tcb->list_wait) {
    remove_list_wait(tcb);
  }
  tcb->list_wait = list;
#if KLITE_CFG_WAIT_LIST_ORDER_BY_PRIO
  list_insert_by_priority(list, &tcb->node_wait);
#else  // FIFO
  list_append(list, &tcb->node_wait);
#endif
}

void sched_tcb_timed_wait(struct kl_tcb *tcb, struct kl_tcb_list *list,
                          kl_tick_t timeout) {
  sched_tcb_wait(tcb, list);
  if (timeout != KLITE_WAIT_FOREVER) sched_tcb_sleep(tcb, timeout);
}

static inline void sched_tcb_wake_up(struct kl_tcb *tcb) {
  if (tcb->list_wait) {
    remove_list_wait(tcb);
  }
  if (tcb->list_sched) {
    remove_list_sched(tcb);
  }
  sched_tcb_ready(tcb);
}

struct kl_tcb *sched_tcb_wake_from(struct kl_tcb_list *list) {
  struct kl_tcb *tcb;
  if (list->head) {
    tcb = list->head->tcb;
    sched_tcb_wake_up(tcb);
    return tcb;
  }
  return NULL;
}

void sched_switch(void) {
  if (sched_susp_nesting) { /* in critical section */
    return;
  }
  struct kl_tcb *tcb;
  tcb = m_list_ready[m_prio_highest].head->tcb;
#if KLITE_CFG_STACK_OVERFLOW_DETECT
  uint8_t *stack = (uint8_t *)(tcb + 1);
  // check 16 bytes of stack overflow magic value
  if (*(stack + 1) != STACK_MAGIC_VALUE || *(stack + 3) != STACK_MAGIC_VALUE ||
      *(stack + 5) != STACK_MAGIC_VALUE || *(stack + 7) != STACK_MAGIC_VALUE ||
      *(stack + 9) != STACK_MAGIC_VALUE || *(stack + 11) != STACK_MAGIC_VALUE ||
      *(stack + 13) != STACK_MAGIC_VALUE ||
      *(stack + 15) != STACK_MAGIC_VALUE) {
#if KLITE_CFG_STACKOF_BEHAVIOR_SUSPEND
    sched_tcb_remove(tcb);
    tcb = m_list_ready[m_prio_highest].head->tcb;
#elif KLITE_CFG_STACKOF_BEHAVIOR_SYSRESET
    NVIC_SystemReset();
#elif KLITE_CFG_STACKOF_BEHAVIOR_HARDFLT
    ((void (*)(void))0x10)();
#elif KLITE_CFG_STACKOF_BEHAVIOR_CALLBACK
    kl_stack_overflow_callback();
#endif
  }
#endif
  list_remove(tcb->list_sched, &tcb->node_sched);
  if (tcb->list_sched->head == NULL) {
    m_prio_bitmap &= ~(1 << tcb->prio);
    m_prio_highest = find_highest_priority(m_prio_highest);
  }
  tcb->list_sched = NULL;
  sched_tcb_next = tcb;
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
  if (sched_susp_nesting) { /* in critical section */
    return;
  }
  if ((m_prio_highest + round_robin) > sched_tcb_now->prio) {
    sched_tcb_ready(sched_tcb_now);
    sched_switch();
  }
}

static inline void sched_timeout(void) {
  struct kl_tcb *tcb;
  struct kl_tcb_node *node;
  struct kl_tcb_node *next;
  m_idle_timeout = UINT32_MAX;
  for (node = m_list_sleep.head; node != NULL; node = next) {
    next = node->next;
    tcb = node->tcb;
    if (tcb->timeout > m_idle_elapse) {
      tcb->timeout -= m_idle_elapse;
      if (tcb->timeout < m_idle_timeout) {
        m_idle_timeout = tcb->timeout;
      }
    } else {
      tcb->timeout = 0;
      sched_tcb_wake_up(tcb);
    }
  }
}

void sched_timing(kl_tick_t time) {
  m_idle_elapse += time;
  sched_tcb_now->time += time;
  if (m_idle_elapse >= m_idle_timeout) {
    sched_timeout();
    m_idle_elapse = 0;
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
  m_idle_elapse = 0;
  m_idle_timeout = UINT32_MAX;
  m_prio_highest = 0;
  m_prio_bitmap = 0;
  sched_tcb_now = NULL;
  sched_tcb_next =
      sched_tcb_now - 1; /* mark the last switch was not completed */
  memset(m_list_ready, 0, sizeof(m_list_ready));
  memset(&m_list_sleep, 0, sizeof(m_list_sleep));
}

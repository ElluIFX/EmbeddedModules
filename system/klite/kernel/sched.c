#include "kl_blist.h"
#include "kl_priv.h"

kl_thread_t kl_sched_tcb_now;
kl_thread_t kl_sched_tcb_next;
static struct kl_thread_list m_list_ready[KLITE_CFG_MAX_PRIO + 1];
static struct kl_thread_list m_list_sleep;
static kl_tick_t m_idle_elapse;
static kl_tick_t m_idle_timeout;
static uint32_t m_prio_highest;
static uint32_t m_prio_bitmap;
static uint32_t m_susp_nesting;
static uint8_t m_susp_pending_flags;

#define SUSPEND_SWITCH_PENDING 0x01
#define SUSPEND_PREEMPT_PENDING 0x02
#define SUSPEND_PREEMPT_ROUND_ROBIN 0x04

static inline void list_insert_by_priority(struct kl_thread_list *list,
                                           struct kl_thread_node *node) {
  uint32_t prio;
  struct kl_thread_node *find;
  prio = node->tcb->prio;
  for (find = list->tail; find != NULL; find = find->prev) {
    if (find->tcb->prio >= prio) {
      break;
    }
  }
  kl_blist_insert_after(list, find, node);
}

static inline uint32_t find_highest_priority(uint32_t highest) {
  for (; highest > 0; highest--) {
    if (m_prio_bitmap & (1 << highest)) {
      break;
    }
  }
  return highest;
}

static inline void remove_list_wait(kl_thread_t tcb) {
  kl_blist_remove(tcb->list_wait, &tcb->node_wait);
  tcb->list_wait = NULL;
  KL_CLR_FLAG32(tcb->info, KL_THREAD_FLAGS_WAIT);
}

static inline void remove_list_sched(kl_thread_t tcb) {
  kl_blist_remove(tcb->list_sched, &tcb->node_sched);
  if (tcb->list_sched != &m_list_sleep) /* in ready list ? */
  {
    if (tcb->list_sched->head == NULL) {
      m_prio_bitmap &= ~(1 << tcb->prio);
      m_prio_highest = find_highest_priority(m_prio_highest);
    }
  }
  tcb->list_sched = NULL;
  KL_CLR_FLAG32(tcb->info, KL_THREAD_FLAGS_READY | KL_THREAD_FLAGS_SLEEP);
}

void kl_sched_tcb_remove(kl_thread_t tcb) {
  if (tcb->list_wait) {
    remove_list_wait(tcb);
  }
  if (tcb->list_sched) {
    remove_list_sched(tcb);
  }
  KL_CLR_FLAG32(tcb->info,
                (KL_THREAD_FLAGS_READY | KL_THREAD_FLAGS_SUSPEND |
                 KL_THREAD_FLAGS_SLEEP | KL_THREAD_FLAGS_WAIT));
  KL_SET_FLAG(tcb->info, KL_THREAD_FLAGS_EXITED);
}

void kl_sched_tcb_ready(kl_thread_t tcb) {
  if (tcb->list_sched) {
    remove_list_sched(tcb);
  }
  tcb->list_sched = &m_list_ready[tcb->prio];
  kl_blist_append(tcb->list_sched, &tcb->node_sched);
  m_prio_bitmap |= (1 << tcb->prio);
  if (m_prio_highest < tcb->prio) {
    m_prio_highest = tcb->prio;
  }
  KL_SET_FLAG(tcb->info, KL_THREAD_FLAGS_READY);
}

void kl_sched_tcb_suspend(kl_thread_t tcb) {
  struct kl_thread_list *list; /* keep list pointer for resume */
  if (tcb->list_wait) {        /* remove wait */
    list = tcb->list_wait;
    remove_list_wait(tcb);
    tcb->list_wait = list;
  }
  if (tcb->list_sched) { /* remove sched */
    list = tcb->list_sched;
    remove_list_sched(tcb);
    tcb->list_sched = list;
    if (tcb->list_sched == &m_list_sleep && tcb->timeout > m_idle_elapse) {
      tcb->timeout -= m_idle_elapse; /* remain sleep time */
    }
  }
  KL_SET_FLAG(tcb->info, KL_THREAD_FLAGS_SUSPEND);
}

void kl_sched_tcb_resume(kl_thread_t tcb) {
  if (KL_GET_FLAG(tcb->info, KL_THREAD_FLAGS_SUSPEND)) {
    KL_CLR_FLAG32(tcb->info, KL_THREAD_FLAGS_SUSPEND);
    if (tcb->list_wait) { /* set wait */
#if KLITE_CFG_WAIT_LIST_ORDER_BY_PRIO
      list_insert_by_priority(tcb->list_wait, &tcb->node_wait);
#else  // FIFO
      kl_blist_append(tcb->list_wait, &tcb->node_wait);
#endif
      KL_SET_FLAG(tcb->info, KL_THREAD_FLAGS_WAIT);
    }
    if (tcb->list_sched == &m_list_sleep) { /* set sleep */
      tcb->timeout += m_idle_elapse;
      kl_blist_append(tcb->list_sched, &tcb->node_sched);
      if (tcb->timeout < m_idle_timeout) {
        m_idle_timeout = tcb->timeout;
      }
      KL_SET_FLAG(tcb->info, KL_THREAD_FLAGS_SLEEP);
    } else if (tcb->list_sched) { /* set ready */
      tcb->list_sched = NULL;
      kl_sched_tcb_ready(tcb);
    }
  }
}

void kl_sched_tcb_reset_prio(kl_thread_t tcb, uint32_t prio) {
  uint32_t old_prio;
  old_prio = tcb->prio;
  tcb->prio = prio;
  if (KL_GET_FLAG(tcb->info, KL_THREAD_FLAGS_SUSPEND)) {
    return; /* suspend state, lists stored are not real */
  }
  if (tcb->list_wait) {
    kl_blist_remove(tcb->list_wait, &tcb->node_wait);
#if KLITE_CFG_WAIT_LIST_ORDER_BY_PRIO
    list_insert_by_priority(tcb->list_wait, &tcb->node_wait);
#else  // FIFO
    kl_blist_append(tcb->list_wait, &tcb->node_wait);
#endif
  }
  if (tcb->list_sched) {
    if (tcb->list_sched != &m_list_sleep) /* in ready list ? */
    {
      /* remove from old list */
      kl_blist_remove(tcb->list_sched, &tcb->node_sched);
      if (tcb->list_sched->head == NULL) {
        m_prio_bitmap &= ~(1 << old_prio);
        m_prio_highest = find_highest_priority(m_prio_highest);
      }
      /* append to new list */
      tcb->list_sched = &m_list_ready[prio];
      kl_blist_append(tcb->list_sched, &tcb->node_sched);
      m_prio_bitmap |= (1 << prio);
      if (m_prio_highest < prio) {
        m_prio_highest = prio;
      }
    }
  }
}

void kl_sched_tcb_sleep(kl_thread_t tcb, kl_tick_t timeout) {
  if (m_susp_nesting) {
    /* sleep is not allowed in critical section */
    return;
  }
  if (tcb->list_sched) {
    remove_list_sched(tcb);
  }
  tcb->timeout = timeout + m_idle_elapse;
  tcb->list_sched = &m_list_sleep;
  kl_blist_append(tcb->list_sched, &tcb->node_sched);
  if (tcb->timeout < m_idle_timeout) {
    m_idle_timeout = tcb->timeout;
  }
  KL_SET_FLAG(tcb->info, KL_THREAD_FLAGS_SLEEP);
}

void kl_sched_tcb_wait(kl_thread_t tcb, struct kl_thread_list *list) {
  if (tcb->list_wait) {
    remove_list_wait(tcb);
  }
  tcb->list_wait = list;
#if KLITE_CFG_WAIT_LIST_ORDER_BY_PRIO
  list_insert_by_priority(list, &tcb->node_wait);
#else  // FIFO
  kl_blist_append(list, &tcb->node_wait);
#endif
  KL_SET_FLAG(tcb->info, KL_THREAD_FLAGS_WAIT);
}

void kl_sched_tcb_timed_wait(kl_thread_t tcb, struct kl_thread_list *list,
                             kl_tick_t timeout) {
  kl_sched_tcb_wait(tcb, list);
  if (timeout != KL_WAIT_FOREVER) {
    kl_sched_tcb_sleep(tcb, timeout);
  } else {
    tcb->timeout = KL_WAIT_FOREVER;
  }
}

static inline void kl_sched_tcb_wake_up(kl_thread_t tcb) {
  if (tcb->list_wait) {
    remove_list_wait(tcb);
  }
  if (tcb->list_sched) {
    remove_list_sched(tcb);
  }
  kl_sched_tcb_ready(tcb);
}

kl_thread_t kl_sched_tcb_wake_from(struct kl_thread_list *list) {
  kl_thread_t tcb;
  if (list->head) {
    tcb = list->head->tcb;
    kl_sched_tcb_wake_up(tcb);
    return tcb;
  }
  return NULL;
}

void kl_sched_suspend(void) {
  if (!m_susp_nesting) { /* clear pending switch */
    m_susp_pending_flags = 0;
  }
  m_susp_nesting++;
}

void kl_sched_resume(void) {
  if (m_susp_nesting == 0) {
    return;
  }
  m_susp_nesting--;
  if (m_susp_nesting == 0) { /* resume switch */
    if (KL_GET_FLAG(m_susp_pending_flags, SUSPEND_PREEMPT_PENDING)) {
      kl_sched_preempt(
          KL_GET_FLAG(m_susp_pending_flags, SUSPEND_PREEMPT_ROUND_ROBIN));
    } else if (KL_GET_FLAG(m_susp_pending_flags, SUSPEND_SWITCH_PENDING)) {
      kl_sched_switch();
    }
    m_susp_pending_flags = 0;
  }
}

void kl_sched_switch(void) {
  if (m_susp_nesting) { /* in suspend state */
    KL_SET_FLAG(m_susp_pending_flags, SUSPEND_SWITCH_PENDING);
    return;
  }
  kl_thread_t tcb = m_list_ready[m_prio_highest].head->tcb;
#if KLITE_CFG_STACK_OVERFLOW_DETECT
  uint32_t *stack_magic = (uint32_t *)(tcb + 1);
  // check 32 bytes of stack overflow magic value
  if (*(stack_magic) != KL_STACK_MAGIC_VALUE ||
      *(stack_magic + 1) != KL_STACK_MAGIC_VALUE ||
      *(stack_magic + 2) != KL_STACK_MAGIC_VALUE ||
      *(stack_magic + 3) != KL_STACK_MAGIC_VALUE ||
      *(stack_magic + 4) != KL_STACK_MAGIC_VALUE ||
      *(stack_magic + 5) != KL_STACK_MAGIC_VALUE ||
      *(stack_magic + 6) != KL_STACK_MAGIC_VALUE ||
      *(stack_magic + 7) != KL_STACK_MAGIC_VALUE) {
#if KLITE_CFG_STACKOF_BEHAVIOR_SUSPEND
    kl_sched_tcb_suspend(tcb);
    tcb = m_list_ready[m_prio_highest].head->tcb;
#elif KLITE_CFG_STACKOF_BEHAVIOR_SYSRESET
    NVIC_SystemReset();
#elif KLITE_CFG_STACKOF_BEHAVIOR_HARDFLT
    ((void (*)(void))0x10)();
#elif KLITE_CFG_STACKOF_BEHAVIOR_CALLBACK
    kl_stack_overflow_hook();
#endif
  }
#endif
  kl_blist_remove(tcb->list_sched, &tcb->node_sched);
  KL_CLR_FLAG32(tcb->info, KL_THREAD_FLAGS_READY);
  if (tcb->list_sched->head == NULL) {
    m_prio_bitmap &= ~(1 << tcb->prio);
    m_prio_highest = find_highest_priority(m_prio_highest);
  }
  tcb->list_sched = NULL;
  kl_sched_tcb_next = tcb;
  kl_port_context_switch();
}

void kl_sched_preempt(const bool round_robin) {
  if (m_prio_bitmap == 0) /* ready list empty */
  {
    return;
  }
  if (kl_sched_tcb_now != kl_sched_tcb_next) /* last switch was not completed */
  {
    return;
  }
  if (m_susp_nesting) { /* in critical section */
    KL_SET_FLAG(m_susp_pending_flags, SUSPEND_PREEMPT_PENDING);
    if (round_robin) {
      KL_SET_FLAG(m_susp_pending_flags, SUSPEND_PREEMPT_ROUND_ROBIN);
    }
    return;
  }
  if ((m_prio_highest + round_robin) > kl_sched_tcb_now->prio) {
    kl_sched_tcb_ready(kl_sched_tcb_now);
    kl_sched_switch();
  }
}

static inline void kl_sched_timeout(void) {
  kl_thread_t tcb;
  struct kl_thread_node *node;
  struct kl_thread_node *next;
  m_idle_timeout = KL_WAIT_FOREVER;
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
      kl_sched_tcb_wake_up(tcb);
    }
  }
}

void kl_sched_timing(kl_tick_t time) {
  m_idle_elapse += time;
  kl_sched_tcb_now->time += time;
  if (m_idle_elapse >= m_idle_timeout) {
    kl_sched_timeout();
    m_idle_elapse = 0;
  }
}

void kl_sched_idle(void) {
  if (m_prio_bitmap != 0) {
    kl_sched_tcb_ready(kl_sched_tcb_now);
    kl_sched_switch();
  } else {
    kl_port_sys_idle(m_idle_timeout);
  }
}

void kl_sched_init(void) {
  m_idle_elapse = 0;
  m_idle_timeout = KL_WAIT_FOREVER;
  m_prio_highest = 0;
  m_prio_bitmap = 0;
  m_susp_nesting = 0;
  m_susp_pending_flags = 0;
  kl_sched_tcb_now = NULL;
  kl_sched_tcb_next =
      kl_sched_tcb_now - 1; /* mark the last switch was not completed */
  memset(m_list_ready, 0, sizeof(m_list_ready));
  memset(&m_list_sleep, 0, sizeof(m_list_sleep));
}

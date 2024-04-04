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

#if KLITE_CFG_OPT_SOFT_TIMER

#include <string.h>

#include "klite_list.h"

struct timer_list {
  struct kl_soft_timer *head;
  struct kl_soft_timer *tail;
};

static struct timer_list m_timer_list;
static kl_mutex_t m_timer_mutex;
static kl_event_t m_timer_event;
static kl_thread_t m_timer_thread;

static kl_tick_t kl_soft_timer_process(kl_tick_t time) {
  struct kl_soft_timer *node;
  kl_tick_t timeout = KL_WAIT_FOREVER;
  kl_mutex_lock(m_timer_mutex);
  for (node = m_timer_list.head; node != NULL; node = node->next) {
    if (node->reload == 0) {
      continue;
    }
    if (node->timeout > time) {
      node->timeout -= time;
    } else {
      node->handler(node->arg);
      node->timeout = node->reload;
    }
    if (node->timeout < timeout) {
      timeout = node->timeout;
    }
  }
  kl_mutex_unlock(m_timer_mutex);
  return timeout;
}

static void kl_soft_timer_service(void *arg) {
  kl_tick_t last;
  kl_tick_t time;
  kl_tick_t timeout;
  last = kl_kernel_tick();
  while (1) {
    time = kl_kernel_tick() - last;
    last += time;
    timeout = kl_soft_timer_process(time);
    time = kl_kernel_tick() - last;
    if (timeout > time) {
      kl_event_timed_wait(m_timer_event, timeout - time);
    }
  }
}

bool kl_soft_timer_init(uint32_t stack_size, uint32_t priority) {
  if (m_timer_thread != NULL) {
    return true;
  }
  memset(&m_timer_list, 0, sizeof(struct timer_list));
  m_timer_mutex = kl_mutex_create();
  if (m_timer_mutex == NULL) {
    return false;
  }
  m_timer_event = kl_event_create(true);
  if (m_timer_event == NULL) {
    kl_mutex_delete(m_timer_mutex);
    return false;
  }
  m_timer_thread =
      kl_thread_create(kl_soft_timer_service, NULL, stack_size, priority);
  if (m_timer_thread == NULL) {
    kl_mutex_delete(m_timer_mutex);
    kl_event_delete(m_timer_event);
    return false;
  }
  return true;
}

void kl_soft_timer_deinit(void) {
  struct kl_soft_timer *node;
  if (m_timer_thread == NULL) {
    return;
  }
  kl_thread_delete(m_timer_thread);
  m_timer_thread = NULL;
  kl_event_delete(m_timer_event);
  m_timer_event = NULL;
  kl_mutex_lock(m_timer_mutex);
  while (m_timer_list.head != NULL) {
    node = m_timer_list.head;
    list_remove(&m_timer_list, node);
    kl_heap_free(node);
  }
  kl_mutex_unlock(m_timer_mutex);
  kl_mutex_delete(m_timer_mutex);
}

kl_soft_timer_t kl_soft_timer_create(void (*handler)(void *), void *arg) {
  struct kl_soft_timer *timer;
  if (!m_timer_thread) {
    if (!kl_soft_timer_init(0, 0)) {  // use default value
      return NULL;
    }
  }
  timer = kl_heap_alloc(sizeof(struct kl_soft_timer));
  if (timer != NULL) {
    memset(timer, 0, sizeof(struct kl_soft_timer));
    timer->handler = handler;
    timer->arg = arg;
    kl_mutex_lock(m_timer_mutex);
    list_append(&m_timer_list, timer);
    kl_mutex_unlock(m_timer_mutex);
  }
  return timer;
}

void kl_soft_timer_delete(kl_soft_timer_t timer) {
  kl_mutex_lock(m_timer_mutex);
  list_remove(&m_timer_list, timer);
  kl_mutex_unlock(m_timer_mutex);
  kl_heap_free(timer);
}

void kl_soft_timer_start(kl_soft_timer_t timer, kl_tick_t timeout) {
  kl_mutex_lock(m_timer_mutex);
  timer->reload = (timeout > 0) ? timeout : 1; /* timeout can't be 0 */
  timer->timeout = timer->reload;
  kl_mutex_unlock(m_timer_mutex);
  kl_event_set(m_timer_event);
}

void kl_soft_timer_stop(kl_soft_timer_t timer) {
  kl_mutex_lock(m_timer_mutex);
  timer->reload = 0;
  kl_mutex_unlock(m_timer_mutex);
  kl_event_set(m_timer_event);
}

#endif /* KLITE_CFG_OPT_SOFT_TIMER */

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
#include "soft_timer.h"

#include <string.h>

#include "kernel.h"
#include "list.h"

struct soft_timer {
  struct soft_timer *prev;
  struct soft_timer *next;
  void (*handler)(void *);
  void *arg;
  uint32_t reload;
  uint32_t timeout;
};

struct timer_list {
  struct soft_timer *head;
  struct soft_timer *tail;
};

static struct timer_list m_timer_list;
static mutex_t m_timer_mutex;
static event_t m_timer_event;
static thread_t m_timer_thread;

static uint32_t soft_timer_process(uint32_t time) {
  struct soft_timer *node;
  uint32_t timeout = UINT32_MAX;
  mutex_lock(m_timer_mutex);
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
  mutex_unlock(m_timer_mutex);
  return timeout;
}

static void soft_timer_service(void *arg) {
  uint32_t last;
  uint32_t time;
  uint32_t timeout;
  last = kernel_tick_count();
  while (1) {
    time = kernel_tick_count() - last;
    last = kernel_tick_count();
    timeout = soft_timer_process(time);
    time = kernel_tick_count() - last;
    if (timeout > time) {
      event_timed_wait(m_timer_event, timeout - time);
    }
  }
}

static bool soft_timer_init(void) {
  if (m_timer_thread != NULL) {
    return true;
  }
  memset(&m_timer_list, 0, sizeof(struct timer_list));
  m_timer_mutex = mutex_create();
  if (m_timer_mutex == NULL) {
    return false;
  }
  m_timer_event = event_create(true);
  if (m_timer_event == NULL) {
    mutex_delete(m_timer_mutex);
    return false;
  }
  m_timer_thread =
      thread_create(soft_timer_service, NULL, 0, SOFT_TIMER_PRIORITY);
  if (m_timer_thread == NULL) {
    mutex_delete(m_timer_mutex);
    event_delete(m_timer_event);
    return false;
  }
  return true;
}

soft_timer_t soft_timer_create(void (*handler)(void *), void *arg) {
  struct soft_timer *timer;
  if (!soft_timer_init()) {
    return NULL;
  }
  timer = heap_alloc(sizeof(struct soft_timer));
  if (timer != NULL) {
    memset(timer, 0, sizeof(struct soft_timer));
    timer->handler = handler;
    timer->arg = arg;
    mutex_lock(m_timer_mutex);
    list_append(&m_timer_list, timer);
    mutex_unlock(m_timer_mutex);
  }
  return timer;
}

void soft_timer_delete(soft_timer_t timer) {
  mutex_lock(m_timer_mutex);
  list_remove(&m_timer_list, timer);
  mutex_unlock(m_timer_mutex);
  heap_free(timer);
}

void soft_timer_start(soft_timer_t timer, uint32_t timeout) {
  mutex_lock(m_timer_mutex);
  timer->reload = (timeout > 0) ? timeout : 1; /* timeout can't be 0 */
  timer->timeout = timer->reload;
  mutex_unlock(m_timer_mutex);
  event_set(m_timer_event);
}

void soft_timer_stop(soft_timer_t timer) {
  mutex_lock(m_timer_mutex);
  timer->reload = 0;
  mutex_unlock(m_timer_mutex);
  event_set(m_timer_event);
}

#include "klite.h"

#if KLITE_CFG_OPT_SOFT_TIMER

#include <string.h>

#include "kl_list.h"

static kl_tick_t kl_timer_process(kl_timer_t timer, kl_tick_t time) {
  kl_timer_task_t node;
  kl_tick_t timeout = KL_WAIT_FOREVER;
  kl_mutex_lock(timer->mutex);
  for (node = timer->head; node != NULL; node = node->next) {
    if (node->reload == 0) {
      continue;
    }
    if (node->timeout > time) {
      node->timeout -= time;
    } else {
      node->handler(node->arg);
      node->timeout = node->reload - (time - node->timeout);
    }
    if (node->timeout < timeout) {
      timeout = node->timeout;
    }
  }
  kl_mutex_unlock(timer->mutex);
  return timeout;
}

static void kl_timer_service(void *arg) {
  kl_tick_t last;
  kl_tick_t time;
  kl_tick_t timeout;
  kl_timer_t timer = (kl_timer_t)arg;
  last = kl_kernel_tick();
  while (1) {
    time = kl_kernel_tick() - last;
    last += time;
    timeout = kl_timer_process(timer, time);
    time = kl_kernel_tick() - last;
    if (timeout > time) {
      kl_event_timed_wait(timer->event, timeout - time);
    }
  }
}

kl_timer_t kl_timer_create(uint32_t stack_size, uint32_t priority) {
  kl_timer_t timer = kl_heap_alloc(sizeof(struct kl_timer));
  if (!timer) {
    return NULL;
  }
  memset(timer, 0, sizeof(struct kl_timer));
  timer->mutex = kl_mutex_create();
  if (timer->mutex == NULL) {
    kl_heap_free(timer);
    return NULL;
  }
  timer->event = kl_event_create(true);
  if (timer->event == NULL) {
    kl_heap_free(timer);
    kl_mutex_delete(timer->mutex);
    return NULL;
  }
  timer->thread =
      kl_thread_create(kl_timer_service, timer, stack_size, priority);
  if (timer->thread == NULL) {
    kl_heap_free(timer);
    kl_mutex_delete(timer->mutex);
    kl_event_delete(timer->event);
    return NULL;
  }
  return timer;
}

void kl_timer_delete(kl_timer_t timer) {
  kl_timer_task_t node;

  kl_thread_delete(timer->thread);
  kl_event_delete(timer->event);
  kl_mutex_lock(timer->mutex);
  while (timer->head != NULL) {
    node = timer->head;
    list_remove(timer, node);
    kl_heap_free(node);
  }
  kl_mutex_unlock(timer->mutex);
  kl_mutex_delete(timer->mutex);
  kl_heap_free(timer);
}

kl_timer_task_t kl_timer_add_task(kl_timer_t timer, void (*handler)(void *),
                                  void *arg) {
  kl_timer_task_t node;
  node = kl_heap_alloc(sizeof(struct kl_timer_task));
  if (node != NULL) {
    memset(node, 0, sizeof(struct kl_timer_task));
    node->timer = timer;
    node->handler = handler;
    node->arg = arg;
    node->reload = 0;
    node->timeout = 0;
    kl_mutex_lock(timer->mutex);
    list_append(timer, node);
    kl_mutex_unlock(timer->mutex);
  }
  return node;
}

void kl_timer_remove_task(kl_timer_task_t task) {
  kl_mutex_lock(task->timer->mutex);
  list_remove(task->timer, task);
  kl_mutex_unlock(task->timer->mutex);
  kl_heap_free(task);
}

void kl_timer_start_task(kl_timer_task_t task, kl_tick_t timeout) {
  kl_mutex_lock(task->timer->mutex);
  task->reload = (timeout > 0) ? timeout : 1; /* timeout can't be 0 */
  task->timeout = task->reload;
  kl_mutex_unlock(task->timer->mutex);
  kl_event_set(task->timer->event);
}

void kl_timer_stop_task(kl_timer_task_t task) {
  kl_mutex_lock(task->timer->mutex);
  task->reload = 0;
  kl_mutex_unlock(task->timer->mutex);
  kl_event_set(task->timer->event);
}

#endif /* KLITE_CFG_OPT_SOFT_TIMER */

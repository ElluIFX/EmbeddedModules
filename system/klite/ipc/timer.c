#include "kl_priv.h"

#if KLITE_CFG_OPT_TIMER

#include <string.h>

#include "kl_slist.h"

static kl_tick_t kl_timer_process(kl_timer_t timer, kl_tick_t time) {
  kl_timer_task_t task;
  kl_tick_t timeout = KL_WAIT_FOREVER;
  kl_mutex_lock(&timer->mutex, KL_WAIT_FOREVER);
  for (task = timer->head; task != NULL; task = task->next) {
    if (task->reload == 0) {
      continue;
    }
    if (task->timeout > time) {
      task->timeout -= time;
    } else {
      task->handler(task->arg);
      task->timeout = task->reload - (time - task->timeout);
    }
    if (task->timeout < timeout) {
      timeout = task->timeout;
    }
  }
  kl_mutex_unlock(&timer->mutex);
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
      kl_cond_wait_complete(&timer->cond, timeout - time);
    }
  }
}

kl_timer_t kl_timer_create(kl_size_t stack_size, uint32_t priority) {
  kl_timer_t timer = kl_heap_alloc(sizeof(struct kl_timer));
  if (!timer) {
    KL_SET_ERRNO(KL_ENOMEM);
    return NULL;
  }
  memset(timer, 0, sizeof(struct kl_timer));
  timer->thread =
      kl_thread_create(kl_timer_service, timer, stack_size, priority);
  if (timer->thread == NULL) {
    kl_heap_free(timer);
    KL_SET_ERRNO(KL_ENOMEM);
    return NULL;
  }
  return timer;
}

void kl_timer_delete(kl_timer_t timer) {
  kl_timer_task_t task;

  kl_thread_delete(timer->thread);
  kl_mutex_lock(&timer->mutex, KL_WAIT_FOREVER);
  while (timer->head != NULL) {
    task = timer->head;
    kl_slist_remove(timer, task);
    kl_heap_free(task);
  }
  kl_mutex_unlock(&timer->mutex);
  kl_heap_free(timer);
}

kl_timer_task_t kl_timer_add_task(kl_timer_t timer, void (*handler)(void *),
                                  void *arg) {
  kl_timer_task_t task;
  task = kl_heap_alloc(sizeof(struct kl_timer_task));
  if (task != NULL) {
    memset(task, 0, sizeof(struct kl_timer_task));
    task->timer = timer;
    task->handler = handler;
    task->arg = arg;
    task->reload = 0;
    task->timeout = 0;
    kl_mutex_lock(&timer->mutex, KL_WAIT_FOREVER);
    kl_slist_append(timer, task);
    kl_mutex_unlock(&timer->mutex);
  } else {
    KL_SET_ERRNO(KL_ENOMEM);
  }
  return task;
}

void kl_timer_remove_task(kl_timer_task_t task) {
  kl_mutex_lock(&task->timer->mutex, KL_WAIT_FOREVER);
  kl_slist_remove(task->timer, task);
  kl_mutex_unlock(&task->timer->mutex);
  kl_heap_free(task);
}

void kl_timer_start_task(kl_timer_task_t task, kl_tick_t timeout) {
  kl_mutex_lock(&task->timer->mutex, KL_WAIT_FOREVER);
  task->reload = (timeout > 0) ? timeout : 1; /* timeout can't be 0 */
  task->timeout = task->reload;
  kl_mutex_unlock(&task->timer->mutex);
  kl_cond_signal(&task->timer->cond);
}

void kl_timer_stop_task(kl_timer_task_t task) {
  kl_mutex_lock(&task->timer->mutex, KL_WAIT_FOREVER);
  task->reload = 0;
  kl_mutex_unlock(&task->timer->mutex);
  kl_cond_signal(&task->timer->cond);
}

#endif /* KLITE_CFG_OPT_TIMER */

#include "kl_priv.h"

#if KLITE_CFG_OPT_THREAD_POOL

#include <string.h>

struct kl_thread_pool_task {
  void (*process)(void *arg);
  void *arg;
};

static void thread_job(void *arg) {
  kl_thread_pool_t pool = (kl_thread_pool_t)arg;
  struct kl_thread_pool_task temp_task;
  kl_sem_give(pool->idle_sem);
  while (1) {
    kl_mqueue_recv(pool->task_queue, &temp_task, KL_WAIT_FOREVER);
    kl_sem_take(pool->idle_sem);
    temp_task.process(temp_task.arg);
    kl_sem_give(pool->idle_sem);
  }
}

kl_thread_pool_t kl_thread_pool_create(kl_size_t worker_num,
                                       kl_size_t worker_stack_size,
                                       uint32_t worker_priority,
                                       kl_size_t max_task_num) {
  kl_thread_pool_t pool =
      (kl_thread_pool_t)kl_heap_alloc(sizeof(struct kl_thread_pool));
  if (pool == NULL) {
    return NULL;
  }
  pool->worker_num = worker_num;
  pool->task_queue =
      kl_mqueue_create(sizeof(struct kl_thread_pool_task), max_task_num);
  if (pool->task_queue == NULL) {
    kl_heap_free(pool);
    return NULL;
  }
  pool->thread_list =
      (kl_thread_t *)kl_heap_alloc(worker_num * sizeof(kl_thread_t *));
  if (pool->thread_list == NULL) {
    kl_mqueue_delete(pool->task_queue);
    kl_heap_free(pool);
    return NULL;
  }
  pool->idle_sem = kl_sem_create(0);
  if (pool->idle_sem == NULL) {
    kl_mqueue_delete(pool->task_queue);
    kl_heap_free(pool->thread_list);
    kl_heap_free(pool);
    return NULL;
  }
  for (uint8_t i = 0; i < worker_num; i++) {
    pool->thread_list[i] =
        kl_thread_create(thread_job, pool, worker_stack_size, worker_priority);
  }
  return pool;
}

bool kl_thread_pool_submit(kl_thread_pool_t pool, void (*process)(void *arg),
                           void *arg, kl_tick_t timeout) {
  struct kl_thread_pool_task task = {
      .process = process,
      .arg = arg,
  };
  if (kl_mqueue_send(pool->task_queue, &task, timeout)) {
    return false;
  }
  return true;
}

kl_size_t kl_thread_pool_pending_task(kl_thread_pool_t pool) {
  return kl_mqueue_count(pool->task_queue) +
         (pool->worker_num - kl_sem_value(pool->idle_sem));
}

void kl_thread_pool_join(kl_thread_pool_t pool) {
  while (kl_thread_pool_pending_task(pool) > 0) {
    kl_thread_sleep(kl_ms_to_ticks(50));
  }
}

void kl_thread_pool_shutdown(kl_thread_pool_t pool) {
  /* exit all thread */
  for (uint8_t i = 0; i < pool->worker_num; i++) {
    kl_thread_delete(pool->thread_list[i]);
  }
  /* release memory */
  kl_mqueue_delete(pool->task_queue);
  kl_heap_free(pool->thread_list);
  kl_heap_free(pool);
}

#endif  // KLITE_CFG_OPT_THREAD_POOL

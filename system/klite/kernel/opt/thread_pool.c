#include "klite.h"

#if KLITE_CFG_OPT_THREAD_POOL

#include <string.h>

#include "klite_internal.h"

struct thread_pool_task {
  void (*process)(void *arg);
  void *arg;
};

struct thread_pool {
  msg_queue_t task_queue;
  thread_t *thread_list;
  sem_t idle_sem;
  uint8_t worker_num;
};

static void thread_job(void *arg) {
  thread_pool_t pool = (thread_pool_t)arg;
  struct thread_pool_task temp_task;
  sem_give(pool->idle_sem);
  while (1) {
    msg_queue_recv(pool->task_queue, &temp_task);
    sem_take(pool->idle_sem);
    temp_task.process(temp_task.arg);
    sem_give(pool->idle_sem);
  }
}

thread_pool_t thread_pool_create(uint8_t worker_num, uint32_t worker_stack_size,
                                 uint32_t worker_priority,
                                 uint32_t max_task_num) {
  thread_pool_t pool = (thread_pool_t)heap_alloc(sizeof(struct thread_pool));
  if (pool == NULL) {
    return NULL;
  }
  pool->worker_num = worker_num;
  pool->task_queue =
      msg_queue_create(sizeof(struct thread_pool_task), max_task_num);
  if (pool->task_queue == NULL) {
    heap_free(pool);
    return NULL;
  }
  pool->thread_list = (thread_t *)heap_alloc(worker_num * sizeof(thread_t *));
  if (pool->thread_list == NULL) {
    msg_queue_delete(pool->task_queue);
    heap_free(pool);
    return NULL;
  }
  pool->idle_sem = sem_create(0);
  if (pool->idle_sem == NULL) {
    msg_queue_delete(pool->task_queue);
    heap_free(pool->thread_list);
    heap_free(pool);
    return NULL;
  }
  for (uint8_t i = 0; i < worker_num; i++) {
    pool->thread_list[i] =
        thread_create(thread_job, pool, worker_stack_size, worker_priority);
  }
  return pool;
}

bool thread_pool_submit(thread_pool_t pool, void (*process)(void *arg),
                        void *arg, klite_tick_t timeout) {
  struct thread_pool_task task = {
      .process = process,
      .arg = arg,
  };
  if (msg_queue_timed_send(pool->task_queue, &task, timeout)) {
    return false;
  }
  return true;
}

uint16_t thread_pool_pending_task(thread_pool_t pool) {
  return msg_queue_count(pool->task_queue) +
         (pool->worker_num - sem_value(pool->idle_sem));
}

void thread_pool_join(thread_pool_t pool) {
  while (thread_pool_pending_task(pool) > 0) {
    thread_sleep(kernel_ms_to_ticks(50));
  }
}

void thread_pool_shutdown(thread_pool_t pool) {
  /* exit all thread */
  for (uint8_t i = 0; i < pool->worker_num; i++) {
    thread_delete(pool->thread_list[i]);
  }
  /* release memory */
  msg_queue_delete(pool->task_queue);
  heap_free(pool->thread_list);
  heap_free(pool);
}

#endif  // KLITE_CFG_OPT_THREAD_POOL

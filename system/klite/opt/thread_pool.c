
#include "thread_pool.h"

#include "internal.h"

#define THREAD_POOL_JOIN_TIME 100  // thread pool join waiting time

static void thread_job(void *arg);

/* a task queue which run in thread pool */
struct thread_pool_task {
  void (*process)(void *arg); /**< task callback function */
  void *arg;                  /**< task callback function's arguments */
  struct thread_pool_task *next;
};

/* thread pool struct */
struct thread_pool {
  thread_pool_task_t queue_head; /**< task queue which place all waiting task */
  mutex_t queue_lock;            /**< task queue mutex lock */
  sem_t queue_ready;             /**< a semaphore which for task queue ready */
  uint8_t is_shutdown;           /**< shutdown state  */
  thread_t *thread_list;         /**< thread queue */
  uint8_t alive_worker_num;      /**< current alive thread number */
  uint16_t waiting_task_num;     /**< current waiting task number */
  uint16_t running_task_num;     /**< current excuting task number */
};

/**
 * This function will initialize the thread pool.
 */
thread_pool_t thread_pool_create(uint8_t worker_num, uint32_t worker_stack_size,
                                 uint32_t worker_priority) {
  thread_pool_t pool = (thread_pool_t)heap_alloc(sizeof(struct thread_pool));
  if (pool == NULL) {
    return NULL;
  }
  pool->queue_lock = mutex_create();
  pool->queue_ready = sem_create(0);
  pool->queue_head = NULL;
  pool->alive_worker_num = 0;
  pool->waiting_task_num = 0;
  pool->running_task_num = 0;
  pool->is_shutdown = 0;
  pool->thread_list = (thread_t *)heap_alloc(worker_num * sizeof(thread_t *));
  for (uint8_t i = 0; i < worker_num; i++) {
    pool->thread_list[i] =
        thread_create(thread_job, pool, worker_stack_size, worker_priority);
  }
  return pool;
}

/**
 * This function will submit a task to thread pool.
 *
 * @param pool thread pool pointer
 * @param process task function pointer
 * @param arg task function arguments
 */
void thread_pool_submit(thread_pool_t pool, void (*process)(void *arg),
                        void *arg) {
  thread_pool_task_t member = NULL;
  thread_pool_task_t newtask =
      (thread_pool_task_t)heap_alloc(sizeof(struct thread_pool_task));
  newtask->process = process;
  newtask->arg = arg;
  newtask->next = NULL;
  /* lock thread pool */
  mutex_lock(pool->queue_lock);
  member = pool->queue_head;
  /* task queue is NULL */
  if (member == NULL) {
    pool->queue_head = newtask;
  } else {
    /* look up for queue tail */
    while (member->next != NULL) {
      member = member->next;
    }
    member->next = newtask;
  }
  /* add current waiting thread number */
  pool->waiting_task_num++;
  mutex_unlock(pool->queue_lock);
  /* wake up a waiting thread to process task */
  sem_post(pool->queue_ready);
}

/**
 * This function will delete all wait task and shutdown all worker thread.
 *
 * @param pool thread pool pointer
 */
void thread_pool_shutdown(thread_pool_t pool) {
  mutex_lock(pool->queue_lock);
  pool->is_shutdown = 1;
  mutex_unlock(pool->queue_lock);
  // wake up all waiting thread
  for (uint8_t i = 0; i < pool->alive_worker_num; i++) {
    sem_post(pool->queue_ready);
  }
  // wait all thread exit
  while (pool->alive_worker_num != 0) {
    thread_sleep(THREAD_POOL_JOIN_TIME);
    sem_post(pool->queue_ready);  // make sure all thread wake up
  }
  mutex_lock(pool->queue_lock);
  /* delete all task in queue */
  for (;;) {
    if (pool->queue_head != NULL) {
      heap_free(pool->queue_head);
      pool->queue_head = pool->queue_head->next;
      pool->waiting_task_num--;
    } else {
      break;
    }
  }
  sem_reset(pool->queue_ready, 0);
  mutex_unlock(pool->queue_lock);
}

/**
 * This function will shutdown all worker thread and delete thread pool.
 *
 * @param pool thread pool pointer
 */
void thread_pool_delete(thread_pool_t pool) {
  thread_pool_task_t head = NULL;
  /* wait all thread exit */
  if (!pool->is_shutdown) thread_pool_shutdown(pool);
  /* delete mutex and semaphore then all waiting thread will wake up */
  mutex_delete(pool->queue_lock);
  sem_delete(pool->queue_ready);
  /* release memory */
  heap_free(pool->thread_list);
  pool->thread_list = NULL;
  /* destroy task queue */
  while (pool->queue_head != NULL) {
    head = pool->queue_head;
    pool->queue_head = pool->queue_head->next;
    heap_free(head);
  }
  /* release memory */
  heap_free(pool);
}

/**
 * This function will join until all task finish.
 *
 * @param pool thread pool pointer
 */
void thread_pool_join(thread_pool_t pool) {
  while (pool->waiting_task_num != 0 || pool->running_task_num != 0) {
    thread_sleep(THREAD_POOL_JOIN_TIME);
  }
}

/**
 * This function will return the current number of unfinished task.
 *
 * @param pool thread pool pointer
 *
 * @return the current alive worker number
 */
uint16_t thread_pool_pending_task(thread_pool_t pool) {
  return pool->waiting_task_num + pool->running_task_num;
}

/**
 * This function is thread job.
 *
 * @param arg the thread job arguments
 *
 */
static void thread_job(void *arg) {
  thread_pool_t pool = (thread_pool_t)arg;
  thread_pool_task_t task = NULL;
  mutex_lock(pool->queue_lock);
  pool->alive_worker_num++;
  mutex_unlock(pool->queue_lock);
  while (1) {
    mutex_lock(pool->queue_lock);
    while (pool->waiting_task_num == 0 && !pool->is_shutdown) {
      if (sem_value(pool->queue_ready) == 0) {
        mutex_unlock(pool->queue_lock);
        sem_wait(pool->queue_ready);
        mutex_lock(pool->queue_lock);
      } else {
        sem_wait(pool->queue_ready);
      }
    }
    if (pool->is_shutdown) {
      pool->alive_worker_num--;
      mutex_unlock(pool->queue_lock);
      return;
    }
    /* load task to thread job */
    pool->waiting_task_num--;
    task = pool->queue_head;
    pool->queue_head = task->next;
    pool->running_task_num++;
    mutex_unlock(pool->queue_lock);
    /* run task */
    (*(task->process))(task->arg);
    /* release memory */
    mutex_lock(pool->queue_lock);
    pool->running_task_num--;
    heap_free(task);
    task = NULL;
    mutex_unlock(pool->queue_lock);
  }
}


#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

#include <string.h>

#include "kernel.h"

typedef struct thread_pool_task* thread_pool_task_t;
typedef struct thread_pool* thread_pool_t;

thread_pool_t thread_pool_create(uint8_t worker_num, uint32_t worker_stack_size,
                                 uint32_t worker_priority);
void thread_pool_delete(thread_pool_t pool);
void thread_pool_submit(thread_pool_t pool, void (*process)(void* arg),
                        void* arg);
void thread_pool_shutdown(thread_pool_t pool);
void thread_pool_join(thread_pool_t pool);
uint16_t thread_pool_pending_task(thread_pool_t pool);

#endif

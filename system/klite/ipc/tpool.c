#include "kl_priv.h"

#if KLITE_CFG_OPT_THREAD_POOL

#include <string.h>
struct kl_thread_pool_task {
    void (*process)(void* arg);
    void* arg;
    bool free_arg;
};

static void thread_job(void* arg) {
    kl_thread_pool_t pool = (kl_thread_pool_t)arg;
    struct kl_thread_pool_task temp_task;
    while (1) {
        kl_mqueue_recv(pool->task_queue, &temp_task, KL_WAIT_FOREVER);
        temp_task.process(temp_task.arg);
        if (temp_task.free_arg) {
            kl_heap_free(temp_task.arg);
        }
        kl_mqueue_task_done(pool->task_queue);
    }
}

kl_thread_pool_t kl_thread_pool_create(kl_size_t worker_num,
                                       kl_size_t worker_stack_size,
                                       uint32_t worker_priority,
                                       kl_size_t task_queue_depth) {
    kl_thread_pool_t pool =
        (kl_thread_pool_t)kl_heap_alloc(sizeof(struct kl_thread_pool));
    if (pool == NULL) {
        KL_SET_ERRNO(KL_ENOMEM);
        return NULL;
    }
    pool->worker_num = worker_num;
    pool->task_queue =
        kl_mqueue_create(sizeof(struct kl_thread_pool_task), task_queue_depth);
    if (pool->task_queue == NULL) {
        goto fail;
    }
    pool->thread_list =
        (kl_thread_t*)kl_heap_alloc(worker_num * sizeof(kl_thread_t*));
    if (pool->thread_list == NULL) {
        goto fail;
    }
    for (uint8_t i = 0; i < worker_num; i++) {
        pool->thread_list[i] = kl_thread_create(
            thread_job, pool, worker_stack_size, worker_priority);
        if (pool->thread_list[i] == NULL)
            goto fail;
    }
    return pool;
fail:
    if (pool->task_queue) {
        kl_mqueue_delete(pool->task_queue);
    }
    if (pool->thread_list) {
        kl_heap_free(pool->thread_list);
    }
    for (uint8_t i = 0; i < worker_num; i++) {
        if (pool->thread_list[i]) {
            kl_thread_delete(pool->thread_list[i]);
        }
    }
    kl_heap_free(pool);
    KL_SET_ERRNO(KL_ENOMEM);
    return NULL;
}

void kl_thread_pool_set_slice(kl_thread_pool_t pool, kl_tick_t slice) {
    for (uint8_t i = 0; i < pool->worker_num; i++) {
        kl_thread_set_slice(pool->thread_list[i], slice);
    }
}

bool kl_thread_pool_submit(kl_thread_pool_t pool, void (*process)(void* arg),
                           void* arg, kl_tick_t timeout) {
    struct kl_thread_pool_task task = {
        .process = process,
        .arg = arg,
        .free_arg = false,
    };
    return kl_mqueue_send(pool->task_queue, &task, timeout);
}

bool kl_thread_pool_submit_copy(kl_thread_pool_t pool,
                                void (*process)(void* arg), void* arg,
                                kl_size_t size, kl_tick_t timeout) {
    void* copy = kl_heap_alloc(size);
    if (copy == NULL) {
        KL_SET_ERRNO(KL_ENOMEM);
        return false;
    }
    memcpy(copy, arg, size);
    struct kl_thread_pool_task task = {
        .process = process,
        .arg = copy,
        .free_arg = true,
    };
    if (!kl_mqueue_send(pool->task_queue, &task, timeout)) {
        kl_heap_free(copy);
        return false;
    }
    return true;
}

kl_size_t kl_thread_pool_pending(kl_thread_pool_t pool) {
    return kl_mqueue_pending(pool->task_queue);
}

bool kl_thread_pool_join(kl_thread_pool_t pool, kl_tick_t timeout) {
    return kl_mqueue_join(pool->task_queue, timeout);
}

void kl_thread_pool_shutdown(kl_thread_pool_t pool) {
    /* exit all thread */
    for (uint8_t i = 0; i < pool->worker_num; i++) {
        kl_thread_delete(pool->thread_list[i]);
    }
    struct kl_thread_pool_task temp;
    while (kl_mqueue_recv(pool->task_queue, &temp, 0)) {
        if (temp.free_arg) {
            kl_heap_free(temp.arg);
        }
    }
    /* release memory */
    kl_mqueue_delete(pool->task_queue);
    kl_heap_free(pool->thread_list);
    kl_heap_free(pool);
}

#endif  // KLITE_CFG_OPT_THREAD_POOL

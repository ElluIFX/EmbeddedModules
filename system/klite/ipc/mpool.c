#include "kl_priv.h"

#if KLITE_CFG_IPC_MPOOL

#include <string.h>

#include "kl_slist.h"

kl_mpool_t kl_mpool_create(kl_size_t block_size, kl_size_t block_count) {
    if (block_size == 0 || block_count == 0) {
        return NULL;
    }
    kl_mpool_t mpool;
    kl_size_t msize;
    uint8_t* block;
    msize = sizeof(struct kl_mpool) +
            block_count * (sizeof(struct kl_mpool_node) + block_size);
    mpool = kl_heap_alloc(msize);
    if (mpool == NULL) {
        KL_SET_ERRNO(KL_ENOMEM);
        return NULL;
    }
    memset(mpool, 0, msize);
    block = (uint8_t*)(mpool + 1);
    for (kl_size_t i = 0; i < block_count; i++) {
        kl_slist_append(&mpool->free_list, block);
        block += block_size + sizeof(struct kl_mpool_node);
    }
    return mpool;
}

void kl_mpool_delete(kl_mpool_t mpool) {
    kl_heap_free(mpool);
}

void* kl_mpool_alloc(kl_mpool_t mpool, kl_tick_t timeout) {
    void* block = NULL;
    kl_mutex_lock(&mpool->mutex, KL_WAIT_FOREVER);
    while (mpool->free_list.head == NULL && timeout > 0) {
        kl_cond_wait(&mpool->wait, &mpool->mutex, timeout);
        timeout = kl_sched_tcb_now->timeout;
    }
    if (mpool->free_list.head != NULL) {
        block = mpool->free_list.head;
        kl_slist_remove(&mpool->free_list, block);
    } else {
        KL_SET_ERRNO(KL_ENOMEM);
    }
    kl_mutex_unlock(&mpool->mutex);
    return block;
}

void kl_mpool_free(kl_mpool_t mpool, void* block) {
    if (block == NULL) {
        KL_SET_ERRNO(KL_EINVAL);
        return;
    }
    kl_mutex_lock(&mpool->mutex, KL_WAIT_FOREVER);
    kl_slist_append(&mpool->free_list, block);
    kl_mutex_unlock(&mpool->mutex);
    kl_cond_signal(&mpool->wait);
}

#endif /* KLITE_CFG_IPC_MPOOL */

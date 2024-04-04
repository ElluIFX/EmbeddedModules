#include "klite_internal.h"

#if KLITE_CFG_OPT_MPOOL

#include <string.h>

kl_mpool_t kl_mpool_create(kl_size_t block_size, kl_size_t block_count) {
  kl_size_t i;
  kl_size_t msize;
  kl_mpool_t mpool;
  uint8_t *block;
  msize = sizeof(struct kl_mpool) +
          block_count * (sizeof(kl_size_t *) + block_size);
  mpool = kl_heap_alloc(msize);
  if (mpool != NULL) {
    memset(mpool, 0, msize);
    mpool->mutex = kl_mutex_create();
    mpool->wait = kl_cond_create();
    if (mpool->mutex != NULL) {
      mpool->block_list = (uint8_t **)(mpool + 1);
      mpool->block_count = block_count;
      block = mpool->block_list[block_count];
      for (i = 0; i < block_count; i++) {
        kl_mpool_free(mpool, block);
        block += block_size;
      }
      return mpool;
    }
    kl_heap_free(mpool);
  }
  return NULL;
}

void kl_mpool_delete(kl_mpool_t mpool) {
  kl_mutex_delete(mpool->mutex);
  kl_cond_delete(mpool->wait);
  kl_heap_free(mpool);
}

void *kl_mpool_alloc(kl_mpool_t mpool) {
  void *block = NULL;
  kl_mutex_lock(mpool->mutex);
  if (mpool->free_count > 0) {
    block = mpool->block_list[mpool->free_head];
    mpool->free_count--;
    mpool->free_head++;
    if (mpool->free_head >= mpool->block_count) {
      mpool->free_head = 0;
    }
  }
  kl_mutex_unlock(mpool->mutex);
  return block;
}

void *kl_mpool_timed_alloc(kl_mpool_t mpool, kl_tick_t timeout) {
  void *block = NULL;
  kl_mutex_lock(mpool->mutex);
  while (mpool->free_count == 0 && timeout > 0) {
    timeout = kl_cond_timed_wait(mpool->wait, mpool->mutex, timeout);
  }
  if (mpool->free_count > 0) {
    block = mpool->block_list[mpool->free_head];
    mpool->free_count--;
    mpool->free_head++;
    if (mpool->free_head >= mpool->block_count) {
      mpool->free_head = 0;
    }
  }
  kl_mutex_unlock(mpool->mutex);
  return block;
}

void kl_mpool_free(kl_mpool_t mpool, void *block) {
  if (block == NULL) {
    return;
  }
  kl_mutex_lock(mpool->mutex);
  mpool->block_list[mpool->free_tail] = block;
  mpool->free_count++;
  mpool->free_tail++;
  if (mpool->free_tail >= mpool->block_count) {
    mpool->free_tail = 0;
  }
  kl_cond_signal(mpool->wait);
  kl_mutex_unlock(mpool->mutex);
}

#endif /* KLITE_CFG_OPT_MPOOL */

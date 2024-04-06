#include "kl_priv.h"

#if KLITE_CFG_OPT_MPOOL

#include <string.h>

kl_mpool_t kl_mpool_create(kl_size_t block_size, kl_size_t block_count) {
  kl_size_t msize;
  kl_mpool_t mpool;
  kl_size_t i;
  uint8_t *block;
  msize = sizeof(struct kl_mpool) + block_count * (sizeof(void *) + block_size);
  mpool = kl_heap_alloc(msize);
  if (mpool == NULL) {
    return NULL;
  }
  memset(mpool, 0, msize);
  mpool->block_list = (uint8_t **)(mpool + 1);
  mpool->block_count = block_count;
  block = mpool->block_list[block_count];
  for (i = 0; i < block_count; i++) {
    kl_mpool_free(mpool, block);
    block += block_size;
  }
  return mpool;
}

void kl_mpool_delete(kl_mpool_t mpool) { kl_heap_free(mpool); }

void *kl_mpool_alloc(kl_mpool_t mpool, kl_tick_t timeout) {
  void *block = NULL;
  kl_mutex_lock(&mpool->mutex, KL_WAIT_FOREVER);
  while (mpool->free_count == 0 && timeout > 0) {
    kl_cond_wait(&mpool->wait, &mpool->mutex, timeout);
    timeout = kl_sched_tcb_now->timeout;
  }
  if (mpool->free_count > 0) {
    block = mpool->block_list[mpool->free_head];
    mpool->free_count--;
    mpool->free_head++;
    if (mpool->free_head >= mpool->block_count) {
      mpool->free_head = 0;
    }
  }
  kl_mutex_unlock(&mpool->mutex);
  return block;
}

void kl_mpool_free(kl_mpool_t mpool, void *block) {
  if (block == NULL) {
    return;
  }
  kl_mutex_lock(&mpool->mutex, KL_WAIT_FOREVER);
  mpool->block_list[mpool->free_tail] = block;
  mpool->free_count++;
  mpool->free_tail++;
  if (mpool->free_tail >= mpool->block_count) {
    mpool->free_tail = 0;
  }
  kl_mutex_unlock(&mpool->mutex);
  kl_cond_signal(&mpool->wait);
}

#endif /* KLITE_CFG_OPT_MPOOL */

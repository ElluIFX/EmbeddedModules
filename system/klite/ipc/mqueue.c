#include "kl_priv.h"

#if KLITE_CFG_OPT_MSG_QUEUE

#include <string.h>

#include "kl_list.h"

kl_mqueue_t kl_mqueue_create(kl_size_t msg_size, kl_size_t queue_depth) {
  kl_mqueue_t queue;
  queue = kl_heap_alloc(sizeof(struct kl_mqueue));
  if (queue != NULL) {
    memset(queue, 0, sizeof(struct kl_mqueue));
    queue->size = msg_size;
    queue->sem = kl_sem_create(0);
    queue->mutex = kl_mutex_create();
    queue->mpool =
        kl_mpool_create(sizeof(struct kl_mqueue_node) + msg_size, queue_depth);
    if (queue->mpool == NULL) {
      if (queue->mutex != NULL) {
        kl_mutex_delete(queue->mutex);
      }
      if (queue->sem != NULL) {
        kl_sem_delete(queue->sem);
      }
      kl_heap_free(queue);
      return NULL;
    }
  }
  return queue;
}

void kl_mqueue_delete(kl_mqueue_t queue) {
  kl_mpool_delete(queue->mpool);
  kl_mutex_delete(queue->mutex);
  kl_sem_delete(queue->sem);
  kl_heap_free(queue);
}

void kl_mqueue_clear(kl_mqueue_t queue) {
  struct kl_mqueue_node *node;
  while (kl_sem_timed_take(queue->sem, 0)) {
    kl_mutex_lock(queue->mutex);
    node = queue->head;
    list_remove(queue, node);
    kl_mutex_unlock(queue->mutex);
    kl_mpool_free(queue->mpool, node);
  }
}

bool kl_mqueue_send(kl_mqueue_t queue, void *item, kl_tick_t timeout) {
  struct kl_mqueue_node *node;
  node = kl_mpool_timed_alloc(queue->mpool, timeout);
  if (node != NULL) {
    memset(node, 0, sizeof(struct kl_mqueue_node));
    memcpy(node->data, item, queue->size);
    kl_mutex_lock(queue->mutex);
    list_append(queue, node);
    kl_mutex_unlock(queue->mutex);
    kl_sem_give(queue->sem);
    return true;
  }
  return false;
}

bool kl_mqueue_send_urgent(kl_mqueue_t queue, void *item, kl_tick_t timeout) {
  struct kl_mqueue_node *node;
  node = kl_mpool_timed_alloc(queue->mpool, timeout);
  if (node != NULL) {
    memset(node, 0, sizeof(struct kl_mqueue_node));
    memcpy(node->data, item, queue->size);
    kl_mutex_lock(queue->mutex);
    list_prepend(queue, node);
    kl_mutex_unlock(queue->mutex);
    kl_sem_give(queue->sem);
    return true;
  }
  return false;
}

bool kl_mqueue_recv(kl_mqueue_t queue, void *item, kl_tick_t timeout) {
  struct kl_mqueue_node *node;
  if (kl_sem_timed_take(queue->sem, timeout)) {
    kl_mutex_lock(queue->mutex);
    node = queue->head;
    memcpy(item, node->data, queue->size);
    list_remove(queue, node);
    kl_mutex_unlock(queue->mutex);
    kl_mpool_free(queue->mpool, node);
    return true;
  }
  return false;
}

kl_size_t kl_mqueue_count(kl_mqueue_t queue) {
  kl_size_t count = 0;
  kl_mutex_lock(queue->mutex);
  for (struct kl_mqueue_node *node = queue->head; node != NULL;
       node = node->next) {
    count++;
  }
  kl_mutex_unlock(queue->mutex);
  return count;
}

#endif  // KLITE_CFG_OPT_MSG_QUEUE

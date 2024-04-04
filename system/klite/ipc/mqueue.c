#include "kl_priv.h"

#if KLITE_CFG_OPT_MSG_QUEUE

#include <string.h>

#include "kl_list.h"

kl_msg_queue_t kl_msg_queue_create(kl_size_t msg_size, kl_size_t queue_depth) {
  kl_msg_queue_t queue;
  queue = kl_heap_alloc(sizeof(struct kl_msg_queue));
  if (queue != NULL) {
    memset(queue, 0, sizeof(struct kl_msg_queue));
    queue->size = msg_size;
    queue->sem = kl_sem_create(0);
    queue->mutex = kl_mutex_create();
    queue->mpool = kl_mpool_create(sizeof(struct kl_msg_queue_node) + msg_size,
                                   queue_depth);
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

void kl_msg_queue_delete(kl_msg_queue_t queue) {
  kl_mpool_delete(queue->mpool);
  kl_mutex_delete(queue->mutex);
  kl_sem_delete(queue->sem);
  kl_heap_free(queue);
}

void kl_msg_queue_clear(kl_msg_queue_t queue) {
  struct kl_msg_queue_node *node;
  while (kl_sem_timed_take(queue->sem, 0)) {
    kl_mutex_lock(queue->mutex);
    node = queue->head;
    list_remove(queue, node);
    kl_mutex_unlock(queue->mutex);
    kl_mpool_free(queue->mpool, node);
  }
}

bool kl_msg_queue_send(kl_msg_queue_t queue, void *item, kl_tick_t timeout) {
  struct kl_msg_queue_node *node;
  node = kl_mpool_timed_alloc(queue->mpool, timeout);
  if (node != NULL) {
    memset(node, 0, sizeof(struct kl_msg_queue_node));
    memcpy(node->data, item, queue->size);
    kl_mutex_lock(queue->mutex);
    list_append(queue, node);
    kl_mutex_unlock(queue->mutex);
    kl_sem_give(queue->sem);
    return true;
  }
  return false;
}

bool kl_msg_queue_send_urgent(kl_msg_queue_t queue, void *item,
                              kl_tick_t timeout) {
  struct kl_msg_queue_node *node;
  node = kl_mpool_timed_alloc(queue->mpool, timeout);
  if (node != NULL) {
    memset(node, 0, sizeof(struct kl_msg_queue_node));
    memcpy(node->data, item, queue->size);
    kl_mutex_lock(queue->mutex);
    list_prepend(queue, node);
    kl_mutex_unlock(queue->mutex);
    kl_sem_give(queue->sem);
    return true;
  }
  return false;
}

bool kl_msg_queue_recv(kl_msg_queue_t queue, void *item, kl_tick_t timeout) {
  struct kl_msg_queue_node *node;
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

kl_size_t kl_msg_queue_count(kl_msg_queue_t queue) {
  kl_size_t count = 0;
  kl_mutex_lock(queue->mutex);
  for (struct kl_msg_queue_node *node = queue->head; node != NULL;
       node = node->next) {
    count++;
  }
  kl_mutex_unlock(queue->mutex);
  return count;
}

#endif  // KLITE_CFG_OPT_MSG_QUEUE

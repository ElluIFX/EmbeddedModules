#include "kl_priv.h"

#if KLITE_CFG_OPT_MQUEUE

#include <string.h>

#include "kl_slist.h"

kl_mqueue_t kl_mqueue_create(kl_size_t msg_size, kl_size_t queue_depth) {
  kl_mqueue_t queue;
  kl_size_t qsize;
  kl_size_t i;
  uint8_t *msg;
  qsize = sizeof(struct kl_mqueue) +
          queue_depth * (sizeof(struct kl_mqueue_node) + msg_size);
  queue = kl_heap_alloc(qsize);
  if (queue == NULL) {
    return NULL;
  }
  memset(queue, 0, qsize);
  queue->size = msg_size;
  queue->pending = 0;
  msg = (uint8_t *)(queue + 1);
  for (i = 0; i < queue_depth; i++) {
    kl_slist_append(&queue->empty_list, msg);
    msg += sizeof(struct kl_mqueue_node) + msg_size;
  }
  return queue;
}

void kl_mqueue_delete(kl_mqueue_t queue) { kl_heap_free(queue); }

void kl_mqueue_clear(kl_mqueue_t queue) {
  struct kl_mqueue_node *node = NULL;
  kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
  while (queue->msg_list.head != NULL) {
    node = queue->msg_list.head;
    kl_slist_remove(&queue->msg_list, node);
    kl_slist_append(&queue->empty_list, node);
  }
  queue->pending = 0;
  kl_mutex_unlock(&queue->mutex);
  kl_cond_broadcast(&queue->write);
  kl_cond_broadcast(&queue->join);
}

bool kl_mqueue_send(kl_mqueue_t queue, void *item, kl_tick_t timeout) {
  struct kl_mqueue_node *node;
  kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
  while (queue->empty_list.head == NULL && timeout > 0) {
    kl_cond_wait(&queue->write, &queue->mutex, timeout);
    timeout = kl_sched_tcb_now->timeout;
  }
  if (queue->empty_list.head != NULL) {
    node = queue->empty_list.head;
    kl_slist_remove(&queue->empty_list, node);
  }
  kl_mutex_unlock(&queue->mutex);
  if (node != NULL) {
    memset(node, 0, sizeof(struct kl_mqueue_node) + queue->size);
    memcpy(node->data, item, queue->size);
    kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
    kl_slist_append(&queue->msg_list, node);
    queue->pending++;
    kl_mutex_unlock(&queue->mutex);
    kl_cond_signal(&queue->read);
    return true;
  }
  return false;
}

bool kl_mqueue_send_urgent(kl_mqueue_t queue, void *item, kl_tick_t timeout) {
  struct kl_mqueue_node *node;
  kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
  while (queue->empty_list.head == NULL && timeout > 0) {
    kl_cond_wait(&queue->write, &queue->mutex, timeout);
    timeout = kl_sched_tcb_now->timeout;
  }
  if (queue->empty_list.head != NULL) {
    node = queue->empty_list.head;
    kl_slist_remove(&queue->empty_list, node);
  }
  kl_mutex_unlock(&queue->mutex);
  if (node != NULL) {
    memset(node, 0, sizeof(struct kl_mqueue_node) + queue->size);
    memcpy(node->data, item, queue->size);
    kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
    kl_slist_prepend(&queue->msg_list, node);
    queue->pending++;
    kl_mutex_unlock(&queue->mutex);
    kl_cond_signal(&queue->read);
    return true;
  }
  return false;
}

bool kl_mqueue_recv(kl_mqueue_t queue, void *item, kl_tick_t timeout) {
  struct kl_mqueue_node *node;
  kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
  while (queue->msg_list.head == NULL && timeout > 0) {
    kl_cond_wait(&queue->read, &queue->mutex, timeout);
    timeout = kl_sched_tcb_now->timeout;
  }
  if (queue->msg_list.head != NULL) {
    node = queue->msg_list.head;
    kl_slist_remove(&queue->msg_list, node);
  }
  kl_mutex_unlock(&queue->mutex);
  if (node != NULL) {
    memcpy(item, node->data, queue->size);
    kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
    kl_slist_append(&queue->empty_list, node);
    kl_mutex_unlock(&queue->mutex);
    kl_cond_signal(&queue->write);
    return true;
  }
  return false;
}

kl_size_t kl_mqueue_count(kl_mqueue_t queue) {
  kl_size_t count = 0;
  struct kl_mqueue_node *node;
  kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
  for (node = queue->msg_list.head; node != NULL; node = node->next) {
    count++;
  }
  kl_mutex_unlock(&queue->mutex);
  return count;
}

kl_size_t kl_mqueue_pending(kl_mqueue_t queue) { return queue->pending; }

void kl_mqueue_task_done(kl_mqueue_t queue) {
  kl_mutex_lock(&queue->mutex, KL_WAIT_FOREVER);
  queue->pending--;
  kl_mutex_unlock(&queue->mutex);
  if (queue->pending == 0) {
    kl_cond_broadcast(&queue->join);
  }
}

bool kl_mqueue_join(kl_mqueue_t queue, kl_tick_t timeout) {
  if (queue->pending == 0) {
    return true;
  }
  if (timeout == 0) {
    return false;
  }
  return kl_cond_wait_complete(&queue->join, timeout);
}

#endif  // KLITE_CFG_OPT_MQUEUE

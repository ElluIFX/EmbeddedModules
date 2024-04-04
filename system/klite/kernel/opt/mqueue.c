/******************************************************************************
 * Copyright (c) 2015-2023 jiangxiaogang<kerndev@foxmail.com>
 *
 * This file is part of KLite distribution.
 *
 * KLite is free software, you can redistribute it and/or modify it under
 * the MIT Licence.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include "klite_internal.h"

#if KLITE_CFG_OPT_MSG_QUEUE

#include <string.h>

#include "klite_internal_list.h"

kl_msg_queue_t kl_msg_queue_create(uint32_t msg_size, uint32_t queue_depth) {
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

void kl_msg_queue_send(kl_msg_queue_t queue, void *item) {
  struct kl_msg_queue_node *node;
  node = kl_mpool_blocked_alloc(queue->mpool);
  node->prev = NULL;
  node->next = NULL;
  memcpy(node->data, item, queue->size);
  kl_mutex_lock(queue->mutex);
  list_append(queue, node);
  kl_mutex_unlock(queue->mutex);
  kl_sem_give(queue->sem);
}

void kl_msg_queue_recv(kl_msg_queue_t queue, void *item) {
  struct kl_msg_queue_node *node;
  kl_sem_take(queue->sem);
  kl_mutex_lock(queue->mutex);
  node = queue->head;
  memcpy(item, node->data, queue->size);
  list_remove(queue, node);
  kl_mutex_unlock(queue->mutex);
  kl_mpool_free(queue->mpool, node);
}

bool kl_msg_queue_timed_send(kl_msg_queue_t queue, void *item,
                             kl_tick_t timeout) {
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

bool kl_msg_queue_timed_recv(kl_msg_queue_t queue, void *item,
                             kl_tick_t timeout) {
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

uint32_t kl_msg_queue_count(kl_msg_queue_t queue) {
  uint32_t count = 0;
  kl_mutex_lock(queue->mutex);
  for (struct kl_msg_queue_node *node = queue->head; node != NULL;
       node = node->next) {
    count++;
  }
  kl_mutex_unlock(queue->mutex);
  return count;
}

#endif  // KLITE_CFG_OPT_MSG_QUEUE

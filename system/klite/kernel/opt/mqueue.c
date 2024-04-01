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

#include "klite.h"

#if KLITE_CFG_OPT_MSG_QUEUE

#include <string.h>

#include "klite_internal_list.h"

struct msg_queue_node {
  struct msg_queue_node *prev;
  struct msg_queue_node *next;
  uint8_t data[4];
};

struct msg_queue {
  struct msg_queue_node *head;
  struct msg_queue_node *tail;
  sem_t sem;
  mutex_t mutex;
  mpool_t mpool;
  uint32_t size;
};

msg_queue_t msg_queue_create(uint32_t item_size, uint32_t queue_depth) {
  msg_queue_t queue;
  queue = heap_alloc(sizeof(struct msg_queue));
  if (queue != NULL) {
    memset(queue, 0, sizeof(struct msg_queue));
    queue->size = item_size;
    queue->sem = sem_create(0);
    queue->mutex = mutex_create();
    queue->mpool =
        mpool_create(sizeof(struct msg_queue_node) + item_size, queue_depth);
    if (queue->mpool == NULL) {
      if (queue->mutex != NULL) {
        mutex_delete(queue->mutex);
      }
      if (queue->sem != NULL) {
        sem_delete(queue->sem);
      }
      heap_free(queue);
      return NULL;
    }
  }
  return queue;
}

void msg_queue_delete(msg_queue_t queue) {
  mpool_delete(queue->mpool);
  mutex_delete(queue->mutex);
  sem_delete(queue->sem);
  heap_free(queue);
}

void msg_queue_clear(msg_queue_t queue) {
  struct msg_queue_node *node;
  while (sem_timed_take(queue->sem, 0)) {
    mutex_lock(queue->mutex);
    node = queue->head;
    list_remove(queue, node);
    mutex_unlock(queue->mutex);
    mpool_free(queue->mpool, node);
  }
}

void msg_queue_send(msg_queue_t queue, void *item) {
  struct msg_queue_node *node;
  node = mpool_blocked_alloc(queue->mpool);
  memset(node, 0, sizeof(struct msg_queue_node));
  memcpy(node->data, item, queue->size);
  mutex_lock(queue->mutex);
  list_append(queue, node);
  mutex_unlock(queue->mutex);
  sem_give(queue->sem);
}

void msg_queue_recv(msg_queue_t queue, void *item) {
  struct msg_queue_node *node;
  sem_take(queue->sem);
  mutex_lock(queue->mutex);
  node = queue->head;
  memcpy(item, node->data, queue->size);
  list_remove(queue, node);
  mutex_unlock(queue->mutex);
  mpool_free(queue->mpool, node);
}

bool msg_queue_timed_send(msg_queue_t queue, void *item, klite_tick_t timeout) {
  struct msg_queue_node *node;
  node = mpool_timed_alloc(queue->mpool, timeout);
  if (node != NULL) {
    memset(node, 0, sizeof(struct msg_queue_node));
    memcpy(node->data, item, queue->size);
    mutex_lock(queue->mutex);
    list_append(queue, node);
    mutex_unlock(queue->mutex);
    sem_give(queue->sem);
    return true;
  }
  return false;
}

bool msg_queue_timed_recv(msg_queue_t queue, void *item, klite_tick_t timeout) {
  struct msg_queue_node *node;
  if (sem_timed_take(queue->sem, timeout)) {
    mutex_lock(queue->mutex);
    node = queue->head;
    memcpy(item, node->data, queue->size);
    list_remove(queue, node);
    mutex_unlock(queue->mutex);
    mpool_free(queue->mpool, node);
    return true;
  }
  return false;
}

uint32_t msg_queue_count(msg_queue_t queue) {
  uint32_t count = 0;
  mutex_lock(queue->mutex);
  for (struct msg_queue_node *node = queue->head; node != NULL;
       node = node->next) {
    count++;
  }
  mutex_unlock(queue->mutex);
  return count;
}

#endif  // KLITE_CFG_OPT_MSG_QUEUE

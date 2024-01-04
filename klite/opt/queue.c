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
#include "queue.h"

#include <string.h>

#include "kernel.h"
#include "list.h"
#include "mpool.h"

struct queue_node {
  struct queue_node *prev;
  struct queue_node *next;
  uint8_t data[4];
};

struct queue {
  struct queue_node *head;
  struct queue_node *tail;
  sem_t sem;
  mutex_t mutex;
  mpool_t mpool;
  uint32_t size;
};

queue_t queue_create(uint32_t item_size, uint32_t queue_depth) {
  queue_t queue;
  queue = heap_alloc( sizeof(struct queue));
  if (queue != NULL) {
    memset(queue, 0, sizeof(struct queue));
    queue->size = item_size;
    queue->sem = sem_create(0);
    queue->mutex = mutex_create();
    queue->mpool =
        mpool_create(sizeof(struct queue_node) + item_size, queue_depth);
    if (queue->mpool == NULL) {
      if (queue->mutex != NULL) {
        mutex_delete(queue->mutex);
      }
      if (queue->sem != NULL) {
        sem_delete(queue->sem);
      }
      heap_free( queue);
      return NULL;
    }
  }
  return queue;
}

void queue_delete(queue_t queue) {
  mpool_delete(queue->mpool);
  mutex_delete(queue->mutex);
  sem_delete(queue->sem);
  heap_free( queue);
}

bool queue_send(queue_t queue, void *item, uint32_t timeout) {
  struct queue_node *node;
  node = mpool_alloc(queue->mpool);
  if (node != NULL) {
    memset(node, 0, sizeof(struct queue_node));
    memcpy(node->data, item, queue->size);
    mutex_lock(queue->mutex);
    list_append(queue, node);
    mutex_unlock(queue->mutex);
    sem_post(queue->sem);
    return true;
  }
  return false;
}

bool queue_recv(queue_t queue, void *item, uint32_t timeout) {
  struct queue_node *node;
  if (sem_timed_wait(queue->sem, timeout)) {
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

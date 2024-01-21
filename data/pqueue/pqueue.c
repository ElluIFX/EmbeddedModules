// Copyright 2023 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <stdlib.h>
#include <string.h>

#ifndef PQUEUE_STATIC
#include "pqueue.h"
#else
#define PQUEUE_EXTERN static
#endif

#ifndef PQUEUE_EXTERN
#define PQUEUE_EXTERN
#endif

struct pqueue {
  size_t elsize;
  int (*compare)(const void *a, const void *b, void *udata);
  void *udata;
  char *items;
  size_t len;
  size_t cap;
};

PQUEUE_EXTERN
struct pqueue *pqueue_new(size_t elsize,
                          int (*compare)(const void *a, const void *b,
                                         void *udata),
                          void *udata) {
  struct pqueue *queue = m_alloc(sizeof(struct pqueue));
  if (!queue) return NULL;
  queue->elsize = elsize;
  queue->compare = compare;
  queue->udata = udata;
  queue->cap = 8;
  queue->len = 0;
  queue->items = m_alloc(elsize * (queue->cap + 1));
  if (!queue->items) {
    free(queue);
    return NULL;
  }
  return queue;
}

PQUEUE_EXTERN
void pqueue_free(struct pqueue *queue) {
  if (queue->items) m_free(queue->items);
  m_free(queue);
}

PQUEUE_EXTERN
void pqueue_clear(struct pqueue *queue) { queue->len = 0; }

static void *get(struct pqueue *queue, size_t index) {
  return queue->items + (queue->elsize * index);
}

static void set(struct pqueue *queue, const void *item, size_t index) {
  memcpy(get(queue, index), item, queue->elsize);
}

static bool push(struct pqueue *queue, const void *item) {
  if (queue->len == queue->cap) {
    size_t cap = queue->cap * 2;
    char *items = m_realloc(queue->items, queue->elsize * (cap + 1));
    if (!items) return false;
    queue->items = items;
    queue->cap = cap;
  }
  set(queue, item, queue->len++);
  return true;
}

static void swap(struct pqueue *queue, size_t i, size_t j) {
  set(queue, get(queue, i), queue->len);
  set(queue, get(queue, j), i);
  set(queue, get(queue, queue->len), j);
}

static void *swap_remove(struct pqueue *queue, size_t i) {
  set(queue, get(queue, i), queue->len);
  set(queue, get(queue, queue->len - 1), i);
  return get(queue, queue->len--);
}

static int compare(struct pqueue *queue, size_t i, size_t j) {
  return queue->compare(get(queue, i), get(queue, j), queue->udata);
}

PQUEUE_EXTERN
bool pqueue_push(struct pqueue *queue, const void *item) {
  if (!push(queue, item)) return false;
  size_t i = queue->len - 1;
  while (i != 0) {
    size_t parent = (i - 1) / 2;
    if (!(compare(queue, parent, i) > 0)) break;
    swap(queue, parent, i);
    i = parent;
  }
  return true;
}

PQUEUE_EXTERN
size_t pqueue_count(const struct pqueue *queue) { return queue->len; }

PQUEUE_EXTERN
const void *pqueue_peek(const struct pqueue *queue) {
  return queue->len == 0 ? NULL : queue->items;
}

PQUEUE_EXTERN
const void *pqueue_pop(struct pqueue *queue) {
  if (queue->len == 0) return NULL;
  void *item = swap_remove(queue, 0);
  size_t i = 0;
  while (1) {
    size_t smallest = i;
    size_t left = i * 2 + 1;
    size_t right = i * 2 + 2;
    if (left < queue->len && compare(queue, left, smallest) <= 0) {
      smallest = left;
    }
    if (right < queue->len && compare(queue, right, smallest) <= 0) {
      smallest = right;
    }
    if (smallest == i) {
      break;
    }
    swap(queue, smallest, i);
    i = smallest;
  }
  return item;
}

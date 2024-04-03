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
#ifndef __KLITE_LIST_H
#define __KLITE_LIST_H

#include <stddef.h>

typedef struct node {
  struct node *prev;
  struct node *next;
} node_t;

typedef struct {
  node_t *head;
  node_t *tail;
} list_t;

static void list_init(void *list) {
  ((list_t *)list)->head = NULL;
  ((list_t *)list)->tail = NULL;
}

static void node_init(void *node) {
  ((node_t *)node)->prev = NULL;
  ((node_t *)node)->next = NULL;
}

static inline void list_remove(void *list, void *node) {
  if (((node_t *)node)->prev == NULL) {
    ((list_t *)list)->head = ((node_t *)node)->next;
  } else {
    ((node_t *)node)->prev->next = ((node_t *)node)->next;
  }
  if (((node_t *)node)->next == NULL) {
    ((list_t *)list)->tail = ((node_t *)node)->prev;
  } else {
    ((node_t *)node)->next->prev = ((node_t *)node)->prev;
  }
  ((node_t *)node)->prev = NULL;
  ((node_t *)node)->next = NULL;
}

static inline void list_insert_before(void *list, void *before, void *node) {
  ((node_t *)node)->next = before;
  if (before == NULL) {
    ((node_t *)node)->prev = ((list_t *)list)->tail;
    ((list_t *)list)->tail = node;
  } else {
    ((node_t *)node)->prev = ((node_t *)before)->prev;
    ((node_t *)before)->prev = node;
  }
  if (((node_t *)node)->prev == NULL) {
    ((list_t *)list)->head = node;
  } else {
    ((node_t *)node)->prev->next = node;
  }
}

static inline void list_insert_after(void *list, void *after, void *node) {
  ((node_t *)node)->prev = after;
  if (after == NULL) {
    ((node_t *)node)->next = ((list_t *)list)->head;
    ((list_t *)list)->head = node;
  } else {
    ((node_t *)node)->next = ((node_t *)after)->next;
    ((node_t *)after)->next = node;
  }
  if (((node_t *)node)->next == NULL) {
    ((list_t *)list)->tail = node;
  } else {
    ((node_t *)node)->next->prev = node;
  }
}

#define list_prepend(list, node) list_insert_after(list, NULL, node)
#define list_append(list, node) list_insert_before(list, NULL, node)

#endif

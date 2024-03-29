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
#ifndef __LIST_H
#define __LIST_H

#include <stddef.h>

struct __node {
  struct __node *prev;
  struct __node *next;
};

struct __list {
  struct __node *head;
  struct __node *tail;
};

static void list_init(void *list) {
  ((struct __list *)list)->head = NULL;
  ((struct __list *)list)->tail = NULL;
}

static void node_init(void *node) {
  ((struct __node *)node)->prev = NULL;
  ((struct __node *)node)->next = NULL;
}

static inline void list_remove(void *list, void *node) {
  if (((struct __node *)node)->prev == NULL) {
    ((struct __list *)list)->head = ((struct __node *)node)->next;
  } else {
    ((struct __node *)node)->prev->next = ((struct __node *)node)->next;
  }
  if (((struct __node *)node)->next == NULL) {
    ((struct __list *)list)->tail = ((struct __node *)node)->prev;
  } else {
    ((struct __node *)node)->next->prev = ((struct __node *)node)->prev;
  }
  ((struct __node *)node)->prev = NULL;
  ((struct __node *)node)->next = NULL;
}

static inline void list_insert_before(void *list, void *before, void *node) {
  ((struct __node *)node)->next = before;
  if (before == NULL) {
    ((struct __node *)node)->prev = ((struct __list *)list)->tail;
    ((struct __list *)list)->tail = node;
  } else {
    ((struct __node *)node)->prev = ((struct __node *)before)->prev;
    ((struct __node *)before)->prev = node;
  }
  if (((struct __node *)node)->prev == NULL) {
    ((struct __list *)list)->head = node;
  } else {
    ((struct __node *)node)->prev->next = node;
  }
}

static inline void list_insert_after(void *list, void *after, void *node) {
  ((struct __node *)node)->prev = after;
  if (after == NULL) {
    ((struct __node *)node)->next = ((struct __list *)list)->head;
    ((struct __list *)list)->head = node;
  } else {
    ((struct __node *)node)->next = ((struct __node *)after)->next;
    ((struct __node *)after)->next = node;
  }
  if (((struct __node *)node)->next == NULL) {
    ((struct __list *)list)->tail = node;
  } else {
    ((struct __node *)node)->next->prev = node;
  }
}

#define list_prepend(list, node) list_insert_after(list, NULL, node)
#define list_append(list, node) list_insert_before(list, NULL, node)

#endif

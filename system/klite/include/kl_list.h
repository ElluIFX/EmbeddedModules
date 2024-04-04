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

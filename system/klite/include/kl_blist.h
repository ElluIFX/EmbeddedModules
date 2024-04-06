#ifndef __KLITE_BLIST_H
#define __KLITE_BLIST_H

#include <stddef.h>

typedef struct __b_node {
  struct __b_node *prev;
  struct __b_node *next;
} __b_node_t;

typedef struct {
  __b_node_t *head;
  __b_node_t *tail;
} __b_list_t;

static inline void kl_blist_remove(void *list, void *node) {
  if (((__b_node_t *)node)->prev == NULL) {
    ((__b_list_t *)list)->head = ((__b_node_t *)node)->next;
  } else {
    ((__b_node_t *)node)->prev->next = ((__b_node_t *)node)->next;
  }
  if (((__b_node_t *)node)->next == NULL) {
    ((__b_list_t *)list)->tail = ((__b_node_t *)node)->prev;
  } else {
    ((__b_node_t *)node)->next->prev = ((__b_node_t *)node)->prev;
  }
  ((__b_node_t *)node)->prev = NULL;
  ((__b_node_t *)node)->next = NULL;
}

static inline void kl_blist_insert_before(void *list, void *before,
                                          void *node) {
  ((__b_node_t *)node)->next = before;
  if (before == NULL) {
    ((__b_node_t *)node)->prev = ((__b_list_t *)list)->tail;
    ((__b_list_t *)list)->tail = node;
  } else {
    ((__b_node_t *)node)->prev = ((__b_node_t *)before)->prev;
    ((__b_node_t *)before)->prev = node;
  }
  if (((__b_node_t *)node)->prev == NULL) {
    ((__b_list_t *)list)->head = node;
  } else {
    ((__b_node_t *)node)->prev->next = node;
  }
}

static inline void kl_blist_insert_after(void *list, void *after, void *node) {
  ((__b_node_t *)node)->prev = after;
  if (after == NULL) {
    ((__b_node_t *)node)->next = ((__b_list_t *)list)->head;
    ((__b_list_t *)list)->head = node;
  } else {
    ((__b_node_t *)node)->next = ((__b_node_t *)after)->next;
    ((__b_node_t *)after)->next = node;
  }
  if (((__b_node_t *)node)->next == NULL) {
    ((__b_list_t *)list)->tail = node;
  } else {
    ((__b_node_t *)node)->next->prev = node;
  }
}

#define kl_blist_prepend(list, node) kl_blist_insert_after(list, NULL, node)
#define kl_blist_append(list, node) kl_blist_insert_before(list, NULL, node)

#endif

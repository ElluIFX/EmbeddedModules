#ifndef __KLITE_SLIST_H
#define __KLITE_SLIST_H

#include <stddef.h>

typedef struct __s_node {
    struct __s_node* next;
} __s_node_t;

typedef struct {
    __s_node_t* head;
} __s_list_t;

static inline __s_node_t* __find_prev_node(__s_list_t* list, __s_node_t* node) {
    __s_node_t* prev = list->head;
    while (prev && prev->next != node && prev->next != NULL) {
        prev = prev->next;
    }
    return prev;
}

static inline void kl_slist_remove(void* list, void* node) {
    if (((__s_node_t*)node) == ((__s_list_t*)list)->head) {
        ((__s_list_t*)list)->head = ((__s_node_t*)node)->next;

    } else {
        __s_node_t* prev = __find_prev_node(list, node);
        if (prev) {
            prev->next = ((__s_node_t*)node)->next;
        } else {
            ((__s_list_t*)list)->head = ((__s_node_t*)node)->next;
        }
    }
    ((__s_node_t*)node)->next = NULL;
}

static inline void kl_slist_insert_before(void* list, void* before,
                                          void* node) {
    ((__s_node_t*)node)->next = before;
    __s_node_t* prev = __find_prev_node(list, before);
    if (prev) {
        prev->next = node;
    } else {
        ((__s_list_t*)list)->head = node;
    }
}

static inline void kl_slist_insert_after(void* list, void* after, void* node) {
    if (after == NULL) {
        ((__s_node_t*)node)->next = ((__s_list_t*)list)->head;
        ((__s_list_t*)list)->head = node;
    } else {
        ((__s_node_t*)node)->next = ((__s_node_t*)after)->next;
        ((__s_node_t*)after)->next = node;
    }
}

#define kl_slist_prepend(list, node) kl_slist_insert_after(list, NULL, node)
#define kl_slist_append(list, node) kl_slist_insert_before(list, NULL, node)

#endif

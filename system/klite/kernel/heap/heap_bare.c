#include "klite_internal.h"

#if KLITE_CFG_HEAP_USE_BARE
#include <string.h>

#define MEM_ALIGN_BYTE (8)
#define MEM_ALIGN_MASK (MEM_ALIGN_BYTE - 1)
#define MEM_ALIGN_PAD(m) (((m) + MEM_ALIGN_MASK) & (~MEM_ALIGN_MASK))
#define MEM_ALIGN_CUT(m) ((m) & (~MEM_ALIGN_MASK))

struct heap_node {
  struct heap_node *prev;
  struct heap_node *next;
  kl_size_t used;
};

struct heap {
  struct kl_thread_list list;
  kl_size_t lock;
  kl_size_t size;
  struct heap_node *head;
  struct heap_node *free;
};
static struct heap *heap;

static struct heap_node *find_free_node(struct heap_node *node) {
  kl_size_t free;
  for (; node->next != NULL; node = node->next) {
    free = ((kl_size_t)node->next) - ((kl_size_t)node) - node->used;
    if (free > sizeof(struct heap_node)) {
      break;
    }
  }
  return node;
}

static void heap_node_init(kl_size_t start, kl_size_t end) {
  struct heap_node *node;
  node = (struct heap_node *)start;
  heap->head = node;
  heap->free = node;
  node->used = sizeof(struct heap_node);
  node->prev = NULL;
  node->next = (struct heap_node *)(end - sizeof(struct heap_node));
  node = node->next;
  node->used = sizeof(struct heap_node);
  node->prev = heap->head;
  node->next = NULL;
}

static void heap_mutex_lock(void) {
  kl_port_enter_critical();
  if (heap->lock == 0) {
    heap->lock = 1;
  } else {
    kl_sched_tcb_wait(kl_sched_tcb_now, &heap->list);
    kl_sched_switch();
  }
  kl_port_leave_critical();
}

static void heap_mutex_unlock(void) {
  kl_port_enter_critical();
  if (kl_sched_tcb_wake_from(&heap->list)) {
    kl_sched_preempt(false);
  } else {
    heap->lock = 0;
  }
  kl_port_leave_critical();
}

void kl_heap_init(void *addr, kl_size_t size) {
  kl_size_t start;
  kl_size_t end;
  heap = (struct heap *)addr;
  start = MEM_ALIGN_PAD((kl_size_t)(heap + 1));
  end = MEM_ALIGN_CUT((kl_size_t)addr + size);
  memset(heap, 0, sizeof(struct heap));
  heap->size = size;
  heap_node_init(start, end);
}

void *kl_heap_alloc(kl_size_t size) {
  kl_size_t free;
  kl_size_t need;
  struct heap_node *temp;
  struct heap_node *node;
  void *mem = NULL;
  need = MEM_ALIGN_PAD(size + sizeof(struct heap_node));
  heap_mutex_lock();
  for (node = heap->free; node->next != NULL; node = node->next) {
    free = ((kl_size_t)node->next) - ((kl_size_t)node) - node->used;
    if (free >= need) {
      temp = (struct heap_node *)((kl_size_t)node + node->used);
      temp->prev = node;
      temp->next = node->next;
      temp->used = need;
      node->next->prev = temp;
      node->next = temp;
      if (node == heap->free) {
        heap->free = find_free_node(node);
      }
      mem = (void *)(temp + 1);
      break;
    }
  }
  if (!mem) mem = kl_heap_alloc_fault_callback(size);
  heap_mutex_unlock();
  return mem;
}

void kl_heap_free(void *mem) {
  struct heap_node *node;
  node = (struct heap_node *)mem - 1;
  heap_mutex_lock();
  if (node->prev->next == node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    if (node->prev < heap->free) {
      heap->free = node->prev;
    }
  }
  heap_mutex_unlock();
}

void *kl_heap_realloc(void *mem, kl_size_t size) {
  void *new_mem;
  new_mem = kl_heap_alloc(size);
  if (new_mem) {
    memcpy(new_mem, mem, size);
    kl_heap_free(mem);
  }
  if (!new_mem) {
    new_mem = kl_heap_alloc_fault_callback(size);
    if (new_mem) {
      memmove(new_mem, mem, size);
      kl_heap_free(mem);
    }
  }
  return new_mem;
}

void kl_heap_info(kl_size_t *used, kl_size_t *free) {
  kl_size_t sum = 0;
  struct heap_node *node;
  heap_mutex_lock();
  for (node = heap->head; node->next != NULL; node = node->next) {
    sum += ((kl_size_t)node->next) - ((kl_size_t)node) - node->used;
  }
  if (used != NULL) {
    *used = heap->size - sum;
  }
  if (free != NULL) {
    *free = sum;
  }
  heap_mutex_unlock();
}

float kl_heap_usage(void) {
  kl_size_t used;
  kl_size_t free;
  kl_heap_info(&used, &free);
  return (float)used / (float)(used + free);
}
#endif  // !HEAP_USE_UMM

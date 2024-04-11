#include "kl_priv.h"

#if KLITE_CFG_HEAP_USE_BUILTIN
#include <string.h>

// fill freeed memory with initial value
#define MEM_INIT_VALUE (0x00)
// memory alignment mask
#define MEM_ALIGN_MASK (KLITE_CFG_HEAP_ALIGN_BYTE - 1)
// pad up memory size to align
#define MEM_ALIGN_PAD(m) (((m) + MEM_ALIGN_MASK) & (~MEM_ALIGN_MASK))
// cut down memory size to align
#define MEM_ALIGN_CUT(m) ((m) & (~MEM_ALIGN_MASK))
// node type
#define MEM_NODE(node) ((heap_node_t)(node))
// get node from user space memory address
#define MEM_GET_NODE(mem) (MEM_NODE(mem) - 1)
// node magic value (hEAp)
#define MEM_NODE_MAGIC (0xEA)
// node avail size
#define MEM_NODE_AVAIL(node) \
  (((kl_size_t)(MEM_NODE(node)->next) - (kl_size_t)(MEM_NODE(node))))
// node used size
#define MEM_NODE_USED(node) ((MEM_NODE(node)->used) & 0xFFFFFF)
// set node used size
#define MEM_NODE_USED_BUILD(used) (((used) & 0xFFFFFF) | MEM_NODE_MAGIC << 24)
// check node is valid by checking magic value
#define MEM_NODE_VALID(node) \
  ((node) != NULL && (((MEM_NODE(node)->used) >> 24) & 0xFF) == MEM_NODE_MAGIC)

struct heap_node {
#if KLITE_CFG_HEAP_STORAGE_PREV_NODE
  struct heap_node *prev;
#endif
  struct heap_node *next;
  kl_size_t used;  // 8bit magic + 24bit used size
#if KLITE_CFG_HEAP_TRACE_OWNER
  kl_thread_t owner;
#endif
};

struct heap {
  kl_size_t size;           // total size
  kl_size_t avail;          // available size
  kl_size_t minimum_avail;  // minimum available size
  kl_size_t alloc_count;    // alloc operation count
  kl_size_t free_count;     // free operation count
  struct heap_node *head;   // first node of heap
  struct heap_node *free;   // first node with free space
};

typedef struct heap_node *heap_node_t;

typedef struct heap *heap_t;

static heap_t heap;

__KL_HEAP_MUTEX_IMPL__

static inline void heap_node_init(kl_size_t start, kl_size_t end) {
  heap_node_t node;

  node = (heap_node_t)start;
  heap->head = node;
  heap->free = node;
  node->used = MEM_NODE_USED_BUILD(sizeof(struct heap_node));
#if KLITE_CFG_HEAP_STORAGE_PREV_NODE
  node->prev = NULL;
#endif
  node->next = (heap_node_t)(end - sizeof(struct heap_node));
  node = node->next;
  node->used = MEM_NODE_USED_BUILD(sizeof(struct heap_node));
#if KLITE_CFG_HEAP_STORAGE_PREV_NODE
  node->prev = heap->head;
#endif
  node->next = NULL;
  heap->avail -= 2 * sizeof(struct heap_node);
}

static inline heap_node_t next_free_node(heap_node_t node) {
  kl_size_t free;

  for (; node->next != NULL; node = node->next) {
    free = MEM_NODE_AVAIL(node) - MEM_NODE_USED(node);
    if (free > sizeof(struct heap_node)) {
      break;
    }
  }
  return node;
}

static inline heap_node_t alloc_node(kl_size_t need) {
  heap_node_t new_node, temp;
  kl_size_t free;
  heap_node_t node = NULL;
#if KLITE_CFG_HEAP_USE_BESTFIT
  kl_size_t min_free = ~(kl_size_t)0;
#endif
  for (temp = heap->free; temp->next != NULL; temp = temp->next) {
    free = MEM_NODE_AVAIL(temp) - MEM_NODE_USED(temp);
#if KLITE_CFG_HEAP_USE_BESTFIT
    if (free >= need && free < min_free) {
      node = temp;
      min_free = free;
    }
#else  // first fit
    if (free >= need) {
      node = temp;
      break;
    }
#endif
  }
  if (NULL == node) {
    return NULL;
  }
  new_node = (heap_node_t)((kl_size_t)node + MEM_NODE_USED(node));
  new_node->next = node->next;
  new_node->used = MEM_NODE_USED_BUILD(need);
#if KLITE_CFG_HEAP_STORAGE_PREV_NODE
  new_node->prev = node;
  node->next->prev = new_node;
#endif
#if KLITE_CFG_HEAP_TRACE_OWNER
  new_node->owner = kl_sched_tcb_now;
#endif
  node->next = new_node;
  if (node == heap->free) {
    heap->free = next_free_node(node);
  }
  // heap info update
  heap->avail -= need;
  if (heap->avail < heap->minimum_avail) {
    heap->minimum_avail = heap->avail;
  }
  heap->alloc_count++;
  return new_node;
}

#if !KLITE_CFG_HEAP_STORAGE_PREV_NODE
static inline heap_node_t find_prev_node(heap_node_t node) {
  for (heap_node_t prev = heap->head; prev->next != NULL; prev = prev->next) {
    if (prev->next == node) {
      return prev;
    }
  }
  return NULL;
}
#endif

static inline void free_node(heap_node_t node) {
  if (!MEM_NODE_VALID(node)) {  // invalid node
    return;
  }
#if KLITE_CFG_HEAP_STORAGE_PREV_NODE
  node->prev->next = node->next;
  node->next->prev = node->prev;
  if (node->prev < heap->free) {
    heap->free = node->prev;
  }
#else
  heap_node_t prev = find_prev_node(node);
  if (!prev) return;
  prev->next = node->next;
  if (prev < heap->free) {
    heap->free = prev;
  }
#endif
  // heap info update
  heap->avail += MEM_NODE_USED(node);
  heap->free_count++;
  node->used = 0;  // mark as invalid
#if KLITE_CFG_HEAP_TRACE_OWNER
  node->owner = NULL;
#endif
#if KLITE_CFG_HEAP_CLEAR_MEMORY_ON_FREE
  memset(node, MEM_INIT_VALUE, MEM_NODE_USED(node));
#endif
}

void kl_heap_init(void *addr, kl_size_t size) {
  kl_size_t start;
  kl_size_t end;

  heap = (heap_t)addr;
  start = MEM_ALIGN_PAD((kl_size_t)(heap + 1));
  end = MEM_ALIGN_CUT((kl_size_t)addr + size);
  memset(heap, MEM_INIT_VALUE, sizeof(struct heap));
  heap->size = size;
  heap->avail = end - start;
  heap->minimum_avail = heap->avail;
  heap->alloc_count = 0;
  heap->free_count = 0;
  heap_node_init(start, end);
}

void *kl_heap_alloc(kl_size_t size) {
  if (size == 0) {
    return NULL;
  }
  kl_size_t need = MEM_ALIGN_PAD(size + sizeof(struct heap_node));
  heap_node_t node;
  void *mem;

  heap_mutex_lock();
  node = alloc_node(need);
  if (node) {
    mem = (void *)(node + 1);
  } else {
    mem = kl_heap_alloc_fault_hook(size);
  }
  heap_mutex_unlock();
  return mem;
}

void kl_heap_free(void *mem) {
  if ((kl_size_t)mem < (kl_size_t)heap->head ||
      (kl_size_t)mem >= (kl_size_t)heap + heap->size) {
    return;  // invalid address
  }
  heap_mutex_lock();
  free_node(MEM_GET_NODE(mem));
  heap_mutex_unlock();
}

void *kl_heap_realloc(void *mem, kl_size_t size) {
  if (mem == NULL) {
    return kl_heap_alloc(size);
  }
  if (size == 0) {
    kl_heap_free(mem);
    return NULL;
  }

  void *new_mem = NULL;
  heap_node_t node;
  heap_node_t temp;
  kl_size_t need = MEM_ALIGN_PAD(size + sizeof(struct heap_node));
  kl_size_t avail;

  heap_mutex_lock();
  node = MEM_GET_NODE(mem);
  if (!MEM_NODE_VALID(node)) goto fin;  // invalid node
  if (need == MEM_NODE_USED(node)) {    // same size
    new_mem = mem;
    goto fin;
  }
  avail = MEM_NODE_AVAIL(node);
  if (avail >= need) {  // in-place realloc
    // heap info update
    if (need > MEM_NODE_USED(node)) {
      heap->avail -= need - MEM_NODE_USED(node);
      if (heap->avail < heap->minimum_avail) {
        heap->minimum_avail = heap->avail;
      }
    } else {
      heap->avail += MEM_NODE_USED(node) - need;
#if KLITE_CFG_HEAP_CLEAR_MEMORY_ON_FREE
      memset((void *)(((kl_size_t)node) + need), MEM_INIT_VALUE,
             MEM_NODE_USED(node) - need);
#endif
    }
    // update node
    node->used = MEM_NODE_USED_BUILD(need);
    new_mem = mem;
#if KLITE_CFG_HEAP_TRACE_OWNER  // update owner
    node->owner = kl_sched_tcb_now;
#endif
  } else {  // new alloc and copy
    temp = alloc_node(need);
    if (temp) {
      new_mem = (void *)(temp + 1);
    } else {
      new_mem = kl_heap_alloc_fault_hook(size);
    }
    if (new_mem) {
      memmove(new_mem, mem, size);
      free_node(node);
    }
  }
fin:
  heap_mutex_unlock();
  return new_mem;
}

void kl_heap_stats(kl_heap_stats_t stats) {
  heap_node_t node;
  kl_size_t free;

  heap_mutex_lock();
  stats->total_size = heap->size;
  stats->avail_size = heap->avail;
  stats->minimum_ever_avail = heap->minimum_avail;
  stats->alloc_count = heap->alloc_count;
  stats->free_count = heap->free_count;
  stats->largest_free = 0;
  stats->second_largest_free = 0;
  stats->smallest_free = ~(kl_size_t)0;
  stats->free_blocks = 0;
  for (node = heap->head; node->next != NULL; node = node->next) {
    free = MEM_NODE_AVAIL(node) - MEM_NODE_USED(node);
    if (free > 0) {
      stats->free_blocks++;
      if (free > stats->largest_free) {
        stats->second_largest_free = stats->largest_free;
        stats->largest_free = free;
      }
      if (free < stats->smallest_free) {
        stats->smallest_free = free;
      }
    }
  }
  if (stats->second_largest_free == 0) {
    stats->second_largest_free = stats->smallest_free;
  }
  heap_mutex_unlock();
}

#if KLITE_CFG_HEAP_TRACE_OWNER

bool kl_heap_iter_nodes(void **iter_tmp, kl_thread_t *owner, kl_size_t *addr,
                        kl_size_t *used, kl_size_t *avail) {
  heap_node_t node;

  heap_mutex_lock();
  if (*iter_tmp == NULL) {
    node = heap->head;
  } else {
    node = MEM_NODE(*iter_tmp);
  }
  if (node->next != NULL && MEM_NODE_VALID(node)) {
    *owner = node->owner;
    *addr = (kl_size_t)(node + 1);
    *used = MEM_NODE_USED(node);
    *avail = MEM_NODE_AVAIL(node);
    *iter_tmp = (void *)node->next;
    heap_mutex_unlock();
    return true;
  }
  *iter_tmp = NULL;
  heap_mutex_unlock();
  return false;
}

#endif  // KLITE_CFG_HEAP_TRACE_OWNER

#endif  // KLITE_CFG_HEAP_USE_BUILTIN

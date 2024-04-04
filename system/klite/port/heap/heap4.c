#include "kl_priv.h"

#if KLITE_CFG_HEAP_USE_HEAP4
#include <string.h>

#include "heap4.h"

__KL_HEAP_MUTEX_IMPL__

void kl_heap_init(void *addr, kl_size_t size) {
  heap4_init((uint8_t *)addr, size);
}

void *kl_heap_alloc(kl_size_t size) {
  heap_mutex_lock();
  void *mem = heap4_alloc(size);
  if (!mem) mem = kl_heap_alloc_fault_callback(size);
  heap_mutex_unlock();
  return mem;
}

void kl_heap_free(void *mem) {
  heap_mutex_lock();
  heap4_free(mem);
  heap_mutex_unlock();
}

void *kl_heap_realloc(void *mem, kl_size_t size) {
  heap_mutex_lock();
  void *new_mem = heap4_realloc(mem, size);
  if (!new_mem) {
    new_mem = kl_heap_alloc_fault_callback(size);
    if (new_mem) {
      memmove(new_mem, mem, size);
      heap4_free(mem);
    }
  }
  heap_mutex_unlock();
  return new_mem;
}

void kl_heap_info(kl_size_t *used, kl_size_t *free) {
  heap_mutex_lock();
  *free = heap4_get_free_size();
  *used = heap4_get_total_size() - *free;
  heap_mutex_unlock();
}

float kl_heap_usage(void) {
  heap_mutex_lock();
  float usage = (float)(heap4_get_total_size() - heap4_get_free_size()) /
                heap4_get_total_size();
  heap_mutex_unlock();
  return usage;
}

#endif

#include "klite_internal.h"

#if KLITE_CFG_HEAP_USE_LWMEM
#include <string.h>

#include "lwmem.h"

volatile static uint8_t heap_lock = 0;
static struct kl_thread_list heap_waitlist;
static void heap_mutex_lock(void) {
  kl_port_enter_critical();
  if (heap_lock == 0) {
    heap_lock = 1;
  } else {
    kl_sched_tcb_wait(kl_sched_tcb_now, &heap_waitlist);
    kl_sched_switch();
  }
  kl_port_leave_critical();
}

static void heap_mutex_unlock(void) {
  kl_port_enter_critical();
  if (kl_sched_tcb_wake_from(&heap_waitlist)) {
    kl_sched_preempt(false);
  } else {
    heap_lock = 0;
  }
  kl_port_leave_critical();
}

void kl_heap_init(void *addr, kl_size_t size) {
  static lwmem_region_t regions[] = {
      /* Set start address and size of each region */
      {NULL, 0},
      {NULL, 0},
  };
  regions[0].start_addr = addr;
  regions[0].size = size;
  lwmem_assignmem(regions);
}

void *kl_heap_alloc(kl_size_t size) {
  heap_mutex_lock();
  void *mem = lwmem_malloc(size);
  if (!mem) mem = kl_heap_alloc_fault_callback(size);
  heap_mutex_unlock();
  return mem;
}

void kl_heap_free(void *mem) {
  heap_mutex_lock();
  lwmem_free(mem);
  heap_mutex_unlock();
}

void *kl_heap_realloc(void *mem, kl_size_t size) {
  heap_mutex_lock();
  void *new_mem = lwmem_realloc(mem, size);
  if (!new_mem) {
    new_mem = kl_heap_alloc_fault_callback(size);
    if (new_mem) {
      memmove(new_mem, mem, size);
      lwmem_free(mem);
    }
  }
  heap_mutex_unlock();
  return new_mem;
}

void kl_heap_info(kl_size_t *used, kl_size_t *free) {
  lwmem_stats_t stats;
  heap_mutex_lock();
  lwmem_get_stats(&stats);
  heap_mutex_unlock();
  *free = stats.mem_available_bytes;
  *used = stats.mem_size_bytes - stats.mem_available_bytes;
}

float kl_heap_usage(void) {
  lwmem_stats_t stats;
  heap_mutex_lock();
  lwmem_get_stats(&stats);
  heap_mutex_unlock();
  return (float)(stats.mem_size_bytes - stats.mem_available_bytes) /
         (float)stats.mem_size_bytes;
}

#endif

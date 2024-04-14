#include "kl_priv.h"

#if KLITE_CFG_HEAP_USE_LWMEM
#include <string.h>

#include "lwmem.h"

__KL_HEAP_MUTEX_IMPL__

void kl_heap_init(void* addr, kl_size_t size) {
    static lwmem_region_t regions[] = {
        /* Set start address and size of each region */
        {NULL, 0},
        {NULL, 0},
    };
    regions[0].start_addr = addr;
    regions[0].size = size;
    lwmem_assignmem(regions);
}

void* kl_heap_alloc(kl_size_t size) {
    heap_mutex_lock();
    void* mem = lwmem_malloc(size);
    if (!mem)
        mem = kl_heap_alloc_fault_hook(size);
    heap_mutex_unlock();
    return mem;
}

void kl_heap_free(void* mem) {
    heap_mutex_lock();
    lwmem_free(mem);
    heap_mutex_unlock();
}

void* kl_heap_realloc(void* mem, kl_size_t size) {
    heap_mutex_lock();
    void* new_mem = lwmem_realloc(mem, size);
    if (!new_mem) {
        new_mem = kl_heap_alloc_fault_hook(size);
        if (new_mem) {
            memmove(new_mem, mem, size);
            lwmem_free(mem);
        }
    }
    heap_mutex_unlock();
    return new_mem;
}

void kl_heap_stats(kl_heap_stats_t* stats) {
    lwmem_stats_t lw_stats;
    heap_mutex_lock();
    lwmem_get_stats(&lw_stats);
    heap_mutex_unlock();
    stats->total_size = lw_stats.mem_size_bytes;
    stats->avail_size = lw_stats.mem_available_bytes;
    stats->largest_free = 0;         // not supported
    stats->second_largest_free = 0;  // not supported
    stats->smallest_free = 0;        // not supported
    stats->free_blocks = 0;          // not supported
    stats->minimum_ever_avail = lw_stats.minimum_ever_mem_available_bytes;
    stats->alloc_count = lw_stats.nr_alloc;
    stats->free_count = lw_stats.nr_free;
}

#endif

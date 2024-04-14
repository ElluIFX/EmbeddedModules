#include "kl_priv.h"

#if KLITE_CFG_HEAP_USE_HEAP4
#include <string.h>

#include "heap4.h"

__KL_HEAP_MUTEX_IMPL__

void kl_heap_init(void* addr, kl_size_t size) {
    prvHeapInit((uint8_t*)addr, size);
}

void* kl_heap_alloc(kl_size_t size) {
    heap_mutex_lock();
    void* mem = pvPortMalloc(size);
    if (!mem)
        mem = kl_heap_alloc_fault_hook(size);
    heap_mutex_unlock();
    return mem;
}

void kl_heap_free(void* mem) {
    heap_mutex_lock();
    vPortFree(mem);
    heap_mutex_unlock();
}

void* kl_heap_realloc(void* mem, kl_size_t size) {
    heap_mutex_lock();
    void* new_mem = pvPortRealloc(mem, size);
    if (!new_mem) {
        new_mem = kl_heap_alloc_fault_hook(size);
        if (new_mem) {
            memmove(new_mem, mem, size);
            vPortFree(mem);
        }
    }
    heap_mutex_unlock();
    return new_mem;
}

void kl_heap_stats(kl_heap_stats_t stats) {
    HeapStats_t heap4_stats;
    heap_mutex_lock();
    vPortGetHeapStats(&heap4_stats);
    stats->total_size = xPortGetTotalHeapSize();
    heap_mutex_unlock();
    stats->avail_size = heap4_stats.xAvailableHeapSpaceInBytes;
    stats->largest_free = heap4_stats.xSizeOfLargestFreeBlockInBytes;
    stats->smallest_free = heap4_stats.xSizeOfSmallestFreeBlockInBytes;
    stats->free_blocks = heap4_stats.xNumberOfFreeBlocks;
    stats->minimum_ever_avail = heap4_stats.xMinimumEverFreeBytesRemaining;
    stats->alloc_count = heap4_stats.xNumberOfSuccessfulAllocations;
    stats->free_count = heap4_stats.xNumberOfSuccessfulFrees;
    stats->second_largest_free = 0;  // not supported
}

#endif

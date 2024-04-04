#ifndef HEAP_4_H
#define HEAP_4_H

#include "modules.h"

/*               TYPEDEF                */

typedef struct Heap4Stats {
  size_t xAvailableHeapSpaceInBytes;
  size_t xSizeOfLargestFreeBlockInBytes;
  size_t xSizeOfSmallestFreeBlockInBytes;
  size_t xNumberOfFreeBlocks;
  size_t xMinimumEverFreeBytesRemaining;
  size_t xNumberOfSuccessfulAllocations;
  size_t xNumberOfSuccessfulFrees;
} Heap4Stats_t;

/*              EXPORTED FUNCTION                */

extern void heap4_init(uint8_t* heap, size_t heap_size);

extern void* heap4_alloc(size_t xWantedSize);

extern void* heap4_calloc(size_t xNum, size_t xSize);

extern void* heap4_realloc(void* pv, size_t xWantedSize);

extern void heap4_free(void* pv);

extern size_t heap4_get_free_size(void);

extern size_t heap4_get_total_size(void);

extern size_t heap4_get_minimum_free_size(void);

extern void heap4_get_stats(Heap4Stats_t* pxHeapStats);

#endif /* HEAP_4_H */

#ifndef HEAP_4_H
#define HEAP_4_H

#include "modules.h"

/*               TYPEDEF                */

typedef struct HeapStats {
  size_t xAvailableHeapSpaceInBytes;
  size_t xSizeOfLargestFreeBlockInBytes;
  size_t xSizeOfSmallestFreeBlockInBytes;
  size_t xNumberOfFreeBlocks;
  size_t xMinimumEverFreeBytesRemaining;
  size_t xNumberOfSuccessfulAllocations;
  size_t xNumberOfSuccessfulFrees;
} HeapStats_t;

/*              EXPORTED FUNCTION                */

extern void prvHeapInit(uint8_t* heap, size_t heap_size);

extern void* pvPortMalloc(size_t xWantedSize);

extern void* pvPortCalloc(size_t xNum, size_t xSize);

extern void* pvPortRealloc(void* pv, size_t xWantedSize);

extern void vPortFree(void* pv);

extern size_t xPortGetFreeHeapSize(void);

extern size_t xPortGetTotalHeapSize(void);

extern size_t xPortGetMinimumEverFreeHeapSize(void);

extern void vPortGetHeapStats(HeapStats_t* pxHeapStats);

#endif /* HEAP_4_H */

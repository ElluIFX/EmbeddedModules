#ifndef HEAP_4_H
#define HEAP_4_H

#include "internal.h"
#include "kernel.h"

/*               CONFIGURATION                */

#define configUSE_MALLOC_FAILED_HOOK 0

#define configHEAP_CLEAR_MEMORY_ON_FREE 0

#define portBYTE_ALIGNMENT 0x04

#define portBYTE_ALIGNMENT_MASK (portBYTE_ALIGNMENT - 1)

/*               PORTING FOR NON-FREERTOS                */

#define vTaskSuspendAll() ((void)0)

#define xTaskResumeAll() ((void)0)

#define taskENTER_CRITICAL() cpu_enter_critical()

#define taskEXIT_CRITICAL() cpu_leave_critical()

#define traceMALLOC(pvReturn, xWantedSize) ((void)0)

#define traceFREE(pv, xSize) ((void)0)

#define configASSERT(x) ((void)0)

#define PRIVILEGED_DATA

#define PRIVILEGED_FUNCTION

#define portPOINTER_SIZE_TYPE size_t

#define portMAX_DELAY UINT32_MAX

#define mtCOVERAGE_TEST_MARKER() ((void)0)

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

/*
 * FreeRTOS Kernel V10.4.6
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include "heap4.h"

#include <stdlib.h>
#include <string.h>

/*               CONFIGURATION                */

#define configUSE_MALLOC_FAILED_HOOK 0

#define configHEAP_CLEAR_MEMORY_ON_FREE 0

#define portBYTE_ALIGNMENT 0x04

#define portBYTE_ALIGNMENT_MASK (portBYTE_ALIGNMENT - 1)

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE ((size_t)8)

/* Max value that fits in a size_t type. */
#define heapSIZE_MAX (~((size_t)0))

/*               PORTING FOR NON-FREERTOS                */

#define vTaskSuspendAll() ((void)0)

#define xTaskResumeAll() ((void)0)

#if MOD_CFG_USE_OS_KLITE
#include "klite.h"
#define taskENTER_CRITICAL() kl_kernel_enter_critical()

#define taskEXIT_CRITICAL() kl_kernel_exit_critical()
#else
#define taskENTER_CRITICAL() ((void)0)

#define taskEXIT_CRITICAL() ((void)0)
#endif

#define traceMALLOC(pvReturn, xWantedSize) ((void)0)

#define traceFREE(pv, xSize) ((void)0)

#define configASSERT(x) ((void)0)

#define portPOINTER_SIZE_TYPE size_t

#define portMAX_DELAY UINT32_MAX

/* Check if multiplying a and b will result in overflow. */
#define heapMULTIPLY_WILL_OVERFLOW(a, b) \
  (((a) > 0) && ((b) > (heapSIZE_MAX / (a))))

/* Check if adding a and b will result in overflow. */
#define heapADD_WILL_OVERFLOW(a, b) ((a) > (heapSIZE_MAX - (b)))

/* MSB of the xBlockSize member of an BlockLink_t structure is used to track
 * the allocation status of a block.  When MSB of the xBlockSize member of
 * an BlockLink_t structure is set then the block belongs to the application.
 * When the bit is free the block is still part of the free heap space. */
#define heapBLOCK_ALLOCATED_BITMASK \
  (((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1))
#define heapBLOCK_SIZE_IS_VALID(xBlockSize) \
  (((xBlockSize) & heapBLOCK_ALLOCATED_BITMASK) == 0)
#define heapBLOCK_IS_ALLOCATED(pxBlock) \
  (((pxBlock->xBlockSize) & heapBLOCK_ALLOCATED_BITMASK) != 0)
#define heapALLOCATE_BLOCK(pxBlock) \
  ((pxBlock->xBlockSize) |= heapBLOCK_ALLOCATED_BITMASK)
#define heapFREE_BLOCK(pxBlock) \
  ((pxBlock->xBlockSize) &= ~heapBLOCK_ALLOCATED_BITMASK)

/*-----------------------------------------------------------*/

static uint8_t* ucHeap;
static size_t ucHeapSize = 0U;

/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK {
  struct A_BLOCK_LINK* pxNextFreeBlock; /*<< The next free block in the list. */
  size_t xBlockSize;                    /*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList(BlockLink_t* pxBlockToInsert);

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
 * block must by correctly byte aligned. */
static const size_t xHeapStructSize =
    (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) &
    ~((size_t)portBYTE_ALIGNMENT_MASK);

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart;
static BlockLink_t* pxEnd = NULL;

/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;
static size_t xNumberOfSuccessfulAllocations = 0;
static size_t xNumberOfSuccessfulFrees = 0;

/*-----------------------------------------------------------*/

void* heap4_alloc(size_t xWantedSize) {
  if (ucHeap == NULL || xWantedSize == 0) return NULL;
  BlockLink_t* pxBlock;
  BlockLink_t* pxPreviousBlock;
  BlockLink_t* pxNewBlockLink;
  void* pvReturn = NULL;
  size_t xAdditionalRequiredSize;

  vTaskSuspendAll();
  {
    if (xWantedSize > 0) {
      /* The wanted size must be increased so it can contain a BlockLink_t
       * structure in addition to the requested amount of bytes. Some
       * additional increment may also be needed for alignment. */
      xAdditionalRequiredSize = xHeapStructSize + portBYTE_ALIGNMENT -
                                (xWantedSize & portBYTE_ALIGNMENT_MASK);

      if (heapADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize) == 0) {
        xWantedSize += xAdditionalRequiredSize;
      } else {
        xWantedSize = 0;
      }
    }
    /* Check the block size we are trying to allocate is not so large that the
     * top bit is set.  The top bit of the block size member of the BlockLink_t
     * structure is used to determine who owns the block - the application or
     * the kernel, so it must be free. */
    if (heapBLOCK_SIZE_IS_VALID(xWantedSize) != 0) {
      if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining)) {
        /* Traverse the list from the start (lowest address) block until
         * one of adequate size is found. */
        pxPreviousBlock = &xStart;
        pxBlock = xStart.pxNextFreeBlock;

        while ((pxBlock->xBlockSize < xWantedSize) &&
               (pxBlock->pxNextFreeBlock != NULL)) {
          pxPreviousBlock = pxBlock;
          pxBlock = pxBlock->pxNextFreeBlock;
        }

        /* If the end marker was reached then a block of adequate size
         * was not found. */
        if (pxBlock != pxEnd) {
          /* Return the memory space pointed to - jumping over the
           * BlockLink_t structure at its start. */
          pvReturn = (void*)(((uint8_t*)pxPreviousBlock->pxNextFreeBlock) +
                             xHeapStructSize);

          /* This block is being returned for use so must be taken out
           * of the list of free blocks. */
          pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

          /* If the block is larger than required it can be split into
           * two. */
          if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE) {
            /* This block is to be split into two.  Create a new
             * block following the number of bytes requested. The void
             * cast is used to prevent byte alignment warnings from the
             * compiler. */
            pxNewBlockLink = (void*)(((uint8_t*)pxBlock) + xWantedSize);
            configASSERT((((size_t)pxNewBlockLink) & portBYTE_ALIGNMENT_MASK) ==
                         0);

            /* Calculate the sizes of two blocks split from the
             * single block. */
            pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
            pxBlock->xBlockSize = xWantedSize;

            /* Insert the new block into the list of free blocks. */
            prvInsertBlockIntoFreeList(pxNewBlockLink);
          }
          xFreeBytesRemaining -= pxBlock->xBlockSize;

          if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining) {
            xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
          }
          /* The block is being returned - it is allocated and owned
           * by the application and has no "next" block. */
          heapALLOCATE_BLOCK(pxBlock);
          pxBlock->pxNextFreeBlock = NULL;
          xNumberOfSuccessfulAllocations++;
        }
      }
    }
    traceMALLOC(pvReturn, xWantedSize);
  }
  (void)xTaskResumeAll();

#if (configUSE_MALLOC_FAILED_HOOK == 1)
  {
    if (pvReturn == NULL) {
      extern void vApplicationMallocFailedHook(void);
      vApplicationMallocFailedHook();
    }
  }
#endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

  configASSERT((((size_t)pvReturn) & (size_t)portBYTE_ALIGNMENT_MASK) == 0);
  return pvReturn;
}
/*-----------------------------------------------------------*/

void heap4_free(void* pv) {
  if (pv == NULL || ucHeap == NULL) return;
  uint8_t* puc = (uint8_t*)pv;
  BlockLink_t* pxLink;

  if (pv != NULL) {
    /* The memory being freed will have an BlockLink_t structure immediately
     * before it. */
    puc -= xHeapStructSize;

    /* This casting is to keep the compiler from issuing warnings. */
    pxLink = (void*)puc;

    configASSERT(heapBLOCK_IS_ALLOCATED(pxLink) != 0);
    configASSERT(pxLink->pxNextFreeBlock == NULL);

    if (heapBLOCK_IS_ALLOCATED(pxLink) != 0) {
      if (pxLink->pxNextFreeBlock == NULL) {
        /* The block is being returned to the heap - it is no longer
         * allocated. */
        heapFREE_BLOCK(pxLink);
#if (configHEAP_CLEAR_MEMORY_ON_FREE == 1)
        {
          (void)memset(puc + xHeapStructSize, 0,
                       pxLink->xBlockSize - xHeapStructSize);
        }
#endif

        vTaskSuspendAll();
        {
          /* Add this block to the list of free blocks. */
          xFreeBytesRemaining += pxLink->xBlockSize;
          traceFREE(pv, pxLink->xBlockSize);
          prvInsertBlockIntoFreeList(((BlockLink_t*)pxLink));
          xNumberOfSuccessfulFrees++;
        }
        (void)xTaskResumeAll();
      }
    }
  }
}

void* heap4_realloc(void* pv, size_t xWantedSize) {
  if (pv == NULL || ucHeap == NULL) return NULL;
  if (xWantedSize == 0) {
    heap4_free(pv);
    return NULL;
  }
  BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
  void* pvReturn = NULL;

  BlockLink_t *pxBlockold, *pxBlockjudge;
  vTaskSuspendAll();
  {
    /* Check the requested block size is not so large that the top bit is
    set.  The top bit of the block size member of the BlockLink_t structure
    is used to determine who owns the block - the application or the
    kernel, so it must be free. */
    if (heapBLOCK_SIZE_IS_VALID(xWantedSize)) {
      if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00) {
        /* Byte alignment required. */
        xWantedSize +=
            (portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK));
        configASSERT((xWantedSize & portBYTE_ALIGNMENT_MASK) == 0);
      }
      if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining)) {
        if (pv == NULL) {
          pvReturn = heap4_alloc(xWantedSize);
          (void)xTaskResumeAll();
          return pvReturn;
        }
        pxBlockold =
            (BlockLink_t*)(pv -
                           xHeapStructSize);  // 找到源地址对应的BlockLink_t结构体，提取其中的xBlockSize信息
        pxBlockjudge =
            (BlockLink_t*)((uint8_t*)pxBlockold +
                           ((pxBlockold->xBlockSize) &
                            (~heapBLOCK_ALLOCATED_BITMASK)));  // 找到源地址对应的块的尾部。

        pxPreviousBlock = &xStart;
        pxBlock = xStart.pxNextFreeBlock;
        while (pxBlock != pxBlockjudge &&
               (pxBlock->pxNextFreeBlock !=
                NULL))  // 判断源地址块后的下一块是否可用
        {
          pxPreviousBlock = pxBlock;
          pxBlock = pxBlock->pxNextFreeBlock;
        }
        if ((xWantedSize < pxBlock->xBlockSize &&
             ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)) &&
            pxBlock ==
                pxBlockjudge)  // 源地址块的下一块地址可用，并且其块大小大于申请的大小
        {
          pxBlockold->xBlockSize += xWantedSize;
          pxNewBlockLink = (BlockLink_t*)((uint8_t*)pxBlock + xWantedSize);
          pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;

          pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
          /* Insert the new block into the list of free blocks. */
          prvInsertBlockIntoFreeList(pxNewBlockLink);
          pxBlock->pxNextFreeBlock = NULL;
          pvReturn = pv;
          xFreeBytesRemaining -= xWantedSize;

          if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining) {
            xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
          }
        } else {
          pvReturn = heap4_alloc(
              (((pxBlockold->xBlockSize) & (~heapBLOCK_ALLOCATED_BITMASK)) -
               xHeapStructSize) +
              xWantedSize);  // 下一块不可用，重新申请一块地址空间，大小为原申请空间加上现在申请空间
          memcpy((uint8_t*)pvReturn, pv,
                 (pxBlockold->xBlockSize &
                  (~heapBLOCK_ALLOCATED_BITMASK) - xHeapStructSize));
          heap4_free(pv);  // 释放源地址空间
        }
      }
    }
  }
  (void)xTaskResumeAll();
  configASSERT((((size_t)pvReturn) & (size_t)portBYTE_ALIGNMENT_MASK) == 0);
  return pvReturn;
}

void* heap4_calloc(size_t xNum, size_t xSize) {
  void* pv = NULL;

  if (heapMULTIPLY_WILL_OVERFLOW(xNum, xSize) == 0) {
    pv = heap4_alloc(xNum * xSize);

    if (pv != NULL) {
      (void)memset(pv, 0, xNum * xSize);
    }
  }

  return pv;
}
/*-----------------------------------------------------------*/

size_t heap4_get_free_size(void) { return xFreeBytesRemaining; }

/*-----------------------------------------------------------*/

size_t heap4_get_total_size(void) { return ucHeapSize; }

/*-----------------------------------------------------------*/

size_t heap4_get_minimum_free_size(void) {
  return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks(void) {
  /* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

void heap4_init(uint8_t* heap, size_t heap_size) {
  taskENTER_CRITICAL();

  ucHeap = heap;
  ucHeapSize = heap_size;

  BlockLink_t* pxFirstFreeBlock;
  uint8_t* pucAlignedHeap;
  portPOINTER_SIZE_TYPE uxAddress;
  size_t xTotalHeapSize = heap_size;

  /* Ensure the heap starts on a correctly aligned boundary. */
  uxAddress = (portPOINTER_SIZE_TYPE)ucHeap;

  if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0) {
    uxAddress += (portBYTE_ALIGNMENT - 1);
    uxAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
    xTotalHeapSize -= uxAddress - (portPOINTER_SIZE_TYPE)ucHeap;
  }

  pucAlignedHeap = (uint8_t*)uxAddress;

  /* xStart is used to hold a pointer to the first item in the list of free
   * blocks.  The void cast is used to prevent compiler warnings. */
  xStart.pxNextFreeBlock = (void*)pucAlignedHeap;
  xStart.xBlockSize = (size_t)0;

  /* pxEnd is used to mark the end of the list of free blocks and is inserted
   * at the end of the heap space. */
  uxAddress = ((portPOINTER_SIZE_TYPE)pucAlignedHeap) + xTotalHeapSize;
  uxAddress -= xHeapStructSize;
  uxAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
  pxEnd = (BlockLink_t*)uxAddress;
  pxEnd->xBlockSize = 0;
  pxEnd->pxNextFreeBlock = NULL;

  /* To start with there is a single free block that is sized to take up the
   * entire heap space, minus the space taken by pxEnd. */
  pxFirstFreeBlock = (BlockLink_t*)pucAlignedHeap;
  pxFirstFreeBlock->xBlockSize =
      (size_t)(uxAddress - (portPOINTER_SIZE_TYPE)pxFirstFreeBlock);
  pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

  /* Only one block exists - and it covers the entire usable heap space. */
  xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
  xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

  taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList(BlockLink_t* pxBlockToInsert) /*  */
{
  BlockLink_t* pxIterator;
  uint8_t* puc;

  /* Iterate through the list until a block is found that has a higher address
   * than the block being inserted. */
  for (pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert;
       pxIterator = pxIterator->pxNextFreeBlock) {
    /* Nothing to do here, just iterate to the right position. */
  }

  /* Do the block being inserted, and the block it is being inserted after
   * make a contiguous block of memory? */
  puc = (uint8_t*)pxIterator;

  if ((puc + pxIterator->xBlockSize) == (uint8_t*)pxBlockToInsert) {
    pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
    pxBlockToInsert = pxIterator;
  }
  /* Do the block being inserted, and the block it is being inserted before
   * make a contiguous block of memory? */
  puc = (uint8_t*)pxBlockToInsert;

  if ((puc + pxBlockToInsert->xBlockSize) ==
      (uint8_t*)pxIterator->pxNextFreeBlock) {
    if (pxIterator->pxNextFreeBlock != pxEnd) {
      /* Form one big block from the two blocks. */
      pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
      pxBlockToInsert->pxNextFreeBlock =
          pxIterator->pxNextFreeBlock->pxNextFreeBlock;
    } else {
      pxBlockToInsert->pxNextFreeBlock = pxEnd;
    }
  } else {
    pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
  }

  /* If the block being inserted plugged a gab, so was merged with the block
   * before and the block after, then it's pxNextFreeBlock pointer will have
   * already been set, and should not be set here as that would make it point
   * to itself. */
  if (pxIterator != pxBlockToInsert) {
    pxIterator->pxNextFreeBlock = pxBlockToInsert;
  }
}
/*-----------------------------------------------------------*/

void heap4_get_stats(Heap4Stats_t* pxHeapStats) {
  BlockLink_t* pxBlock;
  size_t xBlocks = 0, xMaxSize = 0,
         xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of
                                      getting the maximum value. */

  vTaskSuspendAll();
  {
    pxBlock = xStart.pxNextFreeBlock;

    /* pxBlock will be NULL if the heap has not been initialised.  The heap
     * is initialised automatically when the first allocation is made. */
    if (pxBlock != NULL) {
      while (pxBlock != pxEnd) {
        /* Increment the number of blocks and record the largest block seen
         * so far. */
        xBlocks++;

        if (pxBlock->xBlockSize > xMaxSize) {
          xMaxSize = pxBlock->xBlockSize;
        }

        if (pxBlock->xBlockSize < xMinSize) {
          xMinSize = pxBlock->xBlockSize;
        }

        /* Move to the next block in the chain until the last block is
         * reached. */
        pxBlock = pxBlock->pxNextFreeBlock;
      }
    }
  }
  (void)xTaskResumeAll();

  pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
  pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
  pxHeapStats->xNumberOfFreeBlocks = xBlocks;

  taskENTER_CRITICAL();
  {
    pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining;
    pxHeapStats->xNumberOfSuccessfulAllocations =
        xNumberOfSuccessfulAllocations;
    pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees;
    pxHeapStats->xMinimumEverFreeBytesRemaining =
        xMinimumEverFreeBytesRemaining;
  }
  taskEXIT_CRITICAL();
}

/*
 * Copyright 2021 Alexey Vasilenko
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "dalloc.h"

#include "log.h"

#if ALLOC_INFO_U16
#define _OFFSET 15
#else
#define _OFFSET 31
#endif

#define FREEFLAG_GET(arr) ((arr.alloc_info) >> _OFFSET & 0x01)
#define FREEFLAG_SET(arr) ((arr.alloc_info) |= (1 << _OFFSET))
#define FREEFLAG_CLR(arr) ((arr.alloc_info) &= ~(1 << _OFFSET))

#define ALLOCSIZE_GET(arr) ((arr.alloc_info) & ~(1 << _OFFSET))
#define ALLOCSIZE_SET(arr, size) \
  (arr.alloc_info) = ((size) | ((arr.alloc_info) & (1 << _OFFSET)))

#if USE_SINGLE_HEAP_MEMORY
/* define single_heap array somewhere in your code, like on the example below:
              uint8_t single_heap[SINGLE_HEAP_SIZE] = {0};
*/
dl_heap_t default_heap;
#if !HEAP_LOCATION
uint8_t single_heap[SINGLE_HEAP_SIZE] = {0};
#else
#define HEAP_SET_ADDR(addr) __attribute__((section(".ARM.__at_" #addr)))
#define _HEAP_SET_ADDR(addr) HEAP_SET_ADDR(addr)
uint8_t single_heap[SINGLE_HEAP_SIZE] _HEAP_SET_ADDR(HEAP_LOCATION) = {0};
#endif
bool memory_init_flag = false;
#endif

void heap_init(dl_heap_t *heap_struct_ptr, void *mem_ptr,
               uint32_t mem_size) {  // Init here mem structures
  heap_struct_ptr->offset = 0;
  heap_struct_ptr->mem = (uint8_t *)mem_ptr;
  heap_struct_ptr->total_size = mem_size;
  heap_struct_ptr->alloc_info.allocations_num = 0;
  heap_struct_ptr->alloc_info.max_memory_amount = 0;
  for (uint32_t i = 0; i < MAX_NUM_OF_ALLOCATIONS; i++) {
    heap_struct_ptr->alloc_info.ptr_info_arr[i].ptr = NULL;
    heap_struct_ptr->alloc_info.ptr_info_arr[i].alloc_info = 0;
    FREEFLAG_SET(heap_struct_ptr->alloc_info.ptr_info_arr[i]);
  }
  for (uint32_t i = 0; i < heap_struct_ptr->total_size; i++) {
    heap_struct_ptr->mem[i] = 0;
  }
}

void dalloc(dl_heap_t *heap_struct_ptr, uint32_t size, void **ptr) {
#if USE_SINGLE_HEAP_MEMORY
  if (memory_init_flag == false) {
    heap_init(&default_heap, single_heap, SINGLE_HEAP_SIZE);
    memory_init_flag = true;
  }
#endif

  if (!heap_struct_ptr || !size) {
    *ptr = NULL;
  }

  uint32_t new_offset = heap_struct_ptr->offset + size;

  /* Correct offset if use alignment */
#if USE_ALIGNMENT
  while (new_offset % ALLOCATION_ALIGNMENT_BYTES != 0) {
    new_offset += 1;
  }
#endif

  /* Check if there is enough memory for new allocation, and if number of
   * allocations is exceeded */
  if ((new_offset <= heap_struct_ptr->total_size) &&
      (heap_struct_ptr->alloc_info.allocations_num < MAX_NUM_OF_ALLOCATIONS)) {
    *ptr = heap_struct_ptr->mem + heap_struct_ptr->offset;
    heap_struct_ptr->offset = new_offset;

    /* Save info about allocated memory */
    heap_struct_ptr->alloc_info
        .ptr_info_arr[heap_struct_ptr->alloc_info.allocations_num]
        .ptr = (uint8_t **)ptr;
    ALLOCSIZE_SET(
        heap_struct_ptr->alloc_info
            .ptr_info_arr[heap_struct_ptr->alloc_info.allocations_num],
        size);
    FREEFLAG_CLR(
        heap_struct_ptr->alloc_info
            .ptr_info_arr[heap_struct_ptr->alloc_info.allocations_num]);
    heap_struct_ptr->alloc_info.allocations_num =
        heap_struct_ptr->alloc_info.allocations_num + 1;

    if (heap_struct_ptr->offset >
        heap_struct_ptr->alloc_info.max_memory_amount) {
      heap_struct_ptr->alloc_info.max_memory_amount = heap_struct_ptr->offset;
    }
    if (heap_struct_ptr->alloc_info.allocations_num >
        heap_struct_ptr->alloc_info.max_allocations_amount) {
      heap_struct_ptr->alloc_info.max_allocations_amount =
          heap_struct_ptr->alloc_info.allocations_num;
    }
  } else {
    LOG_ERROR("dalloc: Allocation failed");
    print_dalloc_info(heap_struct_ptr);
    *ptr = NULL;
    if (new_offset > heap_struct_ptr->total_size) {
      LOG_ERROR("dalloc: Heap size exceeded");
    }
    if (heap_struct_ptr->alloc_info.allocations_num > MAX_NUM_OF_ALLOCATIONS) {
      LOG_ERROR("dalloc: Max number of allocations exceeded: %lu",
            (long unsigned int)heap_struct_ptr->alloc_info.allocations_num);
    }
  }
}

bool validate_ptr(dl_heap_t *heap_struct_ptr, void **ptr,
                  validate_ptr_condition_t condition, uint32_t *ptr_index) {
  for (uint32_t i = 0; i < heap_struct_ptr->alloc_info.allocations_num; i++) {
    if (condition == USING_PTR_ADDRESS) {
      if (heap_struct_ptr->alloc_info.ptr_info_arr[i].ptr == (uint8_t **)ptr) {
        if (ptr_index != NULL) {
          *ptr_index = i;
        }
        return true;
      }
    } else {
      if (*(heap_struct_ptr->alloc_info.ptr_info_arr[i].ptr) == *ptr) {
        if (ptr_index != NULL) {
          *ptr_index = i;
        }
        return true;
      }
    }
  }
  return false;
}

bool is_ptr_address_in_heap_area(dl_heap_t *heap_struct_ptr, void **ptr) {
  size_t heap_start_area = (size_t)(heap_struct_ptr->mem);
  size_t heap_stop_area =
      (size_t)(heap_struct_ptr->mem) + heap_struct_ptr->total_size;
  if (((size_t)ptr >= heap_start_area) && ((size_t)ptr <= heap_stop_area)) {
    return true;
  }
  return false;
}

void defrag_memory(dl_heap_t *heap_struct_ptr) {
  for (uint32_t i = 0; i < heap_struct_ptr->alloc_info.allocations_num; i++) {
    if (FREEFLAG_GET(heap_struct_ptr->alloc_info.ptr_info_arr[i])) {
      /* Optimize memory */
      uint8_t *start_mem_ptr =
          *(heap_struct_ptr->alloc_info.ptr_info_arr[i].ptr);
      uint32_t start_ind = (uint32_t)(start_mem_ptr - heap_struct_ptr->mem);

      /* Set given ptr to NULL */
      *(heap_struct_ptr->alloc_info.ptr_info_arr[i].ptr) = NULL;

      uint32_t alloc_size =
          ALLOCSIZE_GET(heap_struct_ptr->alloc_info.ptr_info_arr[i]);
#if USE_ALIGNMENT
      while (alloc_size % ALLOCATION_ALIGNMENT_BYTES != 0) {
        alloc_size += 1;
      }
#endif
      /* Check if ptrs adresses of defragmentated memory are in heap region */
      for (uint32_t k = i + 1; k < heap_struct_ptr->alloc_info.allocations_num;
           k++) {
        if (is_ptr_address_in_heap_area(
                heap_struct_ptr,
                (void **)heap_struct_ptr->alloc_info.ptr_info_arr[k].ptr)) {
          if ((size_t)heap_struct_ptr->alloc_info.ptr_info_arr[k].ptr >
              (size_t)(start_mem_ptr)) {
            heap_struct_ptr->alloc_info.ptr_info_arr[k].ptr =
                (uint8_t **)((size_t)(heap_struct_ptr->alloc_info
                                          .ptr_info_arr[k]
                                          .ptr) -
                             alloc_size);
          }
        }
      }

      /* Defragmentate memory */
      uint32_t stop_ind = heap_struct_ptr->offset - alloc_size;
      for (uint32_t k = start_ind; k <= stop_ind; k++) {
        *(heap_struct_ptr->mem + k) = *(heap_struct_ptr->mem + k + alloc_size);
      }

      /* Reassign pointers */
      for (uint32_t k = i + 1; k < heap_struct_ptr->alloc_info.allocations_num;
           k++) {
        *(heap_struct_ptr->alloc_info.ptr_info_arr[k].ptr) -= alloc_size;
      }

      /* Actualize ptr info array */
      for (uint32_t k = i; k < heap_struct_ptr->alloc_info.allocations_num - 1;
           k++) {
        heap_struct_ptr->alloc_info.ptr_info_arr[k] =
            heap_struct_ptr->alloc_info.ptr_info_arr[k + 1];
      }

      /* Decrement allocations number */
      heap_struct_ptr->alloc_info.allocations_num--;

      /* Refresh offset */
      heap_struct_ptr->offset = heap_struct_ptr->offset - alloc_size;

      /* Fill by 0 all freed memory */
#if FILL_FREED_MEMORY_BY_NULLS
      for (uint32_t k = 0; k < alloc_size; k++) {
        heap_struct_ptr->mem[heap_struct_ptr->offset + k] = 0;
      }
#endif
    }
  }
}

void dfree(dl_heap_t *heap_struct_ptr, void **ptr,
           validate_ptr_condition_t condition) {
  /* Check if heap_ptr is not assigned */
  if (heap_struct_ptr == NULL) {
    LOG_ERROR("Heap pointer is not assigned");
    return;
  }

  uint32_t ptr_index = 0;

  /* Try to find given ptr in ptr_info array */
  if (validate_ptr(heap_struct_ptr, ptr, condition, &ptr_index) != true) {
    LOG_ERROR("Try to free unexisting pointer");
    return;
  }

  uint32_t alloc_size =
      ALLOCSIZE_GET(heap_struct_ptr->alloc_info.ptr_info_arr[ptr_index]);
#if USE_ALIGNMENT
  while (alloc_size % ALLOCATION_ALIGNMENT_BYTES != 0) {
    alloc_size += 1;
  }
#endif

  /* Edit ptr info array */
  FREEFLAG_SET(heap_struct_ptr->alloc_info.ptr_info_arr[ptr_index]);
#if FILL_FREED_MEMORY_BY_NULLS
  for (uint32_t i = 0; i < alloc_size; i++) {
    *(*(heap_struct_ptr->alloc_info.ptr_info_arr[ptr_index].ptr) + i) = 0;
  }
#endif
  defrag_memory(heap_struct_ptr);
}

void replace_pointers(dl_heap_t *heap_struct_ptr, void **ptr_to_replace,
                      void **ptr_new) {
  uint32_t ptr_ind = 0;
  if (validate_ptr(heap_struct_ptr, ptr_to_replace, USING_PTR_ADDRESS,
                   &ptr_ind) != true) {
    LOG_ERROR("Can't replace pointers. No pointer found in buffer");
    return;
  }
  *ptr_new = *ptr_to_replace;
  heap_struct_ptr->alloc_info.ptr_info_arr[ptr_ind].ptr = (uint8_t **)ptr_new;
  *ptr_to_replace = NULL;
}

bool drealloc(dl_heap_t *heap_struct_ptr, uint32_t size, void **ptr) {
  uint32_t size_of_old_block = 0;
  uint32_t old_ptr_ind = 0;
  if (validate_ptr(heap_struct_ptr, ptr, USING_PTR_ADDRESS, &old_ptr_ind) ==
      true) {
    size_of_old_block =
        ALLOCSIZE_GET(heap_struct_ptr->alloc_info.ptr_info_arr[old_ptr_ind]);
  } else {
    return false;
  }

  uint8_t *new_ptr = NULL;
  dalloc(heap_struct_ptr, size, (void **)&new_ptr);
  if (new_ptr == NULL) {
    LOG_ERROR("drealloc failed due to dalloc failed");
    return false;
  }

  uint8_t *old_ptr = (uint8_t *)(*ptr);

  for (uint32_t i = 0; i < size_of_old_block; i++) {
    new_ptr[i] = old_ptr[i];
  }
  dfree(heap_struct_ptr, ptr, USING_PTR_ADDRESS);
  replace_pointers(heap_struct_ptr, (void **)&new_ptr, ptr);
  return true;
}

void print_dalloc_info(dl_heap_t *heap_struct_ptr) {
  LOG_RAWLN("************ Mem Info ************LOG_RAWLN$1");
  LOG_RAWLN("Total memory, bytes: %luLOG_RAWLN$1",
            (long unsigned int)heap_struct_ptr->total_size);
  LOG_RAWLN("Memory in use, bytes: %luLOG_RAWLN$1",
            (long unsigned int)heap_struct_ptr->offset);
  LOG_RAWLN("Number of allocations: %luLOG_RAWLN$1",
            (long unsigned int)heap_struct_ptr->alloc_info.allocations_num);
  LOG_RAWLN("The biggest memory was in use: %luLOG_RAWLN$1",
            (long unsigned int)heap_struct_ptr->alloc_info.max_memory_amount);
  LOG_RAWLN(
      "Max allocations number: %luLOG_RAWLN$1",
      (long unsigned int)heap_struct_ptr->alloc_info.max_allocations_amount);
  LOG_RAWLN("**********************************LOG_RAWLN$1");
}

void dump_heap(dl_heap_t *heap_struct_ptr) {
  LOG_RAWLN("************ Dump Heap ***********LOG_RAWLN$1");
  for (uint32_t i = 0; i < heap_struct_ptr->total_size; i++) {
    LOG_RAW("%02X ", heap_struct_ptr->mem[i]);
  }
  LOG_RAWLN("**********************************LOG_RAWLN$1");
}

void dump_dalloc_ptr_info(dl_heap_t *heap_struct_ptr) {
  LOG_RAWLN("************ Ptr Info ************LOG_RAWLN$1");
  for (uint32_t i = 0; i < heap_struct_ptr->alloc_info.allocations_num; i++) {
    LOG_RAWLN(
        "Ptr address: 0x%08X, ptr first val: 0x%02X, alloc size: "
        "%luLOG_RAWLN$1",
        (size_t)(heap_struct_ptr->alloc_info.ptr_info_arr[i].ptr),
        (uint8_t)(**heap_struct_ptr->alloc_info.ptr_info_arr[i].ptr),
        (long unsigned int)ALLOCSIZE_GET(
            heap_struct_ptr->alloc_info.ptr_info_arr[i]));
  }
  LOG_RAWLN("**********************************LOG_RAWLN$1");
}

float get_heap_usage(dl_heap_t *heap_struct_ptr) {
  uint32_t alloc_size = 0;
  for (uint32_t i = 0; i < heap_struct_ptr->alloc_info.allocations_num; i++) {
    alloc_size += ALLOCSIZE_GET(heap_struct_ptr->alloc_info.ptr_info_arr[i]);
  }
  return (float)alloc_size / (float)heap_struct_ptr->total_size;
}

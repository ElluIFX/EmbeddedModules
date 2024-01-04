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

#ifndef DALLOC_H
#define DALLOC_H
#include "modules.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DALLOC_VERSION "1.3.0"

#define FILL_FREED_MEMORY_BY_NULLS 1
#ifdef _MOD_HEAP_MAX_ALLOC
#define MAX_NUM_OF_ALLOCATIONS _MOD_HEAP_MAX_ALLOC
#else
#define MAX_NUM_OF_ALLOCATIONS 32UL
#endif

#define USE_ALIGNMENT 1

#if USE_ALIGNMENT
#define ALLOCATION_ALIGNMENT_BYTES 4U
#endif

#ifdef _MOD_HEAP_ADDR
#define HEAP_LOCATION _MOD_HEAP_ADDR
#else
#define HEAP_LOCATION 0
#endif

#if _MOD_USE_DALLOC
#define USE_SINGLE_HEAP_MEMORY 1
#else
#define USE_SINGLE_HEAP_MEMORY 0
#endif
#define ALLOC_INFO_U16 1

#if USE_SINGLE_HEAP_MEMORY
#ifdef _MOD_HEAP_SIZE
#define SINGLE_HEAP_SIZE _MOD_HEAP_SIZE
#else
#define SINGLE_HEAP_SIZE (1UL * 1024UL)
#endif
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum { DALLOC_OK = 0, DALLOC_ERR } dalloc_stat_t;

typedef enum {
  USING_PTR_ADDRESS = 0,
  USING_PTR_VALUE
} validate_ptr_condition_t;

#pragma pack(2)

typedef struct {
  uint8_t **ptr;
#if ALLOC_INFO_U16
  uint16_t alloc_info;
#else
  uint32_t alloc_info;
#endif
} ptr_info_t;

typedef struct {
  ptr_info_t ptr_info_arr[MAX_NUM_OF_ALLOCATIONS];
  uint32_t allocations_num;
  uint32_t max_memory_amount;
  uint32_t max_allocations_amount;
} alloc_info_t;

typedef struct {
  uint8_t *mem;        /* Pointer to the memory area using for heap */
  uint32_t offset;     /* Size of currently allocated memory */
  uint32_t total_size; /* Total size of memory that can be used for allocation
                        memory */
  alloc_info_t alloc_info;
} dl_heap_t;

#pragma pack()

#if USE_SINGLE_HEAP_MEMORY
extern dl_heap_t default_heap;
extern uint8_t single_heap[];

/**
 * @brief Allocate memory from default heap
 * @param  size - size of memory to allocate
 * @param  ptr - pointer to pointer to allocated memory
 */
#define def_dalloc(size, ptr) dalloc(&default_heap, size, ptr)

/**
 * @brief Free memory from default heap
 * @param  ptr - pointer to pointer to allocated memory
 */
#define def_dfree(ptr) dfree(&default_heap, ptr, USING_PTR_ADDRESS)

/**
 * @brief Free memory from default heap
 * @param  ptr - pointer to allocated memory
 */
#define def_dfree_value(ptr) dfree(&default_heap, ptr, USING_PTR_VALUE)

/**
 * @brief Replace a pointer in the default heap with a new pointer
 * @param  ptr_to_replace - pointer to pointer to be replaced
 * @param  ptr_new - pointer to new memory block
 */
#define def_replace_pointers(ptr_to_replace, ptr_new) \
  replace_pointers(&default_heap, ptr_to_replace, ptr_new)

/**
 * @brief Reallocate memory from default heap
 * @param  size - size of memory to allocate
 * @param  ptr - pointer to pointer to allocated memory
 * @return true if reallocation was successful, false otherwise
 */
#define def_drealloc(size, ptr) drealloc(&default_heap, size, ptr)

/**
 * @brief Print information about the default heap
 */
#define print_def_dalloc_info() print_dalloc_info(&default_heap)

/**
 * @brief Dump information about the default heap
 */
#define dump_def_heap() dump_heap(&default_heap)

/**
 * @brief Dump information about pointers in the default heap
 */
#define dump_def_dalloc_ptr_info() dump_dalloc_ptr_info(&default_heap)

/**
 * @brief Get the usage percentage of the default heap
 * @return the usage percentage of the default heap
 */
#define get_def_heap_usage() get_heap_usage(&default_heap)

#define _dalloc(ptr, size) def_dalloc(size, (void **)&(ptr))
// #define _dfree(ptr) def_dfree((void **)&(ptr))
#define _dfree(ptr) def_dfree_value((void *)(ptr))
#define _drealloc(ptr, size) def_drealloc(size, (void **)&(ptr))
#define _dreplace(ptr_to_replace, ptr_new) \
  def_replace_pointers((void **)&(ptr_to_replace), (void **)&(ptr_new))
#endif

/**
 * @brief Initialize a heap structure with a memory block
 * @param  heap_struct_ptr - pointer to heap structure
 * @param  mem_ptr - pointer to memory block
 * @param  mem_size - size of memory block
 */
void heap_init(dl_heap_t *heap_struct_ptr, void *mem_ptr, uint32_t mem_size);

/**
 * @brief Allocate memory from a heap
 * @param  heap_struct_ptr - pointer to heap structure
 * @param  size - size of memory to allocate
 * @param  ptr - pointer to pointer to allocated memory
 */
void dalloc(dl_heap_t *heap_struct_ptr, uint32_t size, void **ptr);

/**
 * @brief Reallocate memory from a heap
 * @param  heap_struct_ptr - pointer to heap structure
 * @param  size - size of memory to allocate
 * @param  ptr - pointer to pointer to allocated memory
 * @return true if reallocation was successful, false otherwise
 */
bool drealloc(dl_heap_t *heap_struct_ptr, uint32_t size, void **ptr);

/**
 * @brief Validate a pointer in a heap
 * @param  heap_struct_ptr - pointer to heap structure
 * @param  ptr - pointer to pointer to allocated memory
 * @param  condition - condition for validation
 * @param  ptr_index - pointer to index of pointer in heap
 * @return true if pointer is valid, false otherwise
 */
bool validate_ptr(dl_heap_t *heap_struct_ptr, void **ptr,
                  validate_ptr_condition_t condition, uint32_t *ptr_index);

/**
 * @brief Free memory from a heap
 * @param  heap_struct_ptr - pointer to heap structure
 * @param  ptr - pointer to pointer to allocated memory
 * @param  condition - condition for validation
 */
void dfree(dl_heap_t *heap_struct_ptr, void **ptr,
           validate_ptr_condition_t condition);

/**
 * @brief Print information about a heap
 * @param  heap_struct_ptr - pointer to heap structure
 */
void print_dalloc_info(dl_heap_t *heap_struct_ptr);

/**
 * @brief Dump information about pointers in a heap
 * @param  heap_struct_ptr - pointer to heap structure
 */
void dump_dalloc_ptr_info(dl_heap_t *heap_struct_ptr);

/**
 * @brief Dump information about a heap
 * @param  heap_struct_ptr - pointer to heap structure
 */
void dump_heap(dl_heap_t *heap_struct_ptr);

/**
 * @brief Get the usage percentage of a heap
 * @param  heap_struct_ptr - pointer to heap structure
 * @return the usage percentage of the heap
 */
float get_heap_usage(dl_heap_t *heap_struct_ptr);

/**
 * @brief Replace a pointer in a heap with a new pointer
 * @param  heap_struct_ptr - pointer to heap structure
 * @param  ptr_to_replace - pointer to pointer to be replaced
 * @param  ptr_new - pointer to new memory block
 */
void replace_pointers(dl_heap_t *heap_struct_ptr, void **ptr_to_replace,
                      void **ptr_new);

#ifdef __cplusplus
}
#endif

#endif  // DALLOC_H

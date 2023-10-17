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

#define FILL_FREED_MEMORY_BY_NULLS 0
#ifdef _MOD_HEAP_ALLOCATIONS
#define MAX_NUM_OF_ALLOCATIONS _MOD_HEAP_ALLOCATIONS
#else
#define MAX_NUM_OF_ALLOCATIONS 32UL
#endif

#define USE_ALIGNMENT 1

#if USE_ALIGNMENT
#define ALLOCATION_ALIGNMENT_BYTES 4U
#endif

#ifdef _MOD_HEAP_LOCATION
#define HEAP_LOCATION _MOD_HEAP_LOCATION
#else
#define HEAP_LOCATION 0
#endif

#define USE_SINGLE_HEAP_MEMORY 1
#define ALLOC_INFO_U16 1

#if USE_SINGLE_HEAP_MEMORY
#include "modules.h"
#ifdef _MOD_HEAP_SIZE
#define SINGLE_HEAP_SIZE _MOD_HEAP_SIZE
#else
#define SINGLE_HEAP_SIZE (1UL * 1024UL)
#endif
#endif

#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"

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
} heap_t;

#pragma pack()
#define heap_t_size sizeof(heap_t)

#if USE_SINGLE_HEAP_MEMORY
extern heap_t default_heap;
extern uint8_t single_heap[];

void def_dalloc(uint32_t size, void **ptr);
void def_dfree(void **ptr);
void def_replace_pointers(void **ptr_to_replace, void **ptr_new);
bool def_drealloc(uint32_t size, void **ptr);
void print_def_dalloc_info();
void dump_def_heap();
void dump_def_dalloc_ptr_info();

#define _dalloc(ptr, size) def_dalloc(size, (void **)&(ptr))
#define _dfree(ptr) def_dfree((void **)&(ptr))
#define _drealloc(ptr, size) def_drealloc(size, (void **)&(ptr))
#define _dreplace(ptr_to_replace, ptr_new) \
  def_replace_pointers((void **)&(ptr_to_replace), (void **)&(ptr_new))
#endif

void heap_init(heap_t *heap_struct_ptr, void *mem_ptr, uint32_t mem_size);
void dalloc(heap_t *heap_struct_ptr, uint32_t size, void **ptr);
bool drealloc(heap_t *heap_struct_ptr, uint32_t size, void **ptr);
bool validate_ptr(heap_t *heap_struct_ptr, void **ptr,
                  validate_ptr_condition_t condition, uint32_t *ptr_index);
void dfree(heap_t *heap_struct_ptr, void **ptr,
           validate_ptr_condition_t condition);
void print_dalloc_info(heap_t *heap_struct_ptr);
void dump_dalloc_ptr_info(heap_t *heap_struct_ptr);
void dump_heap(heap_t *heap_struct_ptr);
void replace_pointers(heap_t *heap_struct_ptr, void **ptr_to_replace,
                      void **ptr_new);

#ifdef __cplusplus
}
#endif

#endif  // DALLOC_H

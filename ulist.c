/**
 * @file ulist.c
 * @brief 通用的动态列表实现，Python风格的负索引支持，内存连续
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-01-05
 *
 * THINK DIFFERENTLY
 */

#include "ulist.h"

#include <string.h>
#define ulist_memmove memmove
#define ulist_memset memset
#define ulist_malloc m_alloc
#define ulist_free m_free
#define ulist_realloc m_realloc

#define ULIST_CLEAR_DIRTY_REGION 1
#define ULIST_DIRTY_REGION_FILL 0x00

void ulist_init(ulist_t* list, size_t isize) {
  list->data = NULL;
  list->cap = 0;
  list->inum = 0;
  list->isize = isize;
}

static inline ulist_size_t calc_min_req_size(ulist_size_t num) {
  ulist_size_t min_req_size = 2;
  while (min_req_size < num) min_req_size *= 2;
  return min_req_size;
}

static inline ulist_offset_t convert_pylike_offset(ulist_t* list,
                                                   ulist_offset_t index) {
  if (index < 0) {
    index += list->inum;
    if (index < 0) return -1;
  }
  if (index >= list->inum) return -1;
  return index;
}

static bool ulist_expend(ulist_t* list, ulist_size_t req_num) {
  ulist_size_t min_req_size;
  if (list->data == NULL || list->cap == 0) {  // list is empty
    min_req_size = calc_min_req_size(req_num);
    list->data = ulist_malloc(min_req_size * list->isize);
    if (list->data == NULL) return false;
#if ULIST_CLEAR_DIRTY_REGION
    ulist_memset(list->data, ULIST_DIRTY_REGION_FILL, min_req_size);
#endif
    list->cap = min_req_size;
  } else if (req_num > list->cap) {  // list is full
    min_req_size = calc_min_req_size(req_num);
    void* new_data = ulist_realloc(list->data, min_req_size * list->isize);
    if (new_data == NULL) {
      return false;
    }
    list->data = new_data;
#if ULIST_CLEAR_DIRTY_REGION
    ulist_memset((uint8_t*)new_data + list->cap, ULIST_DIRTY_REGION_FILL,
                 min_req_size - list->cap);
#endif
    list->data = new_data;
    list->cap = min_req_size;
  }
  return true;
}

static void ulist_shrink(ulist_t* list, ulist_size_t req_num) {
  if (req_num == 0) {  // free all
    list->inum = 0;
    ulist_free(list->data);
    list->data = NULL;
    list->cap = 0;
    return;
  }
  ulist_size_t min_req_size = calc_min_req_size(req_num);
  if (min_req_size < list->cap) {  // shrink
    void* new_data = ulist_realloc(list->data, min_req_size * list->isize);
    if (new_data != NULL) {
      list->data = new_data;
      list->cap = min_req_size;
    }
  }
}

const void* ulist_append(ulist_t* list, ulist_size_t num) {
  if (!ulist_expend(list, list->inum + num)) return NULL;
  uint8_t* ptr = (uint8_t*)list->data + list->inum * list->isize;
  list->inum += num;
  return (const void*)ptr;
}

const void* ulist_insert(ulist_t* list, ulist_offset_t index,
                         ulist_size_t num) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return NULL;
  if (!ulist_expend(list, list->inum + num)) return NULL;
  uint8_t* src = (uint8_t*)list->data + i * list->isize;
  ulist_size_t move_size = (list->inum - i) * list->isize;
  uint8_t* dst = (uint8_t*)list->data + (i + num) * list->isize;
  ulist_memmove(dst, src, move_size);
#if ULIST_CLEAR_DIRTY_REGION
  ulist_memset(src, ULIST_DIRTY_REGION_FILL, num * list->isize);
#endif
  list->inum += num;
  return (const void*)src;
}

int ulist_delete(ulist_t* list, ulist_offset_t index, ulist_size_t num) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return -1;
  if (list->inum == 0) return -2;
  if (i + num > list->inum) num = list->inum - i;
  if (i + num == list->inum) {  // directly delete from tail
    list->inum -= num;
  } else {  // delete from middle, need to move data
    uint8_t* src = (uint8_t*)list->data + (i + num) * list->isize;
    ulist_size_t move_size = (list->inum - (i + num)) * list->isize;
    uint8_t* dst = (uint8_t*)list->data + i * list->isize;
    ulist_memmove(dst, src, move_size);
    list->inum -= num;
  }
#if ULIST_CLEAR_DIRTY_REGION  // clear tail
  ulist_memset((uint8_t*)list->data + list->inum * list->isize,
               ULIST_DIRTY_REGION_FILL, num * list->isize);
#endif
  ulist_shrink(list, list->inum);
  return 0;
}

const void* ulist_get(ulist_t* list, ulist_offset_t index) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return NULL;
  return (const void*)((uint8_t*)list->data + i * list->isize);
}

void ulist_clear(ulist_t* list) {
  if (list->data != NULL) {
    ulist_free(list->data);
  }
  list->inum = 0;
  list->data = NULL;
  list->cap = 0;
}

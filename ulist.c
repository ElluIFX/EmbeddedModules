/**
 * @file ulist.c
 * @brief 数据类型通用的动态列表实现，Python风格的负索引支持，内存连续
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-01-05
 *
 * THINK DIFFERENTLY
 */

#include "ulist.h"

#include <stdlib.h>
#include <string.h>

#define _ulist_memmove memmove
#define _ulist_memcpy memcpy
#define _ulist_memset memset
#define _ulist_malloc m_alloc
#define _ulist_free m_free
#define _ulist_realloc m_realloc
#define _ulist_memcmp memcmp

/**
 * @brief 计算满足num个元素的最小容量
 * @note  当前实现为2的幂次, 降低内存操作次数
 */
static inline ulist_size_t calc_min_req_size(ulist_size_t num) {
  ulist_size_t min_req_size = 2;
  while (min_req_size < num) min_req_size *= 2;
  return min_req_size;
}

/**
 * @brief 将Python风格的负索引转换为C风格的正索引
 * @note  如果索引越界, 返回-1
 */
static inline ulist_offset_t convert_pylike_offset(ulist list,
                                                   ulist_offset_t index) {
  if (index == 0) return 0;
  if (index == SLICE_START) return 0;
  if (index == SLICE_END) return list->num;
  if (index < 0) {
    index += list->num;
    if (index < 0) {
      if (list->cfg & ULIST_CFG_IGNORE_SLICE_ERROR) return 0;
      return -1;
    }
  }
  if (index >= list->num) {
    if (list->cfg & ULIST_CFG_IGNORE_SLICE_ERROR) return list->num - 1;
    return -1;
  }
  return index;
}

/**
 * @brief 扩容列表, 使其容量至少为req_num
 * @note  如果扩容失败, 返回false
 */
static bool ulist_expend(ulist list, ulist_size_t req_num) {
  ulist_size_t min_req_size;
  if (list->data == NULL || list->cap == 0) {  // list is empty
    if (list->cfg & ULIST_CFG_NO_ALLOC_EXTEND) {
      min_req_size = req_num;
    } else {
      min_req_size = calc_min_req_size(req_num);
    }
    list->data = _ulist_malloc(min_req_size * list->isize);
    if (list->data == NULL) return false;
    if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
      _ulist_memset(list->data, ULIST_DIRTY_REGION_FILL_DATA,
                    min_req_size * list->isize);
    }
    list->cap = min_req_size;
  } else if (req_num > list->cap) {  // list is full
    if (list->cfg & ULIST_CFG_NO_ALLOC_EXTEND) {
      min_req_size = req_num;
    } else {
      min_req_size = calc_min_req_size(req_num);
    }
    void* new_data = _ulist_realloc(list->data, min_req_size * list->isize);
    if (new_data == NULL) {
      return false;
    }
    list->data = new_data;
    if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
      _ulist_memset((uint8_t*)new_data + list->cap * list->isize,
                    ULIST_DIRTY_REGION_FILL_DATA,
                    (min_req_size - list->cap) * list->isize);
    }
    list->data = new_data;
    list->cap = min_req_size;
  }
  return true;
}

/**
 * @brief 缩减列表, 使其容量至少为req_num
 * @note  如果req_num为0, 则释放列表
 */
static void ulist_shrink(ulist list, ulist_size_t req_num) {
  if (list->cfg & ULIST_CFG_NO_SHRINK) return;
  if (req_num == 0) {  // free all
    if (list->cfg & ULIST_CFG_NO_AUTO_FREE) return;
    list->num = 0;
    _ulist_free(list->data);
    list->data = NULL;
    list->cap = 0;
    return;
  }
  ulist_size_t min_req_size = calc_min_req_size(req_num);
  if (min_req_size < list->cap) {  // shrink
    void* new_data = _ulist_realloc(list->data, min_req_size * list->isize);
    if (new_data != NULL) {
      list->data = new_data;
      list->cap = min_req_size;
    }
  }
}

void ulist_init(ulist list, ulist_size_t isize, ulist_size_t initial_cap,
                uint8_t cfg) {
  list->data = NULL;
  list->cap = 0;
  list->num = 0;
  list->isize = isize;
  list->cfg = cfg;
  if (initial_cap > 0) {
    ulist_expend(list, initial_cap);
  }
}

ulist ulist_create(ulist_size_t isize, ulist_size_t initial_cap, uint8_t cfg) {
  ulist list = (ulist)_ulist_malloc(sizeof(ulist_t));
  if (list == NULL) return NULL;
  ulist_init(list, isize, initial_cap, cfg);
  return list;
}

void* ulist_append(ulist list, ulist_size_t num) {
  if (!ulist_expend(list, list->num + num)) return NULL;
  uint8_t* ptr = (uint8_t*)list->data + list->num * list->isize;
  list->num += num;
  return (void*)ptr;
}

bool ulist_append_buf(ulist list, ulist_size_t num, const void* buf) {
  if (!ulist_expend(list, list->num + num)) return false;
  uint8_t* ptr = (uint8_t*)list->data + list->num * list->isize;
  _ulist_memcpy(ptr, buf, num * list->isize);
  list->num += num;
  return true;
}

bool ulist_extend(ulist list, ulist other) {
  if (!ulist_expend(list, list->num + other->num)) return false;
  uint8_t* ptr = (uint8_t*)list->data + list->num * list->isize;
  _ulist_memcpy(ptr, other->data, other->num * list->isize);
  list->num += other->num;
  return true;
}

void* ulist_insert(ulist list, ulist_offset_t index, ulist_size_t num) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == list->num) return ulist_append(list, num);  // append
  if (i == -1) return NULL;
  if (!ulist_expend(list, list->num + num)) return NULL;
  uint8_t* src = (uint8_t*)list->data + i * list->isize;
  ulist_size_t move_size = (list->num - i) * list->isize;
  uint8_t* dst = (uint8_t*)list->data + (i + num) * list->isize;
  _ulist_memmove(dst, src, move_size);
  if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
    _ulist_memset(src, ULIST_DIRTY_REGION_FILL_DATA, num * list->isize);
  }
  list->num += num;
  return (void*)src;
}

bool ulist_insert_buf(ulist list, ulist_offset_t index, ulist_size_t num,
                      const void* buf) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == list->num) return ulist_append_buf(list, num, buf);  // append
  if (i == -1) return false;
  if (!ulist_expend(list, list->num + num)) return false;
  uint8_t* src = (uint8_t*)list->data + i * list->isize;
  ulist_size_t move_size = (list->num - i) * list->isize;
  uint8_t* dst = (uint8_t*)list->data + (i + num) * list->isize;
  _ulist_memmove(dst, src, move_size);
  _ulist_memcpy(src, buf, num * list->isize);
  list->num += num;
  return true;
}

bool ulist_delete(ulist list, ulist_offset_t index, ulist_size_t num) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return false;
  if (list->num == 0) return false;
  if (i + num > list->num) num = list->num - i;
  if (i + num == list->num) {  // directly delete from tail
    list->num -= num;
  } else {  // delete from middle, need to move data
    uint8_t* src = (uint8_t*)list->data + (i + num) * list->isize;
    ulist_size_t move_size = (list->num - (i + num)) * list->isize;
    uint8_t* dst = (uint8_t*)list->data + i * list->isize;
    _ulist_memmove(dst, src, move_size);
    list->num -= num;
  }
  if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
    _ulist_memset((uint8_t*)list->data + list->num * list->isize,
                  ULIST_DIRTY_REGION_FILL_DATA, num * list->isize);
  }
  ulist_shrink(list, list->num);
  return true;
}

bool ulist_remove(ulist list, const void* ptr) {
  ulist_offset_t i = ulist_index(list, ptr);
  if (i == -1) return false;
  return ulist_delete(list, i, 1);
}

void* ulist_copy(ulist list, ulist_offset_t index, ulist_size_t num) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return NULL;
  if (list->num == 0) return NULL;
  if (i + num > list->num) {
    if (list->cfg & ULIST_CFG_IGNORE_NUM_ERROR) {
      num = list->num - i;
    } else {
      return NULL;
    }
  }
  uint8_t* ptr = _ulist_malloc(num * list->isize);
  if (ptr == NULL) return NULL;
  uint8_t* src = (uint8_t*)list->data + i * list->isize;
  _ulist_memcpy(ptr, src, num * list->isize);
  return (void*)ptr;
}

bool ulist_copy_buf(ulist list, ulist_offset_t index, ulist_size_t num,
                    void* buf) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return false;
  if (list->num == 0) return false;
  if (i + num > list->num) {
    if (list->cfg & ULIST_CFG_IGNORE_NUM_ERROR) {
      num = list->num - i;
    } else {
      return false;
    }
  }
  uint8_t* src = (uint8_t*)list->data + i * list->isize;
  _ulist_memcpy(buf, src, num * list->isize);
  return true;
}

ulist ulist_copy_list(ulist list, ulist_offset_t index, ulist_size_t num) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return NULL;
  if (list->num == 0) return NULL;
  if (i + num > list->num) {
    if (list->cfg & ULIST_CFG_IGNORE_NUM_ERROR) {
      num = list->num - i;
    } else {
      return NULL;
    }
  }
  uint8_t* src = (uint8_t*)list->data + i * list->isize;
  ulist new_list = ulist_create(list->isize, num, list->cfg);
  if (new_list == NULL) return NULL;
  ulist_append_buf(new_list, num, src);
  return new_list;
}

void* ulist_pop(ulist list, ulist_offset_t index, ulist_size_t num) {
  void* ptr = ulist_copy(list, index, num);
  if (ptr == NULL) return NULL;
  ulist_delete(list, index, num);
  return ptr;
}

ulist ulist_pop_list(ulist list, ulist_offset_t index, ulist_size_t num) {
  ulist new_list = ulist_copy_list(list, index, num);
  if (new_list == NULL) return NULL;
  ulist_delete(list, index, num);
  return new_list;
}

bool ulist_pop_buf(ulist list, ulist_offset_t index, ulist_size_t num,
                   void* buf) {
  if (!ulist_copy_buf(list, index, num, buf)) return false;
  ulist_delete(list, index, num);
  return true;
}

bool ulist_swap(ulist list, ulist_offset_t index1, ulist_offset_t index2) {
  ulist_offset_t i1 = convert_pylike_offset(list, index1);
  ulist_offset_t i2 = convert_pylike_offset(list, index2);
  if (i1 == -1 || i2 == -1) return false;
  uint8_t* ptr1 = (uint8_t*)list->data + i1 * list->isize;
  uint8_t* ptr2 = (uint8_t*)list->data + i2 * list->isize;
  uint8_t tmp;
  for (ulist_size_t i = 0; i < list->isize; i++) {
    tmp = ptr1[i];
    ptr1[i] = ptr2[i];
    ptr2[i] = tmp;
  }
  return true;
}

void* ulist_get(ulist list, ulist_offset_t index) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return NULL;
  return (void*)((uint8_t*)list->data + i * list->isize);
}

bool ulist_sort(ulist list, int (*cmp)(const void*, const void*),
                ulist_offset_t start, ulist_offset_t end) {
  ulist_offset_t i = convert_pylike_offset(list, start);
  ulist_offset_t j = convert_pylike_offset(list, end);
  if (i == -1 || j == -1) return false;
  qsort((uint8_t*)list->data + i * list->isize, j - i, list->isize, cmp);
  return true;
}

ulist_offset_t ulist_index(ulist list, void* ptr) {
  ulist_offset_t i = (uint8_t*)ptr - (uint8_t*)list->data;
  if (i < 0) return false;
  if (i % list->isize != 0) return false;
  i /= list->isize;
  if (i >= list->num) return false;
  return i;
}

ulist_offset_t ulist_find(ulist list, void* ptr) {
  for (ulist_offset_t i = 0; i < list->num; i++) {
    if (_ulist_memcmp((uint8_t*)list->data + i * list->isize, ptr,
                      list->isize) == 0) {
      return i;
    }
  }
  return -1;
}

void ulist_free(ulist list) {
  if (list->data != NULL) {
    _ulist_free(list->data);
  }
  _ulist_free(list);
}

void ulist_clear(ulist list) {
  if (list->data != NULL) {
    _ulist_free(list->data);
  }
  list->num = 0;
  list->data = NULL;
  list->cap = 0;
}

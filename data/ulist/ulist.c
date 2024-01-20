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

#include "log.h"

#define _ulist_memmove memmove
#define _ulist_memcpy memcpy
#define _ulist_memset memset
#define _ulist_malloc m_alloc
#define _ulist_free m_free
#define _ulist_realloc m_realloc
#define _ulist_memcmp memcmp

#if ULIST_DISABLE_ALL_LOG
#define LIST_LOG(...) ((void)0)
#else
#define LIST_LOG(...)                          \
  if (!(list->cfg & ULIST_CFG_NO_ERROR_LOG)) { \
    LOG_E(__VA_ARGS__);                        \
  }
#endif

/**
 * @brief 将Python风格的负索引转换为C风格的正索引
 * @note  如果索引越界, 返回-1
 */
static inline ulist_offset_t convert_pylike_offset(ULIST list,
                                                   ulist_offset_t index) {
  if (index == 0) return 0;
  if (index == SLICE_START) return 0;
  if (index == SLICE_END) return list->num;
  if (index < 0) {
    index += list->num;
    if (index < 0) {
      if (list->cfg & ULIST_CFG_IGNORE_SLICE_ERROR) {
        return 0;
      }
      LIST_LOG("ulist: index out of range: %d", index);
      return -1;
    }
  }
  if (index >= list->num) {
    if (list->cfg & ULIST_CFG_IGNORE_SLICE_ERROR) {
      return list->num - 1;
    }
    LIST_LOG("ulist: index out of range: %d", index);
    return -1;
  }
  return index;
}

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
 * @brief 扩容列表, 使其容量至少为req_num
 * @note  如果扩容失败, 返回false
 */
static bool ulist_expend(ULIST list, ulist_size_t req_num) {
  if (list->data == NULL || list->cap == 0) {  // list is empty
    if (!(list->cfg & ULIST_CFG_NO_ALLOC_EXTEND)) {
      req_num = calc_min_req_size(req_num);
    }
    list->data = _ulist_malloc(req_num * list->isize);
    if (list->data == NULL) {
      LIST_LOG("ulist: malloc failed");
      return false;
    }
    if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
      _ulist_memset(list->data, ULIST_DIRTY_REGION_FILL_DATA,
                    req_num * list->isize);
    }
    list->cap = req_num;
  } else if (req_num > list->cap) {  // list is full
    if (!(list->cfg & ULIST_CFG_NO_ALLOC_EXTEND)) {
      req_num = calc_min_req_size(req_num);
    }
    void* new_data = _ulist_realloc(list->data, req_num * list->isize);
    if (new_data == NULL) {
      LIST_LOG("ulist: malloc failed");
      return false;
    }
    list->data = new_data;
    if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
      _ulist_memset((uint8_t*)new_data + list->cap * list->isize,
                    ULIST_DIRTY_REGION_FILL_DATA,
                    (req_num - list->cap) * list->isize);
    }
    list->cap = req_num;
  }
  return true;
}

/**
 * @brief 缩减列表, 使其容量至少为req_num
 * @note  如果req_num为0, 则释放列表
 */
static void ulist_shrink(ULIST list, ulist_size_t req_num) {
  if (req_num == 0) {  // free all
    if (list->cfg & ULIST_CFG_NO_AUTO_FREE) {
      goto check_dirty_region;
    }
    _ulist_free(list->data);
    list->num = 0;
    list->data = NULL;
    list->cap = 0;
    return;
  }
  if (list->cfg & ULIST_CFG_NO_SHRINK) {
    goto check_dirty_region;
  }
  if (!(list->cfg & ULIST_CFG_NO_ALLOC_EXTEND)) {
    req_num = calc_min_req_size(req_num);
  }
  if (req_num < list->cap) {  // shrink
    void* new_data = _ulist_realloc(list->data, req_num * list->isize);
    if (new_data != NULL) {
      list->data = new_data;
      list->cap = req_num;
    }
  }
check_dirty_region:
  if ((list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) && list->cap > req_num) {
    _ulist_memset((uint8_t*)list->data + req_num * list->isize,
                  ULIST_DIRTY_REGION_FILL_DATA,
                  (list->cap - req_num) * list->isize);
  }
}

bool ulist_init(ULIST list, ulist_size_t isize, ulist_size_t init_size,
                uint8_t cfg, void (*elfree)(void* item)) {
  list->data = NULL;
  list->cap = 0;
  list->num = 0;
  list->isize = isize;
  list->cfg = cfg;
  list->elfree = elfree;
  list->iter = -1;
  if (init_size > 0) {
    if (!ulist_expend(list, init_size)) {
      list->data = NULL;
      list->cap = 0;
      return false;
    }
    list->num = init_size;
  }
  return true;
}

ULIST ulist_create(ulist_size_t isize, ulist_size_t init_size, uint8_t cfg,
                   void (*elfree)(void* item)) {
  ULIST list = (ULIST)_ulist_malloc(sizeof(ulist_t));
  if (list == NULL) {
    return NULL;
  }
  if (!ulist_init(list, isize, init_size, cfg, elfree)) {
    _ulist_free(list);
    return NULL;
  }
  return list;
}

void* ulist_append_multi(ULIST list, ulist_size_t num) {
  if (!ulist_expend(list, list->num + num)) return NULL;
  uint8_t* ptr = (uint8_t*)list->data + list->num * list->isize;
  list->num += num;
  return (void*)ptr;
}

void* ulist_append(ULIST list) { return ulist_append_multi(list, 1); }

bool ulist_append_from_ptr(ULIST list, const void* src) {
  if (!src) return false;
  void* ptr = ulist_append(list);
  if (ptr == NULL) return false;
  _ulist_memcpy(ptr, src, list->isize);
  return true;
}

void* ulist_insert_multi(ULIST list, ulist_offset_t index, ulist_size_t num) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == list->num) return ulist_append_multi(list, num);  // append
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
  list->iter = -1;
  return (void*)src;
}

void* ulist_insert(ULIST list, ulist_offset_t index) {
  return ulist_insert_multi(list, index, 1);
}

bool ulist_insert_from_ptr(ULIST list, ulist_offset_t index, const void* src) {
  if (!src) return false;
  void* ptr = ulist_insert(list, index);
  if (ptr == NULL) return false;
  _ulist_memcpy(ptr, src, list->isize);
  return true;
}

bool ulist_extend(ULIST list, ULIST other) {
  if (!ulist_expend(list, list->num + other->num)) return false;
  uint8_t* ptr = (uint8_t*)list->data + list->num * list->isize;
  _ulist_memcpy(ptr, other->data, other->num * list->isize);
  list->num += other->num;
  return true;
}

bool ulist_insert_list(ULIST list, ulist_offset_t index, ULIST other) {
  void* ptr = ulist_insert_multi(list, index, other->num);
  if (ptr == NULL) return false;
  _ulist_memcpy(ptr, other->data, other->num * list->isize);
  return true;
}

bool ulist_delete_multi(ULIST list, ulist_offset_t index, ulist_size_t num) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return false;
  if (list->num == 0) return false;
  if (i + num > list->num) num = list->num - i;
  if (list->elfree != NULL) {
    for (ulist_size_t j = 0; j < num; j++) {
      list->elfree((uint8_t*)(list->data + (i + j) * list->isize));
    }
  }
  if (i + num == list->num) {  // directly delete from tail
    list->num -= num;
  } else {  // delete from middle, need to move data
    uint8_t* src = (uint8_t*)list->data + (i + num) * list->isize;
    ulist_size_t move_size = (list->num - (i + num)) * list->isize;
    uint8_t* dst = (uint8_t*)list->data + i * list->isize;
    _ulist_memmove(dst, src, move_size);
    list->num -= num;
  }
  ulist_shrink(list, list->num);
  list->iter = -1;
  return true;
}

bool ulist_delete(ULIST list, ulist_offset_t index) {
  return ulist_delete_multi(list, index, 1);
}

bool ulist_delete_slice(ULIST list, ulist_offset_t start, ulist_offset_t end) {
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) return false;
  return ulist_delete_multi(list, start, end - start);
}

bool ulist_remove(ULIST list, const void* ptr) {
  ulist_offset_t i = ulist_index(list, ptr);
  if (i == -1) return false;
  return ulist_delete(list, i);
}

void* ulist_slice(ULIST list, ulist_offset_t start, ulist_offset_t end) {
  if (list->num == 0) return false;
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) return NULL;
  ulist_size_t num = end - start;
  uint8_t* ptr = _ulist_malloc(num * list->isize);
  if (ptr == NULL) return NULL;
  uint8_t* src = (uint8_t*)list->data + start * list->isize;
  _ulist_memcpy(ptr, src, num * list->isize);
  return (void*)ptr;
}

bool ulist_slice_to_buf(ULIST list, ulist_offset_t start, ulist_offset_t end,
                        void* buf) {
  if (list->num == 0) return false;
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) return NULL;
  ulist_size_t num = end - start;
  uint8_t* src = (uint8_t*)list->data + start * list->isize;
  _ulist_memcpy(buf, src, num * list->isize);
  return true;
}

ULIST ulist_slice_to_newlist(ULIST list, ulist_offset_t start,
                             ulist_offset_t end) {
  if (list->num == 0) return false;
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) return NULL;
  ulist_size_t num = end - start;
  uint8_t* src = (uint8_t*)list->data + start * list->isize;
  ULIST new_list = ulist_create(list->isize, num, list->cfg, list->elfree);
  if (new_list == NULL) return NULL;
  _ulist_memcpy(new_list->data, src, num * list->isize);
  return new_list;
}

void* ulist_pop(ULIST list, ulist_offset_t index) {
  void* ptr = ulist_slice(list, index, index + 1);
  if (ptr == NULL) return NULL;
  ulist_delete(list, index);
  return ptr;
}

bool ulist_update(ULIST list, ulist_offset_t index, const void* src) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return false;
  if (list->elfree != NULL) {
    list->elfree((uint8_t*)(list->data + i * list->isize));
  }
  _ulist_memcpy((uint8_t*)list->data + i * list->isize, src, list->isize);
  return true;
}

void* ulist_get(ULIST list, ulist_offset_t index) {
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) return NULL;
  return (void*)((uint8_t*)list->data + i * list->isize);
}

bool ulist_get_item(ULIST list, ulist_offset_t index, void* target) {
  void* ptr = ulist_get(list, index);
  if (ptr == NULL) return false;
  _ulist_memcpy(target, ptr, list->isize);
  return true;
}

bool ulist_swap(ULIST list, ulist_offset_t index1, ulist_offset_t index2) {
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

bool ulist_sort(ULIST list, int (*cmp)(const void*, const void*),
                ulist_offset_t start, ulist_offset_t end) {
  ulist_offset_t i = convert_pylike_offset(list, start);
  ulist_offset_t j = convert_pylike_offset(list, end);
  if (i == -1 || j == -1) return false;
  qsort((uint8_t*)list->data + i * list->isize, j - i, list->isize, cmp);
  return true;
}

ulist_offset_t ulist_index(ULIST list, const void* ptr) {
  ulist_offset_t i = (uint8_t*)ptr - (uint8_t*)list->data;
  if (i < 0) return false;
  if (i % list->isize != 0) return false;
  i /= list->isize;
  if (i >= list->num) return false;
  return i;
}

ulist_offset_t ulist_find(ULIST list, const void* ptr) {
  for (ulist_offset_t i = 0; i < list->num; i++) {
    if (_ulist_memcmp((uint8_t*)list->data + i * list->isize, ptr,
                      list->isize) == 0) {
      return i;
    }
  }
  return -1;
}

void ulist_free(ULIST list) {
  if (list->elfree != NULL && list->num > 0) {
    for (ulist_size_t i = 0; i < list->num; i++) {
      list->elfree((uint8_t*)(list->data + i * list->isize));
    }
  }
  if (list->data != NULL) {
    _ulist_free(list->data);
  }
  _ulist_free(list);
}

void ulist_manual_shrink(ULIST list) {
  uint8_t cfg = list->cfg;
  list->cfg = 0;
  ulist_shrink(list, list->num);
  list->cfg = cfg;
}

void ulist_clear(ULIST list) {
  if (list->elfree != NULL && list->num > 0) {
    for (ulist_size_t i = 0; i < list->num; i++) {
      list->elfree((uint8_t*)(list->data + i * list->isize));
    }
  }
  ulist_shrink(list, 0);
  list->num = 0;
  list->iter = -1;
}

bool ulist_iter(ULIST list, void** ptrptr, ulist_offset_t start,
                ulist_offset_t end, ulist_offset_t step) {
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) return false;
  if (step == 0) return false;
  if (step < 0) {
    if (start < end) return false;
  } else {
    if (start > end) return false;
  }
  if (list->iter < 0) {
    list->iter = start;
  } else {
    list->iter += step;
  }
  if (list->iter >= end || list->iter >= list->num) {
    list->iter = -1;
    return false;
  }
  *ptrptr = (void*)((uint8_t*)list->data + list->iter * list->isize);
  return true;
}

ULIST_ITER ulist_create_iter(ULIST list, ulist_offset_t start,
                             ulist_offset_t end, ulist_offset_t step) {
  ulist_size_t start_s = convert_pylike_offset(list, start);
  ulist_size_t end_s = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) return NULL;
  ULIST_ITER iter = (ULIST_ITER)_ulist_malloc(sizeof(ulist_iter_t));
  if (iter == NULL) return NULL;
  iter->target = list;
  iter->start = start_s;
  iter->end = end_s;
  iter->step = step;
  iter->now = start;
  iter->started = 0;
  return iter;
}

void ulist_iter_free(ULIST_ITER iter) { _ulist_free(iter); }

void* ulist_iter_next(ULIST_ITER iter) {
  if (iter->started == 0) {
    iter->now = iter->start;
    iter->started = 1;
  } else {
    iter->now += iter->step;
  }
  if (iter->now >= iter->end || iter->now >= iter->target->num) {
    iter->started = 0;
    return NULL;
  }
  return (void*)((uint8_t*)iter->target->data +
                 iter->now * iter->target->isize);
}

void* ulist_iter_now(ULIST_ITER iter) {
  if (!iter->started) return NULL;
  return (void*)((uint8_t*)iter->target->data +
                 iter->now * iter->target->isize);
}

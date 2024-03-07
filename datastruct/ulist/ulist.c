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
    LOG_ERROR(__VA_ARGS__);                        \
  }
#endif

#if !MOD_CFG_USE_OS_NONE
#define ULIST_LOCK()                                      \
  {                                                       \
    if (!(list->cfg & ULIST_CFG_NO_MUTEX)) {              \
      if (!list->mutex) list->mutex = MOD_MUTEX_CREATE(); \
      MOD_MUTEX_ACQUIRE(list->mutex);                     \
    }                                                     \
  }
#define ULIST_UNLOCK()                       \
  {                                          \
    if (!(list->cfg & ULIST_CFG_NO_MUTEX)) { \
      MOD_MUTEX_RELEASE(list->mutex);        \
    }                                        \
  }
#else
#define ULIST_LOCK() ((void)0)
#define ULIST_UNLOCK() ((void)0)
#endif
#define ULIST_UNLOCK_RET(x) \
  {                         \
    ULIST_UNLOCK();         \
    return x;               \
  }

#define ULIST_BSIZE(num) ((num) * list->isize)
#define ULIST_PTR(offset) (((uint8_t*)list->data) + ULIST_BSIZE(offset))

/**
 * @brief 将Python风格的负索引转换为C风格的正索引
 * @note  如果索引越界, 返回-1
 * @note  对于SLICE_START，返回0
 * @note  对于SLICE_END，返回list->num
 * @note  上述规则SLICE_*假定你进行的是正向迭代(start<=i++<end)
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
 * @brief 计算满足num个元素的所需容量
 * @note  当前实现为2的幂次, 降低内存操作次数
 */
static ulist_size_t calc_req_size(ulist_size_t req_num) {
  if (req_num > ULIST_MAX_EXTEND_SIZE) return req_num + ULIST_MAX_EXTEND_SIZE;
  ulist_size_t min_req_size = 2;
  while (min_req_size < req_num) min_req_size *= 2;
  return min_req_size;
}

/**
 * @brief 扩容列表, 使其容量至少为req_num
 * @note  如果扩容失败, 返回false
 */
static bool ulist_expend(ULIST list, ulist_size_t req_num) {
  if (list->data == NULL || list->cap == 0) {  // list is empty
    if (!(list->cfg & ULIST_CFG_NO_ALLOC_EXTEND)) {
      req_num = calc_req_size(req_num);
    }
    list->data = _ulist_malloc(ULIST_BSIZE(req_num));
    if (list->data == NULL) {
      LIST_LOG("ulist: malloc failed");
      return false;
    }
    if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
      _ulist_memset(list->data, ULIST_DIRTY_REGION_FILL_DATA,
                    ULIST_BSIZE(req_num));
    }
    list->cap = req_num;
  } else if (req_num > list->cap) {  // list is full
    if (!(list->cfg & ULIST_CFG_NO_ALLOC_EXTEND)) {
      req_num = calc_req_size(req_num);
    }
    void* new_data = _ulist_realloc(list->data, ULIST_BSIZE(req_num));
    if (new_data == NULL) {
      LIST_LOG("ulist: malloc failed");
      return false;
    }
    list->data = new_data;
    if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
      _ulist_memset(ULIST_PTR(list->cap), ULIST_DIRTY_REGION_FILL_DATA,
                    ULIST_BSIZE(req_num - list->cap));
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
    req_num = calc_req_size(req_num);
  }
  if (req_num < list->cap) {  // shrink
    void* new_data = _ulist_realloc(list->data, ULIST_BSIZE(req_num));
    if (new_data != NULL) {
      list->data = new_data;
      list->cap = req_num;
    }
  }
check_dirty_region:
  if ((list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) && list->cap > req_num) {
    _ulist_memset(ULIST_PTR(req_num), ULIST_DIRTY_REGION_FILL_DATA,
                  ULIST_BSIZE(list->cap - req_num));
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
  list->dyn = false;
  if (init_size > 0) {
    if (!ulist_expend(list, init_size)) {
      list->data = NULL;
      list->cap = 0;
      return false;
    }
    list->num = init_size;
  }
#if !MOD_CFG_USE_OS_NONE
  if (!(list->cfg & ULIST_CFG_NO_MUTEX))
    list->mutex = MOD_MUTEX_CREATE();
  else
    list->mutex = NULL;
#endif
  return true;
}

ULIST ulist_new(ulist_size_t isize, ulist_size_t init_size, uint8_t cfg,
                void (*elfree)(void* item)) {
  ULIST list = (ULIST)_ulist_malloc(sizeof(ulist_t));
  if (list == NULL) {
    return NULL;
  }
  if (!ulist_init(list, isize, init_size, cfg, elfree)) {
    _ulist_free(list);
    return NULL;
  }
  list->dyn = true;
  return list;
}

void ulist_free(ULIST list) {
  if (list->elfree != NULL && list->num > 0) {
    for (ulist_size_t i = 0; i < list->num; i++) {
      list->elfree(ULIST_PTR(i));
    }
  }
  if (list->data != NULL) {
    _ulist_free(list->data);
  }
#if !MOD_CFG_USE_OS_NONE
  if (list->mutex) MOD_MUTEX_FREE(list->mutex);
#endif
  if (list->dyn) _ulist_free(list);
}

void* ulist_append_multi(ULIST list, ulist_size_t num) {
  if (num == 0) return NULL;

  ULIST_LOCK();
  if (!ulist_expend(list, list->num + num)) return NULL;
  uint8_t* ptr = ULIST_PTR(list->num);
  list->num += num;
  ULIST_UNLOCK_RET((void*)ptr);
}

void* ulist_append(ULIST list) { return ulist_append_multi(list, 1); }

bool ulist_append_copy(ULIST list, const void* src) {
  if (!src) return false;

  ULIST_LOCK();
  void* ptr = ulist_append(list);
  if (ptr == NULL) ULIST_UNLOCK_RET(false);
  _ulist_memcpy(ptr, src, list->isize);
  ULIST_UNLOCK_RET(true);
}

void* ulist_insert_multi(ULIST list, ulist_offset_t index, ulist_size_t num) {
  if (num == 0) return NULL;
  if (index == SLICE_END) return ulist_append_multi(list, num);  // append

  ULIST_LOCK();
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) ULIST_UNLOCK_RET(NULL);
  if (!ulist_expend(list, list->num + num)) ULIST_UNLOCK_RET(NULL);
  uint8_t* src = ULIST_PTR(i);
  ulist_size_t move_size = ULIST_BSIZE(list->num - i);
  uint8_t* dst = ULIST_PTR(i + num);
  _ulist_memmove(dst, src, move_size);
  if (list->cfg & ULIST_CFG_CLEAR_DIRTY_REGION) {
    _ulist_memset(src, ULIST_DIRTY_REGION_FILL_DATA, ULIST_BSIZE(num));
  }
  list->num += num;
  list->iter = -1;
  ULIST_UNLOCK_RET((void*)src);
}

void* ulist_insert(ULIST list, ulist_offset_t index) {
  return ulist_insert_multi(list, index, 1);
}

bool ulist_insert_copy(ULIST list, ulist_offset_t index, const void* src) {
  if (!src) return false;

  ULIST_LOCK();
  void* ptr = ulist_insert(list, index);
  if (ptr == NULL) ULIST_UNLOCK_RET(false);
  _ulist_memcpy(ptr, src, list->isize);
  ULIST_UNLOCK_RET(true);
}

bool ulist_extend(ULIST list, ULIST other) {
  if (!other) return false;
  if (other->isize != list->isize) return false;

  ULIST_LOCK();
  if (!ulist_expend(list, list->num + other->num)) ULIST_UNLOCK_RET(false);
  uint8_t* ptr = ULIST_PTR(list->num);
  _ulist_memcpy(ptr, other->data, ULIST_BSIZE(other->num));
  list->num += other->num;
  ULIST_UNLOCK_RET(true);
}

bool ulist_extend_at(ULIST list, ULIST other, ulist_offset_t index) {
  if (!other) return false;
  if (other->isize != list->isize) return false;

  ULIST_LOCK();
  void* ptr = ulist_insert_multi(list, index, other->num);
  if (ptr == NULL) ULIST_UNLOCK_RET(false);
  _ulist_memcpy(ptr, other->data, ULIST_BSIZE(other->num));
  ULIST_UNLOCK_RET(true);
}

bool ulist_delete_multi(ULIST list, ulist_offset_t index, ulist_size_t num) {
  if (num == 0) return false;

  ULIST_LOCK();
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1 || list->num == 0) ULIST_UNLOCK_RET(false);
  if (i + num > list->num) num = list->num - i;
  if (list->elfree != NULL) {
    for (ulist_size_t j = 0; j < num; j++) {
      list->elfree(ULIST_PTR((i + j)));
    }
  }
  if (i + num == list->num) {  // directly delete from tail
    list->num -= num;
  } else {  // delete from middle, need to move data
    uint8_t* src = ULIST_PTR((i + num));
    ulist_size_t move_size = ULIST_BSIZE(list->num - (i + num));
    uint8_t* dst = ULIST_PTR(i);
    _ulist_memmove(dst, src, move_size);
    list->num -= num;
  }
  ulist_shrink(list, list->num);
  list->iter = -1;
  ULIST_UNLOCK_RET(true);
}

bool ulist_delete(ULIST list, ulist_offset_t index) {
  return ulist_delete_multi(list, index, 1);
}

bool ulist_delete_slice(ULIST list, ulist_offset_t start, ulist_offset_t end) {
  ULIST_LOCK();
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) ULIST_UNLOCK_RET(false);
  ULIST_UNLOCK_RET(ulist_delete_multi(list, start, end - start));
}

bool ulist_remove(ULIST list, const void* ptr) {
  ULIST_LOCK();
  ulist_offset_t i = ulist_index(list, ptr);
  if (i == -1) ULIST_UNLOCK_RET(false);
  ULIST_UNLOCK_RET(ulist_delete(list, i));
}

void* ulist_slice_to_newmem(ULIST list, ulist_offset_t start,
                            ulist_offset_t end) {
  ULIST_LOCK();
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) ULIST_UNLOCK_RET(NULL);
  ulist_size_t num = end - start;
  uint8_t* ptr = _ulist_malloc(ULIST_BSIZE(num));
  if (ptr == NULL) ULIST_UNLOCK_RET(NULL);
  uint8_t* src = ULIST_PTR(start);
  _ulist_memcpy(ptr, src, ULIST_BSIZE(num));
  ULIST_UNLOCK_RET((void*)ptr);
}

bool ulist_slice_to_buf(ULIST list, ulist_offset_t start, ulist_offset_t end,
                        void* buf) {
  ULIST_LOCK();
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) ULIST_UNLOCK_RET(false);
  ulist_size_t num = end - start;
  uint8_t* src = ULIST_PTR(start);
  _ulist_memcpy(buf, src, ULIST_BSIZE(num));
  ULIST_UNLOCK_RET(true);
}

ULIST ulist_slice_to_newlist(ULIST list, ulist_offset_t start,
                             ulist_offset_t end) {
  ULIST_LOCK();
  start = convert_pylike_offset(list, start);
  end = convert_pylike_offset(list, end);
  if (start == -1 || end == -1) ULIST_UNLOCK_RET(NULL);
  ulist_size_t num = end - start;
  uint8_t* src = ULIST_PTR(start);
  ULIST new_list = ulist_new(list->isize, num, list->cfg, list->elfree);
  if (new_list == NULL) ULIST_UNLOCK_RET(NULL);
  _ulist_memcpy(new_list->data, src, ULIST_BSIZE(num));
  ULIST_UNLOCK_RET(new_list);
}

void* ulist_pop_to_newmem(ULIST list, ulist_offset_t index) {
  ULIST_LOCK();
  void* ptr = ulist_slice_to_newmem(list, index, index + 1);
  if (ptr == NULL) ULIST_UNLOCK_RET(NULL);
  ulist_delete(list, index);
  ULIST_UNLOCK_RET(ptr);
}

bool ulist_update(ULIST list, ulist_offset_t index, const void* src) {
  ULIST_LOCK();
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) ULIST_UNLOCK_RET(false);
  if (list->elfree != NULL) {
    list->elfree(ULIST_PTR(i));
  }
  _ulist_memcpy(ULIST_PTR(i), src, list->isize);
  ULIST_UNLOCK_RET(true);
}

void* ulist_get(ULIST list, ulist_offset_t index) {
  ULIST_LOCK();
  ulist_offset_t i;
  if (index == SLICE_START) {
    i = 0;
  } else if (index == SLICE_END) {
    i = list->num - 1;
  } else {
    i = convert_pylike_offset(list, index);
  }
  if (i == -1) ULIST_UNLOCK_RET(NULL);
  ULIST_UNLOCK_RET((void*)(ULIST_PTR(i)));
}

void* __ulist_foreach_init_ptr(ULIST list, ulist_offset_t index,
                               ulist_offset_t step, bool isStart) {
  ULIST_LOCK();
  if (step == 0) ULIST_UNLOCK_RET(NULL);
  ulist_offset_t i = convert_pylike_offset(list, index);
  if (i == -1) ULIST_UNLOCK_RET(NULL);
  if (step < 0) {  // reverse
    if (index == SLICE_END && isStart) i = list->num - 1;
    if (index == SLICE_START && !isStart) i = -1;
  }
  ULIST_UNLOCK_RET((void*)(ULIST_PTR(i)));
}

bool ulist_get_item(ULIST list, ulist_offset_t index, void* target) {
  ULIST_LOCK();
  void* ptr = ulist_get(list, index);
  if (ptr == NULL) ULIST_UNLOCK_RET(false);
  _ulist_memcpy(target, ptr, list->isize);
  ULIST_UNLOCK_RET(true);
}

bool ulist_swap(ULIST list, ulist_offset_t index1, ulist_offset_t index2) {
  ULIST_LOCK();
  ulist_offset_t i1 = convert_pylike_offset(list, index1);
  ulist_offset_t i2 = convert_pylike_offset(list, index2);
  if (i1 == -1 || i2 == -1) ULIST_UNLOCK_RET(false);
  uint8_t* ptr1 = ULIST_PTR(i1);
  uint8_t* ptr2 = ULIST_PTR(i2);
  uint8_t tmp;
  for (ulist_size_t i = 0; i < list->isize; i++) {
    tmp = ptr1[i];
    ptr1[i] = ptr2[i];
    ptr2[i] = tmp;
  }
  ULIST_UNLOCK_RET(true);
}

bool ulist_sort(ULIST list, int (*cmp)(const void*, const void*),
                ulist_offset_t start, ulist_offset_t end) {
  ULIST_LOCK();
  ulist_offset_t i = convert_pylike_offset(list, start);
  ulist_offset_t j = convert_pylike_offset(list, end);
  if (i == -1 || j == -1) ULIST_UNLOCK_RET(false);
  qsort(ULIST_PTR(i), j - i, list->isize, cmp);
  ULIST_UNLOCK_RET(true);
}

ulist_offset_t ulist_index(ULIST list, const void* ptr) {
  ULIST_LOCK();
  ulist_offset_t i = (uint8_t*)ptr - (uint8_t*)list->data;
  if (i < 0 || i % list->isize != 0) ULIST_UNLOCK_RET(false);
  i /= list->isize;
  if (i >= list->num) ULIST_UNLOCK_RET(false);
  ULIST_UNLOCK_RET(i);
}

ulist_offset_t ulist_find(ULIST list, const void* ptr) {
  ULIST_LOCK();
  for (ulist_offset_t i = 0; i < list->num; i++) {
    if (_ulist_memcmp(ULIST_PTR(i), ptr, list->isize) == 0) {
      ULIST_UNLOCK_RET(i);
    }
  }
  ULIST_UNLOCK_RET(-1);
}

void* ulist_search_matched(ULIST list, const void* key,
                           bool (*match)(const void* item, const void* key)) {
  ULIST_LOCK();
  for (ulist_offset_t i = 0; i < list->num; i++) {
    if (match(ULIST_PTR(i), key)) {
      ULIST_UNLOCK_RET((void*)(ULIST_PTR(i)));
    }
  }
  ULIST_UNLOCK_RET(NULL);
}

void ulist_mem_shrink(ULIST list) {
  ULIST_LOCK();
  uint8_t cfg = list->cfg;
  list->cfg &= ~ULIST_CFG_NO_AUTO_FREE;
  list->cfg |= ULIST_CFG_NO_SHRINK;
  ulist_shrink(list, list->num);
  list->cfg = cfg;
  ULIST_UNLOCK();
}

void ulist_clear(ULIST list) {
  ULIST_LOCK();
  if (list->elfree != NULL && list->num > 0) {
    for (ulist_size_t i = 0; i < list->num; i++) {
      list->elfree(ULIST_PTR(i));
    }
  }
  ulist_shrink(list, 0);
  list->num = 0;
  list->iter = -1;
  ULIST_UNLOCK();
}

bool ulist_iter(ULIST list, void** ptrptr, ulist_offset_t start,
                ulist_offset_t end, ulist_offset_t step) {
  if (step == 0) return false;
  ULIST_LOCK();
  ulist_offset_t start_n = convert_pylike_offset(list, start);
  ulist_offset_t end_n = convert_pylike_offset(list, end);
  if (start_n == -1 || end_n == -1) ULIST_UNLOCK_RET(false);
  if (step < 0) {  // reverse
    if (start == SLICE_END) start_n = list->num - 1;
    if (end == SLICE_START) end_n = -1;
  }
  if (list->iter < 0) {
    list->iter = start_n;
  } else {
    list->iter += step;
  }
  if ((step > 0 && list->iter >= end_n) || (step < 0 && list->iter <= end_n) ||
      list->iter >= list->num || list->iter < 0) {
    list->iter = -1;
    ULIST_UNLOCK_RET(false);
  }
  *ptrptr = (void*)(ULIST_PTR(list->iter));
  ULIST_UNLOCK_RET(true);
}

ulist_offset_t ulist_iter_index(ULIST list) { return list->iter; }

void ulist_iter_stop(ULIST list) { list->iter = -1; }

ULIST_ITER ulist_new_iterator(ULIST list, ulist_offset_t start,
                              ulist_offset_t end, ulist_offset_t step) {
  ULIST_LOCK();
  ulist_offset_t start_n = convert_pylike_offset(list, start);
  ulist_offset_t end_n = convert_pylike_offset(list, end);
  if (start_n == -1 || end_n == -1) ULIST_UNLOCK_RET(NULL);
  if (step < 0) {  // reverse
    if (start == SLICE_END) start_n = list->num - 1;
    if (end == SLICE_START) end_n = -1;
  }
  ULIST_ITER iter = (ULIST_ITER)_ulist_malloc(sizeof(ulist_iter_t));
  if (iter == NULL) ULIST_UNLOCK_RET(NULL);
  iter->target = list;
  iter->start = start_n;
  iter->end = end_n;
  iter->step = step;
  iter->now = -1;
  ULIST_UNLOCK_RET(iter);
}

void ulist_free_iterator(ULIST_ITER iter) { _ulist_free(iter); }

void* ulist_iterator_next(ULIST_ITER iter) {
  if (iter->now == -1) {
    iter->now = iter->start;
  } else {
    iter->now += iter->step;
  }
  if ((iter->step > 0 && iter->now >= iter->end) ||
      (iter->step < 0 && iter->now <= iter->end) ||
      iter->now >= iter->target->num || iter->now < 0) {
    return NULL;
  }
  return (void*)((uint8_t*)iter->target->data +
                 iter->now * iter->target->isize);
}

void* ulist_iterator_prev(ULIST_ITER iter) {
  if (iter->now == -1) {
    return NULL;
  } else {
    iter->now -= iter->step;
  }
  if ((iter->step > 0 && iter->now >= iter->end) ||
      (iter->step < 0 && iter->now <= iter->end) ||
      iter->now >= iter->target->num || iter->now < 0) {
    return NULL;
  }
  return (void*)((uint8_t*)iter->target->data +
                 iter->now * iter->target->isize);
}

void* ulist_iterator_now(ULIST_ITER iter) {
  if (iter->now == -1) {
    return NULL;
  }
  return (void*)((uint8_t*)iter->target->data +
                 iter->now * iter->target->isize);
}

ulist_offset_t ulist_iterator_index(ULIST_ITER iter) { return iter->now; }

void ulist_iterator_reset(ULIST_ITER iter) { iter->now = -1; }

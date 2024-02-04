/**
 * @file ulist.h
 * @brief 数据类型通用的动态列表实现，支持Python风格的负索引，内存连续
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-01-05
 *
 * THINK DIFFERENTLY
 */

#ifndef __ULIST_H__
#define __ULIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

typedef uint32_t ulist_size_t;
typedef int32_t ulist_offset_t;

#define SLICE_START (INT32_MAX)    // like list[:index] in Python
#define SLICE_END (INT32_MAX - 1)  // like list[index:] in Python

#pragma pack(1)
typedef struct {
  void* data;             // 数据缓冲区
  ulist_size_t num;       // 列表内元素个数
  ulist_size_t cap;       // 缓冲区容量(元素个数)
  ulist_size_t isize;     // 元素大小(字节)
  ulist_offset_t iter;    // 迭代器位置(<0:未初始化)
  uint8_t cfg;            // 配置
  bool dyn;               // 是否动态分配
  void (*elfree)(void*);  // 元素释放函数
#if _MOD_USE_OS
  MOD_MUTEX_HANDLE mutex;  // 互斥锁
#endif
} ulist_t;
typedef ulist_t* ULIST;

typedef struct {
  ULIST target;
  ulist_offset_t step;
  ulist_offset_t start;
  ulist_offset_t end;
  ulist_offset_t now;
} ulist_iter_t;
typedef ulist_iter_t* ULIST_ITER;
#pragma pack()

#define ULIST_DIRTY_REGION_FILL_DATA 0x00  // 区域填充值
#define ULIST_DISABLE_ALL_LOG 0            // 禁用所有日志
#define ULIST_MAX_EXTEND_SIZE 128          // 最大扩展大小(元素个数)

#define ULIST_CFG_CLEAR_DIRTY_REGION 0x01  // 用memset填充申请/释放的内存区域
#define ULIST_CFG_NO_ALLOC_EXTEND 0x02  // 严格按照需要的大小分配内存
#define ULIST_CFG_NO_SHRINK 0x04  // 不自动缩小内存(但元素个数为0时仍会释放内存)
#define ULIST_CFG_NO_AUTO_FREE 0x08        // 元素个数为0时不释放内存
#define ULIST_CFG_IGNORE_SLICE_ERROR 0x10  // 忽略切片越界错误
#define ULIST_CFG_NO_ERROR_LOG 0x20        // 出错时不打印日志
#define ULIST_CFG_NO_MUTEX 0x40            // 不使用互斥锁

/**
 * @brief 初始化一个已创建的列表
 * @param  list         列表结构体
 * @param  isize        列表元素大小(使用sizeof计算)
 * @param  init_size    列表初始大小
 * @param  cfg          配置
 * @param  elfree       元素释放函数,当数组元素需要额外的释放操作时使用,可为NULL
 * @retval              是否初始化成功
 * @note 需手动调用ulist_clear释放列表
 * @note 对于静态分配的ulist_t, 可手动设置isize代替本函数
 */
extern bool ulist_init(ULIST list, ulist_size_t isize, ulist_size_t init_size,
                       uint8_t cfg, void (*elfree)(void* item));

/**
 * @brief 创建列表并初始化
 * @param  list         列表结构体
 * @param  isize        列表元素大小(使用sizeof计算)
 * @param  init_size    列表初始大小
 * @param  cfg          配置
 * @param  elfree       元素释放函数,当数组元素需要额外的释放操作时使用,可为NULL
 * @retval              返回列表结构体
 * @note 需手动调用ulist_free释放列表
 * @note 返回NULL说明内存操作失败
 */
extern ULIST ulist_new(ulist_size_t isize, ulist_size_t init_size, uint8_t cfg,
                       void (*elfree)(void* item));

/**
 * @brief 将num个空元素追加到列表末尾
 * @param  list    列表结构体
 * @param  num     追加元素个数
 * @return         返回追加部分的头指针
 * @note 返回NULL说明内存操作失败
 */
extern void* ulist_append_multi(ULIST list, ulist_size_t num);

/**
 * @brief 将1个空元素追加到列表末尾
 * @param  list    列表结构体
 * @return         返回追加部分的头指针
 * @note 返回NULL说明内存操作失败
 */
extern void* ulist_append(ULIST list);

/**
 * @brief 将1个外部元素的数据追加到列表末尾
 * @param  list    列表结构体
 * @param  src     追加元素指针
 * @return         是否追加成功
 * @note 返回false说明内存操作失败
 */
extern bool ulist_append_copy(ULIST list, const void* src);

/**
 * @brief 将另一个列表追加到列表末尾
 * @param  list    列表结构体
 * @param  other   另一个列表结构体
 * @retval         是否追加成功
 * @note 返回false说明内存操作失败
 */
extern bool ulist_extend(ULIST list, ULIST other);

/**
 * @brief 将另一个列表插入到列表中index位置(第index个元素之前)
 * @param  list    列表结构体
 * @param  index   插入位置(`Python-like`)
 * @param  other   另一个列表结构体
 * @retval         是否插入成功
 * @note 返回false说明内存操作失败
 */
extern bool ulist_extend_at(ULIST list, ULIST other, ulist_offset_t index);

/**
 * @brief 将1个元素插入到列表中index位置(第index个元素之前)
 * @param  list    列表结构体
 * @param  index   插入位置(`Python-like`)
 * @return         返回插入部分的头指针
 * @note 返回NULL说明内存操作失败或者index越界
 * @note index==`SLICE_END`时, 等价于append
 */
extern void* ulist_insert(ULIST list, ulist_offset_t index);

/**
 * @brief 将num个元素插入到列表中index位置(第index个元素之前)
 * @param  list    列表结构体
 * @param  index   插入位置(`Python-like`)
 * @param  num     插入元素个数
 * @return         返回插入部分的头指针
 * @note 返回NULL说明内存操作失败或者index越界
 * @note index==`SLICE_END`时, 等价于append
 */
extern void* ulist_insert_multi(ULIST list, ulist_offset_t index,
                                ulist_size_t num);

/**
 * @brief 将1个外部元素插入到列表中index位置(第index个元素之前)
 * @param  list    列表结构体
 * @param  index   插入位置(`Python-like`)
 * @param  src     插入元素指针
 * @return         是否插入成功
 * @note 返回false说明内存操作失败或者index越界
 * @note index==`SLICE_END`时, 等价于append_buf
 * @note 缓冲区数据需具有和列表元素相同的大小
 */
extern bool ulist_insert_copy(ULIST list, ulist_offset_t index,
                              const void* src);

/**
 * @brief 删除列表中给定位置的元素
 * @param  list       列表结构体
 * @param  index      删除位置(`Python-like`)
 * @return            是否删除成功
 * @note 返回false说明index越界
 */
extern bool ulist_delete(ULIST list, ulist_offset_t index);

/**
 * @brief 删除列表中给定位置的元素
 * @param  list       列表结构体
 * @param  index      删除位置(`Python-like`)
 * @param  num        删除元素个数
 * @return            是否删除成功
 * @note 超出列表范围的num会被截断, 不会导致删除失败
 * @note 返回false说明index越界
 */
extern bool ulist_delete_multi(ULIST list, ulist_offset_t index,
                               ulist_size_t num);

/**
 * @brief 删除列表中的给定元素
 * @param  list       列表结构体
 * @param  start      删除起始位置(`Python-like`)
 * @param  end        删除结束位置(`Python-like`, 不包括)
 * @return            是否删除成功
 * @note 返回false说明index越界
 */
extern bool ulist_delete_slice(ULIST list, ulist_offset_t start,
                               ulist_offset_t end);

/**
 * @brief 删除列表中的给定元素
 * @param  list       列表结构体
 * @param  ptr        元素指针(必须在列表中)
 * @return            是否删除成功
 * @note 返回false说明ptr不在列表中
 */
extern bool ulist_remove(ULIST list, const void* ptr);

/**
 * @brief 返回列表元素的切片(浅拷贝), 创建新数据块
 * @param  list       列表结构体
 * @param  start      切片起始位置(`Python-like`)
 * @param  end        切片结束位置(`Python-like`, 不包括)
 * @retval            返回切片后的数据块头指针
 * @note 切片的元素需由用户手动调用free释放
 * @note 返回NULL说明index越界
 * @note 常规意义的切片应使用ulist_get_ptr，此处返回副本
 */
extern void* ulist_slice_to_newmem(ULIST list, ulist_offset_t start,
                                   ulist_offset_t end);

/**
 * @brief 返回列表元素的切片(浅拷贝), 写入缓冲区
 * @param  list       列表结构体
 * @param  start      切片起始位置(`Python-like`)
 * @param  end        切片结束位置(`Python-like`, 不包括)
 * @param  buf        切片元素缓冲区
 * @return            是否切片成功
 * @note 超出列表范围的num会导致拷贝失败
 * @note 返回false说明index越界
 */
extern bool ulist_slice_to_buf(ULIST list, ulist_offset_t start,
                               ulist_offset_t end, void* buf);

/**
 * @brief 返回列表元素的切片(浅拷贝), 创建新列表
 * @param  list      列表结构体
 * @param  start      切片起始位置(`Python-like`)
 * @param  end        切片结束位置(`Python-like`, 不包括)
 * @retval           返回切片后的列表结构体
 * @note 切片后的列表需由用户手动调用ulist_free释放
 * @note 返回NULL说明内存操作失败
 */
extern ULIST ulist_slice_to_newlist(ULIST list, ulist_offset_t start,
                                    ulist_offset_t end);

/**
 * @brief 弹出并删除列表中的元素, 创建新数据块
 * @param  list       列表结构体
 * @param  index      弹出位置(`Python-like`)
 * @return            返回弹出部分的头指针
 * @note 弹出的元素需由用户手动调用free释放
 * @note 返回NULL说明index越界
 * @note 建议使用get + delete代替pop
 */
extern void* ulist_pop_to_newmem(ULIST list, ulist_offset_t index);

/**
 * @brief 用外部元素更新列表中index位置的元素
 * @param  list       列表结构体
 * @param  index      元素位置(`Python-like`)
 * @param  src        元素指针
 * @return            是否更新成功
 * @note 返回false说明index越界
 */
extern bool ulist_update(ULIST list, ulist_offset_t index, const void* src);

/**
 * @brief 获取列表中index位置的元素
 * @param  list       列表结构体
 * @param  index      元素位置(`Python-like`)
 * @return            返回元素指针
 * @note 返回NULL说明index越界
 */
extern void* ulist_get(ULIST list, ulist_offset_t index);

/**
 * @brief 获取列表中index位置的元素, 写入给定副本中
 * @param  list       列表结构体
 * @param  index      元素位置(`Python-like`)
 * @param  target     元素副本指针
 * @return            是否获取成功
 * @note 返回NULL说明index越界
 */
extern bool ulist_get_item(ULIST list, ulist_offset_t index, void* target);

/**
 * @brief 获取元素指针在列表中的位置
 * @param  list       列表结构体
 * @param  ptr        元素指针(必须在列表中)
 * @return            返回元素位置(>=0)
 * @note 返回-1说明指针不在列表中
 */
extern ulist_offset_t ulist_index(ULIST list, const void* ptr);

/**
 * @brief 查找与对应数据相同的元素在列表中的位置
 * @param  list       列表结构体
 * @param  ptr        数据指针(比较完整数据内容)
 * @return            返回元素位置(>=0)
 * @note 返回-1说明数据不在列表中
 * @note 数据需具有和列表元素相同的结构
 */
extern ulist_offset_t ulist_find(ULIST list, const void* ptr);

/**
 * @brief 查找与对应键匹配的元素
 * @param  list       列表结构体
 * @param  key        键指针
 * @param  match      匹配函数(返回true时匹配成功)
 * @return            返回元素指针
 * @note 返回NULL说明未找到匹配的元素
 */
extern void* ulist_search_matched(ULIST list, const void* key,
                                  bool (*match)(const void* item,
                                                const void* key));

/**
 * @brief 交换列表中两个元素的位置
 * @param  list       列表结构体
 * @param  index1     元素1位置(`Python-like`)
 * @param  index2     元素2位置(`Python-like`)
 * @return            是否交换成功
 */
extern bool ulist_swap(ULIST list, ulist_offset_t index1,
                       ulist_offset_t index2);

/**
 * @brief 列表排序(qsort)
 * @param  list      列表结构体
 * @param  cmp       比较函数(与qsort相同,cmp(p1,p2))
 * @param  start     排序起始位置(`Python-like`)
 * @param  end       排序结束位置(`Python-like`, 不包括)
 * @retval           是否排序成功
 * @note 全列表排序: start=`SLICE_START`, end=`SLICE_END`
 * @note `<0:p1,p2`  `=0:p?,p?`  `>0:p2,p1`
 */
extern bool ulist_sort(ULIST list, int (*cmp)(const void*, const void*),
                       ulist_offset_t start, ulist_offset_t end);

/**
 * @brief 内部迭代列表
 * @param  list     列表结构体
 * @param  ptrptr   将要指向当前元素指针的指针
 * @param  start    迭代起始位置(`Python-like`)
 * @param  end      迭代结束位置(`Python-like`, 不包括)
 * @param  step     迭代步长
 * @retval          是否继续迭代
 * @note    返回false后的下一次iter将自动重置迭代器, 从头开始迭代
 * @warning 该函数线程不安全
 * @warning 在迭代过程中增删元素会重置迭代器为0
 */
extern bool ulist_iter(ULIST list, void** ptrptr, ulist_offset_t start,
                       ulist_offset_t end, ulist_offset_t step);

/**
 * @brief 获取内部迭代器当前元素位置
 * @param  list    列表结构体
 * @retval         返回元素位置
 * @note 返回-1说明迭代结束或迭代器未曾迭代过
 */
extern ulist_offset_t ulist_iter_index(ULIST list);

/**
 * @brief 提前停止内部迭代
 * @param  list     列表结构体
 * @warning 该函数线程不安全
 */
extern void ulist_iter_stop(ULIST list);

/**
 * @brief 创建列表迭代器
 * @param  list      列表结构体
 * @param  start     迭代起始位置(`Python-like`)
 * @param  end       迭代结束位置(`Python-like`, 不包括)
 * @param  step      迭代步长
 * @retval           返回迭代器结构体
 * @note 返回NULL说明index越界
 * @note 迭代器需由用户手动调用`ulist_free_iterator`释放
 * @note step为负数时, start应大于end
 */
extern ULIST_ITER ulist_new_iterator(ULIST list, ulist_offset_t start,
                                     ulist_offset_t end, ulist_offset_t step);

/**
 * @brief 释放列表迭代器
 * @param  iter      迭代器结构体
 */
extern void ulist_free_iterator(ULIST_ITER iter);

/**
 * @brief 迭代器下一元素
 * @param  iter      迭代器结构体
 * @retval           返回元素指针
 * @note 返回NULL说明迭代结束
 * @warning 返回NULL后的下一次next将自动重置迭代器, 从头开始迭代
 */
extern void* ulist_iterator_next(ULIST_ITER iter);

/**
 * @brief 迭代器上一元素
 * @param  iter      迭代器结构体
 * @retval           返回元素指针
 * @note 返回NULL说明迭代结束
 * @warning 返回NULL后的下一次prev将自动重置迭代器, 从尾开始迭代
 */
extern void* ulist_iterator_prev(ULIST_ITER iter);

/**
 * @brief 迭代器当前元素
 * @param  iter      迭代器结构体
 * @retval           返回元素指针
 * @note 返回NULL说明迭代结束或迭代器未曾迭代过
 */
extern void* ulist_iterator_now(ULIST_ITER iter);

/**
 * @brief 迭代器当前元素位置
 * @param  iter      迭代器结构体
 * @retval           返回元素位置
 * @note 返回-1说明迭代结束或迭代器未曾迭代过
 */
extern ulist_offset_t ulist_iterator_index(ULIST_ITER iter);

/**
 * @brief 重置迭代器
 * @param  iter      迭代器结构体
 */
extern void ulist_iterator_reset(ULIST_ITER iter);

/**
 * @brief 清空列表
 * @param  list       列表结构体
 */
extern void ulist_clear(ULIST list);

/**
 * @brief 释放列表
 * @param  list       列表结构体
 */
extern void ulist_free(ULIST list);

/**
 * @brief 手动缩减列表储存区
 * @param  list       列表结构体
 * @note
 * 这是为启用了`ULIST_CFG_NO_SHRINK`或`ULIST_CFG_NO_AUTO_FREE`的列表提供的手动缩减函数
 */
extern void ulist_mem_shrink(ULIST list);

/**
 * @brief 获取列表长度
 * @param  list       列表结构体
 */
static inline ulist_size_t ulist_len(ULIST list) { return list->num; }

/**
 * @brief 获取列表对应位置的元素指针
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  index      元素位置(`Python-like`)
 */
#define ulist_get_ptr(list, type, index) ((type*)ulist_get(list, index))

extern void* __ulist_foreach_init_ptr(ULIST list, ulist_offset_t index,
                                      ulist_offset_t step, bool isStart);
#define _ulist_foreach_init_ptr(list, type, index, step, isStart) \
  ((type*)__ulist_foreach_init_ptr(list, index, step, isStart))
/**
 * @brief 循环遍历列表, 就像 `for var in list[from_index:to_index:step]` 一样
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @param  from_index 起始位置
 * @param  to_index   结束位置(`Python-like`, 不包括)
 * @param  step       步长
 * @note step为负数时, from_index应大于to_index
 * @note 无越界检查, 不要在循环中修改列表结构，如必须增删需考虑修改[var]_end
 */
#define ulist_foreach_from_to_step(list, type, var, from_index, to_index,  \
                                   step)                                   \
  for (type* var##_end =                                                   \
           _ulist_foreach_init_ptr(list, type, to_index, step, false);     \
       var##_end != NULL; var##_end = NULL)                                \
    for (type* var =                                                       \
             _ulist_foreach_init_ptr(list, type, from_index, step, true);  \
         var != NULL &&                                                    \
         ((step > 0 && var < var##_end) || (step < 0 && var > var##_end)); \
         var += step)

/**
 * @brief 循环遍历列表, 就像 `for var in list[from_index:to_index]` 一样
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @param  from_index 起始位置
 * @param  to_index   结束位置(`Python-like`, 不包括)
 * @note 无越界检查, 不要在循环中修改列表结构，如必须增删需考虑修改[var]_end
 */
#define ulist_foreach_from_to(list, type, var, from_index, to_index) \
  ulist_foreach_from_to_step(list, type, var, from_index, to_index, 1)

/**
 * @brief 循环遍历列表, 就像 `for var in list` 一样
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @note 无运行时越界检查,
 * 不要在循环中修改列表结构，如必须增删需考虑修改[var]_end
 */
#define ulist_foreach(list, type, var) \
  ulist_foreach_from_to(list, type, var, 0, SLICE_END)

/**
 * @brief 循环遍历列表, 就像 `for var in list[from_index:]` 一样
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @param  from_index 起始位置(`Python-like`)
 * @note 无运行时越界检查,
 * 不要在循环中修改列表结构，如必须增删需考虑修改[var]_end
 */
#define ulist_foreach_from(list, type, var, from_index) \
  ulist_foreach_from_to(list, type, var, from_index, SLICE_END)

/**
 * @brief 循环遍历列表, 就像 `for var in list[:to_index]` 一样
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @param  to_index   结束位置(`Python-like`, 不包括)
 * @note 无运行时越界检查,
 * 不要在循环中修改列表结构，如必须增删需考虑修改[var]_end
 */
#define ulist_foreach_to(list, type, var, to_index) \
  ulist_foreach_from_to(list, type, var, 0, to_index)

#ifdef __cplusplus
}
#endif
#endif  // __ULIST_H__

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

typedef uint16_t ulist_size_t;
typedef int16_t ulist_offset_t;

#define SLICE_START 0x7FFF  // like list[:index] in Python
#define SLICE_END 0x7FFE    // like list[index:] in Python

typedef struct {
  void* data;          // 数据缓冲区
  ulist_size_t num;    // 列表内元素个数
  ulist_size_t cap;    // 缓冲区容量(元素个数)
  ulist_size_t isize;  // 元素大小(字节)
  uint8_t cfg;         // 配置
} ulist_t;

typedef ulist_t* ULIST;

#define ULIST_DIRTY_REGION_FILL_DATA 0x66  // 区域填充值

#define ULIST_CFG_CLEAR_DIRTY_REGION 0x01  // 用memset填充释放的内存区域
#define ULIST_CFG_NO_ALLOC_EXTEND 0x02  // 严格按照需要的大小分配内存
#define ULIST_CFG_NO_SHRINK 0x04        // 不自动缩小内存
#define ULIST_CFG_NO_AUTO_FREE 0x08     // 元素个数为0时不释放内存
#define ULIST_CFG_IGNORE_SLICE_ERROR 0x10  // 忽略切片越界错误
#define ULIST_CFG_IGNORE_NUM_ERROR 0x20    // 忽略元素个数错误
#define ULIST_CFG_SLICE_ERROR_NO_LOG 0x40  // 切片越界时不打印日志

/**
 * @brief 初始化列表
 * @param  list         列表结构体
 * @param  isize        列表元素大小(使用sizeof计算)
 * @param  initial_cap  列表初始容量
 * @param  cfg          配置
 * @note 对于静态分配的ulist_t, 可手动设置isize代替本函数
 */
extern void ulist_init(ULIST list, ulist_size_t isize, ulist_size_t initial_cap,
                       uint8_t cfg);

/**
 * @brief 创建列表
 * @param  list         列表结构体
 * @param  isize        列表元素大小(使用sizeof计算)
 * @param  initial_cap  列表初始容量
 * @param  cfg          配置
 * @retval              返回列表结构体
 * @note 需手动调用ulist_free释放列表
 * @note 返回NULL说明内存操作失败
 */
extern ULIST ulist_create(ulist_size_t isize, ulist_size_t initial_cap,
                          uint8_t cfg);

/**
 * @brief 将num个元素追加到列表末尾
 * @param  list    列表结构体
 * @param  num     追加元素个数
 * @return         返回追加部分的头指针
 * @note 返回NULL说明内存操作失败
 */
extern void* ulist_append(ULIST list, ulist_size_t num);

/**
 * @brief 从指定缓冲区将num个元素追加到列表末尾
 * @param  list    列表结构体
 * @param  num     追加元素个数
 * @param  buf     追加元素缓冲区
 * @return         是否追加成功
 * @note 返回false说明内存操作失败
 * @note 缓冲区数据需具有和列表元素相同的大小
 */
extern bool ulist_append_buf(ULIST list, ulist_size_t num, const void* buf);

/**
 * @brief 将num个元素插入到列表中index位置(第index个元素之前)
 * @param  list    列表结构体
 * @param  index   插入位置(Python-like)
 * @param  num     插入元素个数
 * @return         返回插入部分的头指针
 * @note 返回NULL说明内存操作失败或者index越界
 * @note index==SLICE_END时, 等价于append
 */
extern void* ulist_insert(ULIST list, ulist_offset_t index, ulist_size_t num);

/**
 * @brief 从指定缓冲区将num个元素插入到列表中index位置(第index个元素之前)
 * @param  list    列表结构体
 * @param  index   插入位置(Python-like)
 * @param  num     插入元素个数
 * @param  buf     插入元素缓冲区
 * @return         是否插入成功
 * @note 返回false说明内存操作失败或者index越界
 * @note index==SLICE_END时, 等价于append_buf
 * @note 缓冲区数据需具有和列表元素相同的大小
 */
extern bool ulist_insert_buf(ULIST list, ulist_offset_t index, ulist_size_t num,
                             const void* buf);

/**
 * @brief 将另一个列表追加到列表末尾
 * @param  list    列表结构体
 * @param  other   另一个列表结构体
 * @retval         是否追加成功
 * @note 返回false说明内存操作失败
 */
extern bool ulist_extend(ULIST list, ULIST other);

/**
 * @brief 删除列表中给定位置的元素
 * @param  list       列表结构体
 * @param  index      删除位置(Python-like)
 * @param  num        删除元素个数
 * @return            是否删除成功
 * @note 超出列表范围的num会被截断, 不会导致删除失败
 * @note 返回false说明index越界
 */
extern bool ulist_delete(ULIST list, ulist_offset_t index, ulist_size_t num);

/**
 * @brief 删除列表中的给定元素
 * @param  list       列表结构体
 * @param  ptr        元素指针(必须在列表中)
 * @return            是否删除成功
 * @note 返回false说明ptr不在列表中
 */
extern bool ulist_remove(ULIST list, const void* ptr);

/**
 * @brief 返回列表元素的浅拷贝, 创建新数据块
 * @param  list       列表结构体
 * @param  index      拷贝起始位置(Python-like)
 * @param  num        拷贝元素个数
 * @retval            返回拷贝后的数据块头指针
 * @note 弹出的元素需由用户手动调用free释放
 * @note 超出列表范围的num会导致弹出失败
 * @note 返回NULL说明index越界
 */
extern void* ulist_copy(ULIST list, ulist_offset_t index, ulist_size_t num);

/**
 * @brief 返回列表元素的浅拷贝, 写入缓冲区
 * @param  list       列表结构体
 * @param  index      拷贝起始位置(Python-like)
 * @param  num        拷贝元素个数
 * @param  buf        拷贝元素缓冲区
 * @return            是否拷贝成功
 * @note 超出列表范围的num会导致拷贝失败
 * @note 返回false说明index越界
 */
extern bool ulist_copy_buf(ULIST list, ulist_offset_t index, ulist_size_t num,
                           void* buf);

/**
 * @brief 返回列表元素的浅拷贝, 创建新列表
 * @param  list      列表结构体
 * @param  index     拷贝起始位置(Python-like)
 * @param  num       拷贝元素个数
 * @retval           返回拷贝后的列表结构体
 * @note 拷贝后的列表需由用户手动调用ulist_free释放
 * @note 返回NULL说明内存操作失败
 */
extern ULIST ulist_copy_list(ULIST list, ulist_offset_t index,
                             ulist_size_t num);

/**
 * @brief 弹出并删除列表中的元素, 创建新数据块
 * @param  list       列表结构体
 * @param  index      弹出位置(Python-like)
 * @param  num        弹出元素个数
 * @return            返回弹出部分的头指针
 * @note 弹出的元素需由用户手动调用free释放
 * @note 超出列表范围的num会导致弹出失败
 * @note 返回NULL说明index越界
 */
extern void* ulist_pop(ULIST list, ulist_offset_t index, ulist_size_t num);

/**
 * @brief 弹出并删除列表中的元素, 创建新列表
 * @param  list       列表结构体
 * @param  index      弹出位置(Python-like)
 * @param  num        弹出元素个数
 * @return            返回弹出部分的列表结构体
 * @note 弹出的元素需由用户手动调用ulist_free释放
 * @note 超出列表范围的num会导致弹出失败
 * @note 返回NULL说明index越界
 */
extern ULIST ulist_pop_list(ULIST list, ulist_offset_t index, ulist_size_t num);

/**
 * @brief 弹出并删除列表中的元素, 写入缓冲区
 * @param  list       列表结构体
 * @param  index      弹出位置(Python-like)
 * @param  num        弹出元素个数
 * @param  buf        弹出元素缓冲区
 * @return            是否弹出成功
 * @note 超出列表范围的num会导致弹出失败
 * @note 返回false说明index越界
 */
extern bool ulist_pop_buf(ULIST list, ulist_offset_t index, ulist_size_t num,
                          void* buf);

/**
 * @brief 获取列表中index位置的元素
 * @param  list       列表结构体
 * @param  index      元素位置(Python-like)
 * @return            返回元素指针
 * @note 返回NULL说明index越界
 */
extern void* ulist_get(ULIST list, ulist_offset_t index);

/**
 * @brief 获取元素指针在列表中的位置
 * @param  list       列表结构体
 * @param  ptr        元素指针(必须在列表中)
 * @return            返回元素位置(>=0)
 * @note 返回<0说明指针不在列表中
 */
extern ulist_offset_t ulist_index(ULIST list, const void* ptr);

/**
 * @brief 查找与对应数据相同的元素在列表中的位置
 * @param  list       列表结构体
 * @param  ptr        数据指针(比较数据内容)
 * @return            返回元素位置(>=0)
 * @note 返回<0说明数据不在列表中
 * @note 数据需具有和列表元素相同的大小
 */
extern ulist_offset_t ulist_find(ULIST list, const void* ptr);

/**
 * @brief 交换列表中两个元素的位置
 * @param  list       列表结构体
 * @param  index1     元素1位置(Python-like)
 * @param  index2     元素2位置(Python-like)
 * @return            是否交换成功
 */
extern bool ulist_swap(ULIST list, ulist_offset_t index1,
                       ulist_offset_t index2);

/**
 * @brief 列表排序(qsort)
 * @param  list      列表结构体
 * @param  cmp       比较函数(与qsort相同,cmp(p1,p2))
 * @param  start     排序起始位置(Python-like)
 * @param  end       排序结束位置(Python-like, 不包括)
 * @retval           是否排序成功
 * @note 全列表排序: start=SLICE_START, end=SLICE_END
 * @note <0:p1,p2  =0:p?,p?  >0:p2,p1
 */
extern bool ulist_sort(ULIST list, int (*cmp)(const void*, const void*),
                       ulist_offset_t start, ulist_offset_t end);

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
 * @brief 获取列表长度
 * @param  list       列表结构体
 */
static inline ulist_size_t ulist_len(ULIST list) { return list->num; }

/**
 * @brief 获取列表对应位置的元素指针
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  index      元素位置(Python-like)
 */
#define ulist_get_ptr(list, type, index) ((type*)ulist_get(list, index))

/**
 * @brief 弹出并删除列表对应位置的元素指针
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  index      元素位置(Python-like)
 * @param  num        元素个数
 */
#define ulist_pop_ptr(list, type, index, num) \
  ((type*)ulist_pop(list, index, num))

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @note 不要在循环中修改列表结构
 */
#define ulist_foreach(list, type, var)                          \
  for (type* var = ulist_get_ptr(list, type, 0),                \
             *var##_end = ulist_get_ptr(list, type, SLICE_END); \
       var < var##_end; var++)

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @param  from_index 起始位置(Python-like)
 * @note 不要在循环中修改列表结构
 */
#define ulist_foreach_from(list, type, var, from_index)         \
  for (type* var = ulist_get_ptr(list, type, from_index),       \
             *var##_end = ulist_get_ptr(list, type, SLICE_END); \
       var < var##_end; var++)

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @param  to_index   结束位置(Python-like, 不包括)
 * @note 不要在循环中修改列表结构
 */
#define ulist_foreach_to(list, type, var, to_index)            \
  for (type* var = ulist_get_ptr(list, type, 0),               \
             *var##_end = ulist_get_ptr(list, type, to_index); \
       var < var##_end; var++)

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @param  from_index 起始位置
 * @param  to_index   结束位置(Python-like, 不包括)
 * @note 不要在循环中修改列表结构
 */
#define ulist_foreach_from_to(list, type, var, from_index, to_index) \
  for (type* var = ulist_get_ptr(list, type, from_index),            \
             *var##_end = ulist_get_ptr(list, type, to_index);       \
       var < var##_end; var++)

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名([var]->[var]_end)
 * @param  from_index 起始位置
 * @param  to_index   结束位置(Python-like, 不包括)
 * @param  step       步长
 * @note step为负数时, from_index应大于to_index
 * @note 不要在循环中修改列表结构
 */
#define ulist_foreach_from_to_step(list, type, var, from_index, to_index, \
                                   step)                                  \
  for (type* var = ulist_get_ptr(list, type, from_index),                 \
             *var##_end = ulist_get_ptr(list, type, to_index);            \
       (step > 0 && var < var##_end) || (step < 0 && var > var##_end);    \
       var += step)

#ifdef __cplusplus
}
#endif
#endif  // __ULIST_H__

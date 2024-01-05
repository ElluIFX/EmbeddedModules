/**
 * @file ulist.h
 * @brief 通用的动态列表实现，支持Python风格的负索引，内存连续
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

typedef struct {
  void* data;          // data buffer
  ulist_size_t cap;    // actual buffer capacity
  ulist_size_t inum;   // number of elements
  ulist_size_t isize;  // size of each element
} ulist_t;

/**
 * @brief 初始化列表
 * @param  list    列表结构体
 * @param  isize   列表元素大小(使用sizeof计算)
 */
extern void ulist_init(ulist_t* list, size_t isize);

/**
 * @brief 将num个元素追加到列表末尾
 * @param  list    列表结构体
 * @param  num     元素个数
 * @return         返回追加部分的头指针
 * @note 返回NULL说明内存操作失败
 */
extern const void* ulist_append(ulist_t* list, ulist_size_t num);

/**
 * @brief 将num个元素插入到列表中index位置(第index个元素之前)
 * @param  list    列表结构体
 * @param  index   插入位置(Python-like)
 * @param  num     元素个数
 * @return         返回插入部分的头指针
 * @note 返回NULL说明内存操作失败或者index越界
 */
extern const void* ulist_insert(ulist_t* list, ulist_offset_t index,
                                ulist_size_t num);

/**
 * @brief 删除列表中index位置开始的num个元素(包括index位置的元素)
 * @param  list       列表结构体
 * @param  index      删除位置(Python-like)
 * @param  num        元素个数
 * @return            非零值表示删除失败
 */
extern int ulist_delete(ulist_t* list, ulist_offset_t index, ulist_size_t num);

/**
 * @brief 获取列表中index位置的元素
 * @param  list       列表结构体
 * @param  index      元素位置(Python-like)
 * @return            返回元素指针
 * @note 返回NULL说明index越界
 */
extern const void* ulist_get(ulist_t* list, ulist_offset_t index);

/**
 * @brief 清空列表
 * @param  list       列表结构体
 */
extern void ulist_clear(ulist_t* list);

/**
 * @brief 获取列表长度
 * @param  list       列表结构体
 */
#define ulist_len(list) ((list)->inum)

/**
 * @brief 获取列表对应位置的元素指针
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  index      元素位置(Python-like)
 */
#define ulist_get_ptr(list, type, index) ((type*)ulist_get(list, index))

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名
 */
#define ulist_foreach(list, type, var)  \
  for (type* var = (type*)(list)->data; \
       var < (type*)(list)->data + (list)->inum; var++)

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名
 * @param  from_index 起始位置(Python-like)
 */
#define ulist_foreach_from(list, type, var, from_index) \
  for (type* var = (type*)ulist_get(list, from_index);  \
       var < (type*)(list)->data + (list)->inum; var++)

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名
 * @param  to_index   结束位置(Python-like, 不包括)
 */
#define ulist_foreach_to(list, type, var, to_index) \
  for (type* var = (type*)(list)->data;             \
       var < (type*)ulist_get(list, to_index); var++)

/**
 * @brief 循环遍历列表
 * @param  list       列表结构体
 * @param  type       元素类型
 * @param  var        循环变量名
 * @param  from_index 起始位置
 * @param  to_index   结束位置(Python-like, 不包括)
 */
#define ulist_foreach_from_to(list, type, var, from_index, to_index) \
  for (type* var = (type*)ulist_get(list, from_index);               \
       var < (type*)ulist_get(list, to_index); var++)

#ifdef __cplusplus
}
#endif
#endif  // __ULIST_H__

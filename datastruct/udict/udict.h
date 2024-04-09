/**
 * @file udict.h
 * @brief 数据类型通用的动态字典实现
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-01-23
 *
 * THINK DIFFERENTLY
 */

#ifndef __UDICT_H__
#define __UDICT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"
#include "uthash.h"

typedef struct udict_node {
  UT_hash_handle hh;  // hashable
  bool is_ptr;
  mod_size_t size;
  const char* key;
  uint8_t value[];
} udict_node_t;

typedef struct udict {
  udict_node_t* nodes;
  mod_size_t size;
  MOD_MUTEX_HANDLE mutex;  // 互斥锁
  bool dyn;
} udict_t;
typedef udict_t* UDICT;

/**
 * @brief 初始化一个已创建的字典
 * @param dict 字典
 *
 * @return 是否成功
 */
extern bool udict_init(UDICT dict);

/**
 * @brief 创建一个字典并初始化
 *
 * @return 字典，如果失败则为NULL
 */
extern UDICT udict_new(void);

/**
 * @brief 清空字典
 * @param dict 字典
 */
extern void udict_clear(UDICT dict);

/**
 * @brief 释放字典
 * @param dict 字典
 */
extern void udict_free(UDICT dict);

/**
 * @brief 获取字典大小
 * @param dict 字典
 *
 * @return 字典大小
 */
static inline mod_size_t udict_len(UDICT dict) { return dict->size; }

/**
 * @brief 检查字典是否包含某个键
 * @param  dict     字典
 * @param  key      键
 * @retval true     包含
 */
extern bool udict_has_key(UDICT dict, const char* key);

/**
 * @brief 获取字典中的值
 * @param  dict     字典
 * @param  key      键
 * @retval value    值，如果不存在则为NULL
 */
extern void* udict_get(UDICT dict, const char* key);

/**
 * @brief 获取字典中的键
 * @param  dict     字典
 * @param  value    值
 * @retval key      键，如果不存在则为NULL
 */
extern const char* udict_get_reverse(UDICT dict, void* value);

/**
 * @brief 设置字典中的值
 * @param  dict     字典
 * @param  key      键
 * @param  value    值
 * @retval true     成功
 */
extern bool udict_set(UDICT dict, const char* key, void* value);

/**
 * @brief 从缓冲区复制数据以设置字典中的值
 * @param  dict     字典
 * @param  key      键
 * @param  value    值
 * @param  size     大小
 * @retval true     成功
 */
extern bool udict_set_copy(UDICT dict, const char* key, void* value,
                           size_t size);

/**
 * @brief 分配一片新内存给指定的值并返回
 * @param  dict     字典
 * @param  key      键
 * @param  size     大小
 * @retval ptr      值指针，如果失败则为NULL
 */
extern void* udict_set_alloc(UDICT dict, const char* key, size_t size);

/**
 * @brief 删除字典中的项目
 * @param  dict     字典
 * @param  key      键
 * @retval true     成功
 */
extern bool udict_del(UDICT dict, const char* key);

/**
 * @brief 弹出字典中的项目(动态分配内存的项目需要手动释放)
 * @param  dict         字典
 * @param  key          键
 * @retval value        值，如果不存在则为NULL
 */
extern void* udict_pop(UDICT dict, const char* key);

/**
 * @brief 循环迭代字典
 * @param  dict         字典
 * @param  key          键
 * @param  value        值
 * @retval true         继续迭代
 * @warning key必须初始化为NULL
 */
extern bool udict_iter(UDICT dict, const char** key, void** value);

/**
 * @brief 打印字典
 * @param  dict         字典
 * @param  name         名称
 */
extern void udict_print(UDICT dict, const char* name);

#ifdef __cplusplus
}
#endif

#endif  // __UDICT_H__

#ifndef __MF_H__
#define __MF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(1)
typedef struct {
  uint8_t next_key;
  uint32_t name_length : 24;
  uint32_t data_size;
} mf_key_info_t;

typedef struct {
  uint32_t header;
  mf_key_info_t key;
} mf_flash_info_t;
#pragma pack()

typedef enum {
  MF_OK = 0,          // 操作成功
  MF_ERR = -1,        // 未知错误
  MF_ERR_FULL = -2,   // 数据库已满
  MF_ERR_NULL = -3,   // 键值不存在
  MF_ERR_EXIST = -4,  // 键值已存在
  MF_ERR_SIZE = -5,   // 数据大小不匹配
} mf_status_t;

/**
 * @brief 初始化MiniFlashDB
 */
void mf_init(void);

/**
 * @brief 执行数据库保存
 */
void mf_save(void);

/**
 * @brief 重载数据库, 丢弃所有未保存的更改
 */
void mf_load(void);

/**
 * @brief 执行数据库清空
 * @warning 会同时清空备份区，危险操作
 */
void mf_purge(void);

/**
 * @brief 获取数据库键值数量
 * @retval 数据库键值数量
 */
size_t mf_len(void);

/**
 * @brief 搜索键值
 * @param  name 键值名称
 * @retval 键值信息指针
 * @note   如果键值不存在，则返回NULL
 */
mf_key_info_t *mf_search_key(const char *name);

/**
 * @brief 添加新的键值
 * @param  name 键值名称
 * @param  data 键值数据指针
 * @param  size 数据大小
 * @retval 操作结果
 * @note   如果键值已存在，则抛出MF_ERR_EXIST错误
 * @note   如果数据库已满，则抛出MF_ERR_FULL错误
 */
mf_status_t mf_add_key(const char *name, const void *data, size_t size);

/**
 * @brief 删除已有键值
 * @param  name 键值名称
 * @retval 操作结果
 * @note   如果键值不存在，则抛出MF_ERR_NULL错误
 */
mf_status_t mf_del_key(const char *name);

/**
 * @brief 修改已有键值的数据
 * @param  name 键值名称
 * @param  data 键值数据指针
 * @param  size 数据大小
 * @retval 操作结果
 * @note   如果键值不存在，则抛出MF_ERR_NULL错误
 * @note   如果数据大小与当前键值不匹配，则抛出MF_ERR_SIZE错误
 */
mf_status_t mf_modify_key(const char *name, const void *data, size_t size);

/**
 * @brief 设置键值数据
 * @note  如果键值不存在则添加, 否则修改, 数据大小不匹配则重新分配
 * @param  name 键值名称
 * @param  data 键值数据指针
 * @param  size 数据大小
 * @retval 操作结果
 */
mf_status_t mf_set_key(const char *name, const void *data, size_t size);

/**
 * @brief 获取键值对应的名称指针
 * @param  key 键值信息指针
 * @retval 键值名称指针
 */
const char *mf_get_key_name(mf_key_info_t *key);

/**
 * @brief 获取键值对应的数据指针
 * @param  key 键值信息指针
 * @retval 键值数据指针
 */
const void *mf_get_key_data(mf_key_info_t *key);

/**
 * @brief 获取键值对应的数据大小
 * @param  key 键值信息指针
 * @retval 数据大小
 */
size_t mf_get_key_data_size(mf_key_info_t *key);

/**
 * @brief 内部遍历数据库
 * @param  fun 遍历函数(传入键值信息指针和操作参数, 返回是否继续遍历)
 * @param  arg 操作参数
 */
void mf_foreach(bool (*fun)(mf_key_info_t *key, void *arg), void *arg);

/**
 * @brief 循环遍历数据库
 * @param[out]  key 键值信息指针的指针(必须初始化为NULL)
 * @param[out]  name 键值名称指针的指针(可选)
 * @param[out]  value 键值数据指针的指针(可选)
 * @param[out]  data_size 数据大小的指针(可选)
 * @retval 是否可以继续遍历
 */
bool mf_iter(mf_key_info_t **key, const char **name, const void **value,
             size_t *data_size);

#ifdef __cplusplus
}
#endif

#endif

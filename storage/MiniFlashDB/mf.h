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

#define MF_FLASH_HEADER 0x3366CCFF

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

typedef enum {
  MF_OK,
  MF_ERR,
  MF_ERR_FULL,
  MF_ERR_NULL,
  MF_ERR_EXIST
} mf_status_t;
#pragma pack()

/* 初始化MiniFlashDB */
void mf_init();

/* 执行数据库保存 */
void mf_save();

/* 执行数据库读取 */
void mf_load();

/* 执行数据库清空(会同时清空备份区，危险操作) */
void mf_purge();

/* 查找键值信息 */
mf_key_info_t *mf_search_key(const char *name);

/* 添加键值 */
mf_status_t mf_add_key(const char *name, const void *data, size_t size);

/* 删除键值 */
mf_status_t mf_del_key(const char *name);

/* 设置键值数据 */
mf_status_t mf_set_key(const char *name, const void *data, size_t size);

/* 获取键值数据 */
uint8_t *mf_get_key_data(mf_key_info_t *key);

/* 获取键值名称 */
const char *mf_get_key_name(mf_key_info_t *key);

/* 遍历数据库 */
void mf_foreach(bool (*fun)(mf_key_info_t *key, void *arg), void *arg);

#ifdef __cplusplus
}
#endif

#endif

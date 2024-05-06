#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "main.h"

/* 一块FLASH空间的大小 */
#define MF_FLASH_BLOCK_SIZE (2048)

/* 主FLASH地址 */
#define MF_FLASH_MAIN_ADDR (0)

/* 备份FLASH地址，注释则不使用 */
#define MF_FLASH_BACKUP_ADDR (0)

/* FLASH空数据填充值 */
#define MF_FLASH_FILL 0xFF

/* Flash读写函数 */

/**
 * @brief 从addr开始，擦除长度为MF_FLASH_BLOCK_SIZE的flash
 * @param addr 起始地址
 * @retval 操作结果 true: 成功, false: 失败
 * @note   实际入参只可能是MF_FLASH_MAIN_ADDR或MF_FLASH_BACKUP_ADDR，可以考虑简化
 */
__attribute__((unused)) static bool mf_erase(uint32_t addr) {
    //
    return true;
}

/**
 * @brief 从addr开始，写入MF_FLASH_BLOCK_SIZE大小的数据
 * @param addr 起始地址
 * @param buf 数据指针
 * @retval 操作结果 true: 成功, false: 失败
 * @note   实际入参只可能是MF_FLASH_MAIN_ADDR或MF_FLASH_BACKUP_ADDR，可以考虑简化
 */
__attribute__((unused)) static bool mf_write(uint32_t addr, void* buf) {
    //
    return true;
}

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

/* 从addr开始，擦除长度为MF_FLASH_BLOCK_SIZE的flash */
__attribute__((unused)) static void mf_erase(uint32_t addr) {
  // 实际入参只可能是MF_FLASH_MAIN_ADDR或MF_FLASH_BACKUP_ADDR，可以考虑简化
}

/* 从addr开始，把buf写入长度为MF_FLASH_BLOCK_SIZE的flash */
__attribute__((unused)) static void mf_write(uint32_t addr, void *buf) {
  // 同理，也可以考虑简化
}

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ee_internal.h"

#define LOG_MODULE "mf"
// #define LOG_LEVEL LOG_LEVEL_DEBUG
#include "log.h"
#include "main.h"

/* 一块FLASH空间的大小 */
#define MF_FLASH_BLOCK_SIZE (EE_SIZE / 2)

/* 主FLASH地址 */
#define MF_FLASH_MAIN_ADDR EE_ADDRESS(0)  // 倒数第一个page

/* 备份FLASH地址，注释则不使用 */
#define MF_FLASH_BACKUP_ADDR EE_ADDRESS(1)  // 倒数第二个page

/* FLASH空数据填充值 */
#define MF_FLASH_FILL 0xFF

/* Flash读写函数 */
__attribute__((unused)) static void mf_erase(uint32_t addr) {
  uint8_t offset;
  for (offset = 0;; offset++) {
    if (offset >= EE_PAGE_NUMBER) {
      LOG_ERROR("failed find page %p", addr);
      return;
    }
    if (addr == (uint32_t)EE_PageAddress(offset)) {
      break;
    }
  }
  LOG_DEBUG("erasing offset %d, address=%X", offset, addr);
  if (!EE_Erase(offset)) {
    LOG_ERROR("erase failed at %d", offset);
  }
}

__attribute__((unused)) static void mf_write(uint32_t addr, void* buf) {
  uint8_t offset;
  for (offset = 0;; offset++) {
    if (offset >= EE_PAGE_NUMBER) {
      LOG_ERROR("failed find page %p", addr);
      return;
    }
    if (addr == (uint32_t)EE_PageAddress(offset)) {
      break;
    }
  }
  LOG_DEBUG("writing offset %d, address=%X, size=%d", offset, addr, EE_SIZE);
  if (!EE_Write(offset, buf, EE_SIZE, false)) {
    LOG_ERROR("write failed at %d", offset);
  }
}

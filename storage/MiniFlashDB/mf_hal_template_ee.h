#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ee_internal.h"
#define LOG_MODULE "mf"
#include "log.h"
#include "main.h"

/* 一块FLASH空间的大小 */
#define MF_FLASH_BLOCK_SIZE (EE_SIZE / 2)

/* 主FLASH地址 */
#define MF_FLASH_MAIN_ADDR EE_ADDRESS(0)

/* 备份FLASH地址，注释则不使用 */
#define MF_FLASH_BACKUP_ADDR EE_ADDRESS(1)

/* Flash读写函数 */
__attribute__((unused)) static void mf_erase(uint32_t addr) {
  uint8_t offset;
  for (offset = 0;; offset++) {
    if (offset >= EE_PAGE_NUMBER) {
      LOG_ERROR("EE_Page failed find %p", addr);
      return;
    }
    if (addr == (uint32_t)EE_PageAddress(offset)) {
      break;
    }
  }
  if (!EE_Erase(offset)) {
    LOG_ERROR("EE_Erase failed at %d", offset);
  }
}

__attribute__((unused)) static void mf_write(uint32_t addr, void* buf) {
  uint8_t offset;
  for (offset = 0;; offset++) {
    if (offset >= EE_PAGE_NUMBER) {
      LOG_ERROR("EE_Page failed find %p", addr);
      return;
    }
    if (addr == (uint32_t)EE_PageAddress(offset)) {
      break;
    }
  }
  if (!EE_Write(offset, buf, EE_SIZE, false)) {
    LOG_ERROR("EE_Write failed at %d", offset);
  }
}

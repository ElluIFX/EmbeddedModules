/**
 * @file spif_port_lfs.h
 * @brief SPI Flash文件系统接口
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-03-11
 *
 * THINK DIFFERENTLY
 */

#ifndef __SPIF_PORT_LFS_H__
#define __SPIF_PORT_LFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lfs.h"
#include "log.h"
#include "spif.h"

SPIF_HandleTypeDef hspif;
lfs_t lfs;

static int spif_block_device_read(const struct lfs_config *c, lfs_block_t block,
                                  lfs_off_t off, void *buffer,
                                  lfs_size_t size) {
  if (!SPIF_ReadAddress(&hspif, block * c->block_size + off, (uint8_t *)buffer,
                        size)) {
    LOG_ERROR("SPIF Read Failed (%d, %d, %d)", block, off, size);
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

static int spif_block_device_prog(const struct lfs_config *c, lfs_block_t block,
                                  lfs_off_t off, const void *buffer,
                                  lfs_size_t size) {
  if (!SPIF_WriteAddress(&hspif, block * c->block_size + off, (uint8_t *)buffer,
                         size)) {
    LOG_ERROR("SPIF Write Failed (%d, %d, %d)", block, off, size);
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

static int spif_block_device_erase(const struct lfs_config *c,
                                   lfs_block_t block) {
  if (!SPIF_EraseBlock(&hspif, block)) {
    LOG_ERROR("SPIF Erase Failed (%d)", block);
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

static int spif_block_device_sync(const struct lfs_config *c) {
  return LFS_ERR_OK;
}

#if LFS_THREADSAFE
static MOD_MUTEX_HANDLE lfs_mutex;

static int sh_lfslock(const struct lfs_config *c) {
  MOD_MUTEX_ACQUIRE(lfs_mutex);
  return 0;
}

static int sh_lfsunlock(const struct lfs_config *c) {
  MOD_MUTEX_RELEASE(lfs_mutex);
  return 0;
}
#endif

static struct lfs_config cfg = {
    // block device operations
    .read = spif_block_device_read,
    .prog = spif_block_device_prog,
    .erase = spif_block_device_erase,
    .sync = spif_block_device_sync,
#if LFS_THREADSAFE
    .lock = sh_lfslock,
    .unlock = sh_lfsunlock,
#endif

    // block device configuration
    .block_size = 4096,
    .block_count = 1024,

    .read_size = 16,
    .prog_size = 16,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};

static uint8_t spif_init_lfs(void) {
#if LFS_THREADSAFE
  lfs_mutex = MOD_MUTEX_CREATE("lfs");
#endif
  if (!SPIF_Init(&hspif, &FLASH_HSPI_HANDLE, FLASH_CS_GPIO_Port,
                 FLASH_CS_Pin)) {
    LOG_ERROR("SPIF Driver Init Failed");
    return 0;
  }
  LOG_PASS("SPIF Driver Initialized");
  // LOG_DEBUG("SPIF Block=%d, Page=%d, Sector=%d", hspif.BlockCnt, hspif.PageCnt,
            // hspif.SectorCnt);
  cfg.block_size = SPIF_BLOCK_SIZE;
  cfg.block_count = hspif.BlockCnt;
  int err;
  if ((err = lfs_mount(&lfs, &cfg)) != LFS_ERR_OK) {
    LOG_ERROR("lfs_mount failed: %d, formatting", err);
    if ((err = lfs_format(&lfs, &cfg)) != LFS_ERR_OK) {
      LOG_ERROR("lfs_format failed: %d", err);
      return 0;
    }
    LOG_PASS("LittleFS Formatted");
    if ((err = lfs_mount(&lfs, &cfg)) != LFS_ERR_OK) {
      LOG_ERROR("lfs_mount failed again: %d", err);
      return 0;
    }
  }
  LOG_PASS("LittleFS Mounted");
  return 1;
}

#ifdef __cplusplus
}
#endif

#else
#error "Multiple inclusion of spif_port_lfs.h"
#endif /* __SPIF_PORT_LFS__ */

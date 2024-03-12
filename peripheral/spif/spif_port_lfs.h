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

#include "led.h"
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
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

static int spif_block_device_prog(const struct lfs_config *c, lfs_block_t block,
                                  lfs_off_t off, const void *buffer,
                                  lfs_size_t size) {
  if (!SPIF_WriteAddress(&hspif, block * c->block_size + off, (uint8_t *)buffer,
                         size)) {
    return LFS_ERR_IO;
  }
  return LFS_ERR_OK;
}

static int spif_block_device_erase(const struct lfs_config *c,
                                   lfs_block_t block) {
  if (!SPIF_EraseBlock(&hspif, block)) {
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
  // kernel_enter_critical();
  return 0;
}

static int sh_lfsunlock(const struct lfs_config *c) {
  MOD_MUTEX_RELEASE(lfs_mutex);
  // kernel_exit_critical();
  return 0;
}
#endif

static const struct lfs_config cfg = {
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
    .read_size = 16,
    .prog_size = 16,
    .block_size = 4096,
    .block_count = 128,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};

static bool spif_init_lfs(void) {
#if LFS_THREADSAFE
  lfs_mutex = MOD_MUTEX_CREATE();
#endif
  if (!SPIF_Init(&hspif, &hspi2, FLASH_CS_GPIO_Port, FLASH_CS_Pin)) {
    LOG_ERROR("SPIF Driver Init Failed");
    return false;
  }
  LOG_PASS("SPIF Driver Initialized");
  int err = lfs_mount(&lfs, &cfg);
  if (err) {
    LOG_ERROR("lfs_mount failed: %d, formatting", err);
    lfs_format(&lfs, &cfg);
    err = lfs_mount(&lfs, &cfg);
    if (err) {
      LOG_ERROR("lfs_mount failed again: %d", err);
      return false;
    }
  }
  LOG_PASS("LittleFS Mounted");
  return true;
}

#ifdef __cplusplus
}
#endif

#else
#error "Multiple inclusion of spif_port_lfs.h"
#endif /* __SPIF_PORT_LFS__ */

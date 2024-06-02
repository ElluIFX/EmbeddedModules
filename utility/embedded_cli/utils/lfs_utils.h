/**
 * @file lfs_utils.h
 * @brief LFS文件系统命令行工具
 * @author Ellu (ellu.grif@gmail.com)
 * @date 2024-03-11
 *
 * THINK DIFFERENTLY
 */

#ifndef __LFS_UTILS_H__
#define __LFS_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "embedded_cli.h"
#include "lfs.h"
#include "modules.h"

// Public Defines ---------------------------

// Public Typedefs --------------------------

// Public Macros ----------------------------

// Exported Variables -----------------------

// Exported Functions -----------------------

extern void lfs_utils_add_command_to_cli(EmbeddedCli* cli, lfs_t* lfs);

#ifdef __cplusplus
}
#endif

#endif /* __LFS_UTILS__ */

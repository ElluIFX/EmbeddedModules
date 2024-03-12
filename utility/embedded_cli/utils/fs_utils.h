/**
 * @file fs_utils.h
 * @brief 文件系统命令行工具
 * @author Ellu (ellu.grif@gmail.com)
 * @date 2024-03-11
 *
 * THINK DIFFERENTLY
 */

#ifndef __FS_UTILS_H__
#define __FS_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "embedded_cli.h"
#include "lfs.h"
#include "modules.h"

// Public Defines ---------------------------

#define FS_ENABLE_YMODEM 1  // 启用YMODEM接收命令
#define FS_ENABLE_DEBUG 0  // 启用调试信息

// Public Typedefs --------------------------

// Public Macros ----------------------------

// Exported Variables -----------------------

// Exported Functions -----------------------

extern void FSUtils_AddCmdToCli(EmbeddedCli *cli, lfs_t *lfs);

#ifdef __cplusplus
}
#endif

#endif /* __FS_UTILS__ */

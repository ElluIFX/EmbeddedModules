/**
 * @file system_utils.h
 * @brief 系统命令行工具
 * @author Ellu (ellu.grif@gmail.com)
 * @date 2024-03-02
 *
 * THINK DIFFERENTLY
 */

#ifndef __SYSTEM_UTILS_H__
#define __SYSTEM_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "embedded_cli.h"
#include "modules.h"

// Public Defines ---------------------------

// Public Typedefs --------------------------

// Public Macros ----------------------------

// Exported Variables -----------------------

// Exported Functions -----------------------

extern void SystemUtils_AddCmdToCli(EmbeddedCli *cli);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_UTILS__ */

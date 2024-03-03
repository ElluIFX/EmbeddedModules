/**
 * @file cli_utils.h
 * @brief 终端命令集
 * @author Ellu (ellu.grif@gmail.com)
 * @date 2024-03-02
 *
 * THINK DIFFERENTLY
 */

#ifndef __CLI_UTILS_H__
#define __CLI_UTILS_H__

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

extern void CliUtils_AddCmdToCli(EmbeddedCli *cli);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_UTILS__ */

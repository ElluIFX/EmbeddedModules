#ifndef _SCHEDULER_CONF_H_
#define _SCHEDULER_CONF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

#ifndef SCH_CFG_ENABLE_TASK
#define SCH_CFG_ENABLE_TASK 1  // 支持任务
#endif

#ifndef SCH_CFG_ENABLE_EVENT
#define SCH_CFG_ENABLE_EVENT 1  // 支持事件
#endif

#ifndef SCH_CFG_ENABLE_COROUTINE
#define SCH_CFG_ENABLE_COROUTINE 1  // 支持宏协程
#endif

#ifndef SCH_CFG_ENABLE_CALLLATER
#define SCH_CFG_ENABLE_CALLLATER 1  // 支持延时调用
#endif

#ifndef SCH_CFG_ENABLE_SOFTINT
#define SCH_CFG_ENABLE_SOFTINT 1  // 支持软中断
#endif

#ifndef SCH_CFG_COMP_RANGE_US
#define SCH_CFG_COMP_RANGE_US 1000  // 任务调度自动补偿范围(us)
#endif

#ifndef SCH_CFG_STATIC_NAME
#define SCH_CFG_STATIC_NAME 1       // 是否使用静态标识名
#endif

#ifndef SCH_CFG_STATIC_NAME_LEN
#define SCH_CFG_STATIC_NAME_LEN 16  // 静态标识名长度
#endif

#ifndef SCH_CFG_DEBUG_REPORT
#define SCH_CFG_DEBUG_REPORT 0  // 输出调度器统计信息(调试模式/低性能)
#endif

#ifndef SCH_CFG_DEBUG_PERIOD
#define SCH_CFG_DEBUG_PERIOD 5  // 调试报告打印周期(s)(超过10s的值可能导致溢出)
#endif

#ifndef SCH_CFG_DEBUG_MAXLINE
#define SCH_CFG_DEBUG_MAXLINE 10  // 调试报告最大行数
#endif

#ifndef SCH_CFG_ENABLE_TERMINAL
#define SCH_CFG_ENABLE_TERMINAL 0  // 是否启用终端命令集(依赖embedded-cli)
#endif

#ifdef __cplusplus
}
#endif

#endif  // _SCHEDULER_CONF_H_

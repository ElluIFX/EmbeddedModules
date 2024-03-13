/**
 * @file scheduler.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2021-12-11
 *
 * THINK DIFFERENTLY
 */

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "modules.h"
#include "scheduler_calllater.h"
#include "scheduler_coroutine.h"
#include "scheduler_event.h"
#include "scheduler_softint.h"
#include "scheduler_task.h"

#if !KCONFIG_AVAILABLE  // 由Kconfig配置
/******************************调度器设置******************************/
#define SCH_CFG_ENABLE_TASK 1       // 支持任务
#define SCH_CFG_ENABLE_EVENT 1      // 支持事件
#define SCH_CFG_ENABLE_COROUTINE 1  // 支持宏协程
#define SCH_CFG_ENABLE_CALLLATER 1  // 支持延时调用
#define SCH_CFG_ENABLE_SOFTINT 1    // 支持软中断

#define SCH_CFG_COMP_RANGE_US 1000  // 任务调度自动补偿范围(us)
#define SCH_CFG_STATIC_NAME 1       // 是否使用静态标识名
#define SCH_CFG_STATIC_NAME_LEN 16  // 静态标识名长度
#define SCH_CFG_PRI_ORDER_ASC 1  // 优先级升序排序(升序:值大的优先级高)

#define SCH_CFG_DEBUG_REPORT 1  // 输出调度器统计信息(调试模式/低性能)
#define SCH_CFG_DEBUG_PERIOD 5  // 调试报告打印周期(s)(超过10s的值可能导致溢出)
#define SCH_CFG_DEBUG_MAXLINE 10  // 调试报告最大行数

#define SCH_CFG_ENABLE_TERMINAL 1  // 是否启用终端命令集(依赖embedded-cli)

#endif  // KCONFIG_AVAILABLE

/**
 * @brief 调度器主函数
 * @param  block            是否阻塞, 若不阻塞则应将此函数放在SuperLoop中
 * @retval uint64_t         返回时间: 距离下一次调度的时间(us)
 * @note block=0时. SuperLoop应保证在返回时间前交还CPU以最小化调度延迟
 * @note block=1时. 查看Scheduler_Idle_Callback函数说明
 **/
extern uint64_t Scheduler_Run(const uint8_t block);

/**
 * @brief 调度器空闲回调函数, 由用户实现(block=1时调用)
 * @param  idleTimeUs      空闲时间(us)
 * @note  应保证在空闲时间前交还CPU以最小化调度延迟
 * @note  可在此函数内实现低功耗
 */
extern void Scheduler_Idle_Callback(uint64_t idleTimeUs);

#if SCH_CFG_ENABLE_TERMINAL
#include "embedded_cli.h"
/**
 * @brief 添加调度器相关的终端命令(task/event/cortn/softint)
 */
extern void Sch_AddCmdToCli(EmbeddedCli *cli);
#endif  // SCH_CFG_ENABLE_TERMINAL

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_H_

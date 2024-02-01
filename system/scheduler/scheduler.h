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
#include "scheduler_conf.h"

#include "scheduler_calllater.h"
#include "scheduler_coroutine.h"
#include "scheduler_event.h"
#include "scheduler_softint.h"
#include "scheduler_task.h"

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

#if _SCH_ENABLE_TERMINAL
#include "embedded_cli.h"
/**
 * @brief 添加调度器相关的终端命令(task/event/cortn/softint)
 */
extern void Sch_AddCmdToCli(EmbeddedCli *cli);
#endif  // _SCH_ENABLE_TERMINAL

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_H_

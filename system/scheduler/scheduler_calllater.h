#ifndef _SCHEDULER_CALLLATER_H
#define _SCHEDULER_CALLLATER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"

#if _SCH_ENABLE_CALLLATER

typedef void (*cl_func_t)(void *args);

/**
 * @brief 在指定时间后执行目标函数
 * @param  func             任务函数指针
 * @param  delayUs          延时启动时间(us)
 * @param  args             任务参数
 * @retval uint8_t          是否成功
 */
extern uint8_t Sch_CallLater(cl_func_t func, uint64_t delayUs, void *args);

/**
 * @brief 取消所有对应函数的延时调用任务
 * @param func              任务函数指针
 */
extern void Sch_CancelCallLater(cl_func_t func);
#endif  // _SCH_ENABLE_CALLLATER
#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_CALLLATER_H

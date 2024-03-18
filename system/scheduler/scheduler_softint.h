#ifndef _SCHEDULER_SOFTINT_H
#define _SCHEDULER_SOFTINT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"

#if SCH_CFG_ENABLE_SOFTINT

/**
 * @brief 触发软中断
 * @param  mainChannel     主通道(0-7)
 * @param  subChannel      子通道(0-7)
 */
extern void sch_trigger_soft_int(uint8_t mainChannel, uint8_t subChannel);

/**
 * @brief 软中断回调函数
 * @param  mainChannel    主通道(0-7)
 * @param  subMask        8个子通道掩码(1 << subChannel)
 * @note  调度器自动调用, 由用户实现
 */
extern void scheduler_soft_int_handler(uint8_t mainChannel, uint8_t subMask);

#endif  // SCH_CFG_ENABLE_SOFTINT

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_SOFTINT_H

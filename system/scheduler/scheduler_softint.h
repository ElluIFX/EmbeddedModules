#ifndef _SCHEDULER_SOFTINT_H
#define _SCHEDULER_SOFTINT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"

#if SCH_CFG_ENABLE_SOFTINT

/**
 * @brief 触发软中断
 * @param  main_channel     主通道(0-7)
 * @param  sub_channel      子通道(0-7)
 */
extern void sch_softint_trigger(uint8_t main_channel, uint8_t sub_channel);

/**
 * @brief 软中断回调函数
 * @param  main_channel    主通道(0-7)
 * @param  sub_channel_mask        8个子通道掩码(1 << sub_channel)
 * @note  调度器自动调用, 由用户实现
 */
extern void scheduler_softint_handler(uint8_t main_channel,
                                      uint8_t sub_channel_mask);

#endif  // SCH_CFG_ENABLE_SOFTINT

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_SOFTINT_H

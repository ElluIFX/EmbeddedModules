/**
 * @file scheduler_lite.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-05-08
 *
 * THINK DIFFERENTLY
 */

#ifndef __SCHEDULER_LITE_H__
#define __SCHEDULER_LITE_H__
#include "modules.h"

typedef struct {       // 用户任务结构
  void (*task)(void);  // 任务函数指针
  uint32_t periodMs;   // 任务调度周期(ms)
  m_time_t lastRun;    // 上次执行时间(Tick)
} scheduler_task_t;

#define __SCH_SECTION(x) __attribute__((section(".scheduler_list.s" x)))

/**
 * @brief 声明一个调度器任务
 * @param func 任务函数
 * @param periodMs 任务调度周期(ms)
 */
#define SCH_TASK(func, periodMs)                                              \
  __attribute__((used)) scheduler_task_t _sch_task_item_##func __SCH_SECTION( \
      "1") = {func, periodMs, 0}

extern void Scheduler_Run(const uint8_t block);

#endif  // __SCHEDULER_LITE_H__

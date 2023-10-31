/**
 * @file scheduler_lite.c
 * @brief 精简版时分调度器
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-05-08
 *
 * THINK DIFFERENTLY
 */

#include "scheduler_lite.h"
__attribute__((used))
scheduler_task_t _sch_task_start_ __SCH_SECTION("0.end") = {NULL, 0, 0, 0};
__attribute__((used))
scheduler_task_t _sch_task_end_ __SCH_SECTION("1.end") = {NULL, 0, 0, 0};

scheduler_task_t *schTaskList = NULL;

__attribute__((always_inline)) void Scheduler_Run(const uint8_t block) {
  m_time_t now;
  m_time_t period;
  schTaskList = (scheduler_task_t *)(&_sch_task_start_ + 1);
  do {
    for (uint16_t i = 0; schTaskList[i].task != NULL; i++) {
      now = m_tick();
      period = m_tick_clk / 1000 * schTaskList[i].periodMs;
      if ((now >= schTaskList[i].lastRun + period)) {
        schTaskList[i].task();
        if (now - (schTaskList[i].lastRun + period) < _SCH_COMP_RANGE)
          schTaskList[i].lastRun += period;
        else
          schTaskList[i].lastRun = now;
      }
    }
  } while (block);
}

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
scheduler_task_t _sch_task_start_ __SCH_SECTION("0.end") = {NULL, 0, 0};
__attribute__((used))
scheduler_task_t _sch_task_end_ __SCH_SECTION("1.end") = {NULL, 0, 0};

__attribute__((always_inline)) void SchedulerLite_Run(const uint8_t block) {
  static uint8_t inited = 0;
  static scheduler_task_t *schTaskList = NULL;
  m_time_t now;
  if (!inited) {
    schTaskList = (scheduler_task_t *)(&_sch_task_start_ + 1);
    for (uint16_t i = 0; schTaskList[i].task != NULL; i++) {
      schTaskList[i].period =
          m_tick_clk(m_time_t) * schTaskList[i].period / 1000;
    }
    inited = 1;
  }
  do {
    for (uint16_t i = 0; schTaskList[i].task != NULL; i++) {
      now = m_tick();
      if ((now >= schTaskList[i].lastRun + schTaskList[i].period)) {
        schTaskList[i].task();
        if (now - (schTaskList[i].lastRun + schTaskList[i].period) <
            _SCH_COMP_RANGE_US * m_tick_per_us(m_time_t))
          schTaskList[i].lastRun += schTaskList[i].period;
        else
          schTaskList[i].lastRun = now;
      }
    }
  } while (block);
}

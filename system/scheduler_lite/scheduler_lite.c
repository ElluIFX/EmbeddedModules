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
scheduler_task_t _sch_task_start_ _SCH_CFG_SECTION("0.end") = {NULL, 0, 0};
__attribute__((used))
scheduler_task_t _sch_task_end_ _SCH_CFG_SECTION("1.end") = {NULL, 0, 0};

__attribute__((always_inline)) void scheduler_lite_run(const uint8_t block) {
  static scheduler_task_t *schTaskList = NULL;
  m_time_t now;
  if (NULL == schTaskList) {
    schTaskList = (scheduler_task_t *)(&_sch_task_start_ + 1);
    for (uint16_t i = 0; schTaskList[i].task != NULL; i++) {
      schTaskList[i].period =
          (m_time_t)m_tick_clk * schTaskList[i].period / 1000;
    }
  }
  do {
    for (uint16_t i = 0; schTaskList[i].task != NULL; i++) {
      now = m_tick();
      if ((now >= schTaskList[i].lastRun + schTaskList[i].period)) {
        schTaskList[i].task();
        if (now - (schTaskList[i].lastRun + schTaskList[i].period) <
            m_tick_clk / 1000)
          schTaskList[i].lastRun += schTaskList[i].period;
        else
          schTaskList[i].lastRun = now;
      }
    }
  } while (block);
}

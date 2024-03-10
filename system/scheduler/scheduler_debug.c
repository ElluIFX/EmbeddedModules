#include "scheduler_internal.h"

#if SCH_CFG_DEBUG_REPORT
_INLINE uint8_t DebugInfo_Runner(uint64_t sleep_us) {
  static uint8_t first_print = 1;
  static uint64_t last_print = 0;
  static uint64_t sleep_sum = 0;
  static uint16_t sleep_cnt = 0;
  uint64_t now = get_sys_tick();
  if (!first_print) {  // 因为初始化耗时等原因，第一次的数据无参考价值，不打印
    if (now - last_print <= us_to_tick(SCH_CFG_DEBUG_PERIOD * 1000000)) {
      sleep_sum += sleep_us;
      sleep_cnt++;
      return 0;
    }
    uint64_t period = now - last_print;
    uint64_t other = period;
    TT tt = TT_NewTable(-1);
#if SCH_CFG_ENABLE_TASK
    sch_task_add_debug(tt, period, &other);
#endif  // SCH_CFG_ENABLE_TASK
#if SCH_CFG_ENABLE_EVENT
    sch_event_add_debug(tt, period, &other);
#endif  // SCH_CFG_ENABLE_EVENT
#if SCH_CFG_ENABLE_COROUTINE
    sch_cortn_add_debug(tt, period, &other);
#endif  // SCH_CFG_ENABLE_COROUTINE
    TT_AddTitle(
        tt,
        TT_Str(TT_ALIGN_LEFT, TT_FMT1_BLUE, TT_FMT2_BOLD, "[ Scheduler Info ]"),
        '-');
    TT_AddString(
        tt,
        TT_FmtStr(TT_ALIGN_CENTER, TT_FMT1_GREEN, TT_FMT2_NONE,
                  "Dur: %.3fs / Idle: %.2f%% / AvgSleep: %.3fus",
                  tick_to_us(now - last_print) / 1000000.0,
                  (float)other / period * 100, (float)sleep_sum / sleep_cnt),
        -1);
    sleep_sum = 0;
    sleep_cnt = 0;
    TT_AddSeparator(tt, TT_FMT1_BLUE, TT_FMT2_BOLD, '-');
    TT_LineBreak(tt, 1);
    TT_Print(tt);
    TT_FreeTable(tt);
  }
  now = get_sys_tick() - now;  // 补偿打印LOG的时间
#if SCH_CFG_ENABLE_TASK
  sch_task_finish_debug(first_print, now);
#endif  // SCH_CFG_ENABLE_TASK
#if SCH_CFG_ENABLE_EVENT
  sch_event_finish_debug(first_print, now);
#endif  // SCH_CFG_ENABLE_EVENT
#if SCH_CFG_ENABLE_COROUTINE
  sch_cortn_finish_debug(first_print, now);
#endif
  last_print = get_sys_tick();
  first_print = 0;
  return 1;
}
#endif  // SCH_CFG_DEBUG_REPORT

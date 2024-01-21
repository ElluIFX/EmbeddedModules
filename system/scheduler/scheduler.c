#include "scheduler_internal.h"

/**
 * @brief 利用Systick中断配合WFI实现低功耗延时
 * @param  us 延时时间(us)
 */
static void SysTick_Sleep(uint32_t us) {
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  uint32_t load = SysTick->LOAD;   // 保存原本的重装载值
  SysTick->LOAD = us_to_tick(us);  // 在指定时间后中断
  uint32_t val = SysTick->VAL;
  SysTick->VAL = 0;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  __wfi();  // 关闭CPU等待中断
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  SysTick->LOAD = load;  // 恢复重装载值
  SysTick->VAL = val;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

__weak void Scheduler_Idle_Callback(uint64_t idleTimeUs) {
  if (idleTimeUs > 1000) idleTimeUs = 1000;  // 最多休眠1ms以保证事件的及时响应
#if _MOD_USE_OS > 0
  m_delay_us(idleTimeUs);
#else  // 关闭CPU
  SysTick_Sleep(idleTimeUs);
#endif
}

_STATIC_INLINE uint8_t DebugInfo_Runner(uint64_t sleep_us);

uint64_t _INLINE Scheduler_Run(const uint8_t block) {
// #define CHECK(rslp, name) LOG_LIMIT(1000, #name " rslp=%d", rslp)
#define CHECK(rslp, name) ((void)0)
  uint64_t mslp, rslp;
  do {
    mslp = UINT64_MAX;
#if _SCH_ENABLE_SOFTINT
    SoftInt_Runner();
#endif
#if _SCH_ENABLE_TASK
    rslp = Task_Runner();
    CHECK(rslp, Task);
    if (rslp < mslp) mslp = rslp;
#endif
#if _SCH_ENABLE_COROUTINE
    rslp = Cortn_Runner();
    CHECK(rslp, Cortn);
    if (rslp < mslp) mslp = rslp;
#endif
#if _SCH_ENABLE_CALLLATER
    rslp = CallLater_Runner();
    CHECK(rslp, CallLater);
    if (rslp < mslp) mslp = rslp;
#endif
#if _SCH_ENABLE_EVENT
    Event_Runner();
#endif
    if (mslp == UINT64_MAX) mslp = 1000;  // 没有任何任务
#if _SCH_DEBUG_REPORT
    if (mslp && DebugInfo_Runner(mslp)) continue;
#endif
    if (block && mslp) {
      Scheduler_Idle_Callback(mslp);
    }
  } while (block);
  return mslp;
}

#if _SCH_DEBUG_REPORT
#warning Scheduler Debug-Report is on, expect performance degradation and increased memory usage of task handles

_STATIC_INLINE uint8_t DebugInfo_Runner(uint64_t sleep_us) {
  static uint8_t first_print = 1;
  static uint64_t last_print = 0;
  static uint64_t sleep_sum = 0;
  static uint16_t sleep_cnt = 0;
  uint64_t now = get_sys_tick();
  if (!first_print) {  // 因为初始化耗时等原因，第一次的数据无参考价值，不打印
    if (now - last_print <= us_to_tick(_SCH_DEBUG_PERIOD * 1000000)) {
      sleep_sum += sleep_us;
      sleep_cnt++;
      return 0;
    }
    uint64_t period = now - last_print;
    uint64_t other = period;
    TT tt = TT_NewTable(-1);
#if _SCH_ENABLE_TASK
    sch_task_add_debug(tt, period, &other);
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
    sch_event_add_debug(tt, period, &other);
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
    sch_cortn_add_debug(tt, period, &other);
#endif  // _SCH_ENABLE_COROUTINE
    TT_AddTitle(
        tt,
        TT_Str(TT_ALIGN_LEFT, TT_FMT1_BLUE, TT_FMT2_BOLD, "[ Scheduler Info ]"),
        '-');
    TT_AddString(
        tt,
        TT_FmtStr(TT_ALIGN_CENTER, TT_FMT1_GREEN, TT_FMT2_NONE,
                  "Run: %.3fs / Idle: %.2f%% / AvgSleep: %.3fus",
                  tick_to_us(now - last_print) / 1000000.0,
                  (float)other / period * 100, (float)sleep_sum / sleep_cnt),
        -1);
    sleep_sum = 0;
    sleep_cnt = 0;
    TT_AddSeparator(tt, TT_FMT1_BLUE, TT_FMT2_BOLD, '-');
    TT_LineBreak(tt, 1);
    TT_Print(tt);
    // TT_CursorBack(tt);
    TT_FreeTable(tt);
  }
  now = get_sys_tick() - now;  // 补偿打印LOG的时间
#if _SCH_ENABLE_TASK
  sch_task_finish_debug(first_print, now);
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
  sch_event_finish_debug(first_print, now);
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
  sch_cortn_finish_debug(first_print, now);
#endif
  last_print = get_sys_tick();
  first_print = 0;
  return 1;
}
#endif  // _SCH_DEBUG_REPORT

#if _SCH_ENABLE_TERMINAL
#include "lwmem.h"
#include "term_table.h"

static void sysinfo_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  TT tt = TT_NewTable(-1);
  TT_AddTitle(
      tt, TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ System Info ]"),
      '-');
  TT_ITEM_KVPAIR kv = TT_AddKVPair(tt, 0);
  TT_FMT1 f1 = TT_FMT1_GREEN;
  TT_FMT2 f2 = TT_FMT2_NONE;
  TT_ALIGN al = TT_ALIGN_LEFT;
  TT_STR sep = TT_Str(al, f1, f2, " : ");
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Core Clock"),
      TT_FmtStr(al, f1, f2, "%.3f Mhz", (float)get_sys_freq() / 1000000), sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "After Boot"),
      TT_FmtStr(al, f1, f2, "%.2fs", (float)get_sys_us() / 1000000), sep);
#if _MOD_USE_OS == 1  // klite
  static uint64_t last_kernel_tick = 0;
  static uint64_t last_idle_time = 0;
  uint64_t kernel_tick = kernel_tick_count64();
  uint64_t idle_time = kernel_idle_time();
  float usage =
      (float)((kernel_tick - last_kernel_tick) - (idle_time - last_idle_time)) /
      (float)(kernel_tick - last_kernel_tick);
  float usage_avg = (float)(kernel_tick - idle_time) / (float)kernel_tick;
  last_kernel_tick = kernel_tick;
  last_idle_time = idle_time;
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "RTOS Usage"),
                    TT_FmtStr(al, f1, f2, "%.2f%%", usage * 100), sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "RTOS Avg Usage"),
                    TT_FmtStr(al, f1, f2, "%.2f%%", usage_avg * 100), sep);
#endif
#if _SCH_ENABLE_TASK
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Task Num"),
                    TT_FmtStr(al, f1, f2, "%d", Sch_GetTaskNum()), sep);
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Event Num"),
                    TT_FmtStr(al, f1, f2, "%d", Sch_GetEventNum()), sep);
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Coroutine Num"),
                    TT_FmtStr(al, f1, f2, "%d", Sch_GetCortnNum()), sep);
#endif  // _SCH_ENABLE_COROUTINE
#if _MOD_HEAP_MATHOD == 1 || (_MOD_HEAP_MATHOD == 2 && HEAP_USE_LWMEM)
  TT_AddTitle(
      tt, TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ LwMem Info ]"),
      '-');
  kv = TT_AddKVPair(tt, 0);
  lwmem_stats_t stats;
  lwmem_get_stats(&stats);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Total"),
                    TT_FmtStr(al, f1, f2, "%d Bytes", stats.mem_size_bytes),
                    sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Avail"),
      TT_FmtStr(al, f1, f2, "%d Bytes (%.4f%%)", stats.mem_available_bytes,
                (float)(stats.mem_available_bytes) /
                    (float)stats.mem_size_bytes * 100),
      sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Min Avail"),
                    TT_FmtStr(al, f1, f2, "%d Bytes (%.4f%%)",
                              stats.minimum_ever_mem_available_bytes,
                              (float)(stats.minimum_ever_mem_available_bytes) /
                                  (float)stats.mem_size_bytes * 100),
                    sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Allocated"),
                    TT_FmtStr(al, f1, f2, "%d blocks", stats.nr_alloc), sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Freed"),
                    TT_FmtStr(al, f1, f2, "%d blocks", stats.nr_free), sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Alive"),
      TT_FmtStr(al, f1, f2, "%d blocks", stats.nr_alloc - stats.nr_free), sep);
#endif
  TT_AddSeparator(tt, TT_FMT1_GREEN, TT_FMT2_BOLD, '-');
  TT_Print(tt);
  TT_FreeTable(tt);
}

void Sch_AddCmdToCli(EmbeddedCli *cli) {
#if _SCH_ENABLE_TASK
  static CliCommandBinding sch_cmd = {
      .name = "task",
      .usage =
          "task [list|enable|disable|delete|setfreq|setpri|excute] "
          "[name] "
          "[freq|pri]",
      .help = "Task control command (Scheduler)",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = task_cmd_func,
  };

  embeddedCliAddBinding(cli, sch_cmd);
#endif  // _SCH_ENABLE_TASK

#if _SCH_ENABLE_EVENT
  static CliCommandBinding event_cmd = {
      .name = "event",
      .usage = "event [list|enable|disable|delete|trigger] [name]",
      .help = "Event control command (Scheduler)",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = event_cmd_func,
  };

  embeddedCliAddBinding(cli, event_cmd);
#endif  // _SCH_ENABLE_EVENT

#if _SCH_ENABLE_COROUTINE
  static CliCommandBinding cortn_cmd = {
      .name = "cortn",
      .usage = "cortn [list|enable|disable|delete] [name]",
      .help = "Coroutine control command (Scheduler)",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = cortn_cmd_func,
  };

  embeddedCliAddBinding(cli, cortn_cmd);
#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_SOFTINT
  static CliCommandBinding softint_cmd = {
      .name = "softint",
      .usage = "softint [channel] [sub-channel]",
      .help = "SoftInt manual trigger command (Scheduler)",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = softint_cmd_func,
  };

  embeddedCliAddBinding(cli, softint_cmd);
#endif  // _SCH_ENABLE_SOFTINT

  static CliCommandBinding sysinfo_cmd = {
      .name = "sysinfo",
      .usage = "sysinfo",
      .help = "Show system info (Scheduler)",
      .context = NULL,
      .autoTokenizeArgs = 0,
      .func = sysinfo_cmd_func,
  };

  embeddedCliAddBinding(cli, sysinfo_cmd);
}
#endif  // _SCH_ENABLE_TERMINAL

/**
 * @file cli_utils.c
 * @brief 终端命令集
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0.0
 * @date 2024-03-02
 *
 * THINK DIFFERENTLY
 */

#include "cli_utils.h"

#include "log.h"
#include "scheduler.h"
#include "term_table.h"
// Private Defines --------------------------

#if (MOD_CFG_HEAP_MATHOD_LWMEM || \
     (MOD_CFG_HEAP_MATHOD_KLITE && KERNEL_CFG_HEAP_USE_LWMEM))
#include "lwmem.h"
#define SHOWLWMEM 1
#elif (MOD_CFG_HEAP_MATHOD_HEAP4 || \
       (MOD_CFG_HEAP_MATHOD_KLITE && KERNEL_CFG_HEAP_USE_HEAP4))
#include "heap_4.h"
#define SHOWHEAP4 1
#elif (MOD_CFG_HEAP_MATHOD_RTT)
#define SHOWRTTHREAD 1
#endif
#if MOD_CFG_USE_OS_KLITE && KERNEL_CFG_HOOK_ENABLE
#include "kernel.h"
#define SHOWKLITE 1
#endif

// Private Typedefs -------------------------

// Private Macros ---------------------------

// Private Variables ------------------------

// Public Variables -------------------------

// Private Functions ------------------------
#if SHOWKLITE
#include "internal.h"
ULIST thread_list;

void kernel_hook_thread_create(thread_t thread) {
  if (!thread_list) {
    thread_list = ulist_new(sizeof(thread_t), 0, NULL, NULL);
  }
  ulist_append_copy(thread_list, &thread);
}

void kernel_hook_thread_delete(thread_t thread) {
  ulist_delete(thread_list, ulist_find(thread_list, &thread));
}

static void list_klite(void) {
  TT tt = TT_NewTable(-1);
  TT_FMT1 f1 = TT_FMT1_GREEN;
  TT_FMT2 f2 = TT_FMT2_BOLD;
  TT_ALIGN al = TT_ALIGN_LEFT;
  TT_AddTitle(
      tt, TT_FmtStr(al, f1, f2, "[ Thread List / %d ]", ulist_len(thread_list)),
      '-');
  TT_ITEM_GRID grid = TT_AddGrid(tt, 0);
  TT_ITEM_GRID_LINE line =
      TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
  const char *head[] = {"ID", "Pri", "Entry", "Usage", "Stack", "Free"};
  for (int i = 0; i < sizeof(head) / sizeof(char *); i++)
    TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head[i]));
  int i = 0;
  size_t sfree, ssize;
  f2 = TT_FMT2_NONE;
  ulist_foreach(thread_list, thread_t, thread_p) {
    line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " "));
    thread_t thread = *thread_p;
    double usage = (double)thread_time(thread) / (double)kernel_tick_count64();
    thread_stack_info(thread, &sfree, &ssize);
    TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
    TT_GridLine_AddItem(
        line, TT_FmtStr(al, f1, f2, "%d", thread_get_priority(thread)));
    TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%p", thread->entry));
    TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%.4f%%", usage * 100));
    TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", ssize));
    TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", sfree));
    i++;
  }
  TT_AddSeparator(tt, f1, f2, '-');
  TT_Print(tt);
  TT_FreeTable(tt);
}

static void klite_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (embeddedCliCheckToken(args, "-l", 1)) {
    list_klite();
    return;
  }
  if (argc < 2) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Thread ID is required" T_RST);
    return;
  }
  int id = atoi(embeddedCliGetToken(args, 2));
  thread_t *thread_p = (thread_t *)ulist_get(thread_list, id);
  if (!thread_p) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Thread ID not found: %d" T_RST, id);
    return;
  }
  if (embeddedCliCheckToken(args, "-s", 1)) {
    thread_suspend(*thread_p);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Thread %d suspended" T_RST, id);
  } else if (embeddedCliCheckToken(args, "-r", 1)) {
    thread_resume(*thread_p);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Thread %d resumed" T_RST, id);
  } else if (embeddedCliCheckToken(args, "-d", 1)) {
    thread_delete(*thread_p);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Thread %d deleted" T_RST, id);
  } else if (embeddedCliCheckToken(args, "-p", 1)) {
    if (argc < 3) {
      LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Priority is required" T_RST);
      return;
    }
    int pri = atoi(embeddedCliGetToken(args, 3));
    if (pri < 0 || pri >= __THREAD_PRIORITY_MAX__) {
      LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Priority must be 0-%d",
                __THREAD_PRIORITY_MAX__ - 1);
      return;
    }
    thread_set_priority(*thread_p, pri);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Thread %d priority set to %d" T_RST, id,
              pri);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Invalid command" T_RST);
  }
}
#endif

static void sysinfo_cmd_func(EmbeddedCli *cli, char *args, void *context) {
#if SHOWLWMEM
  lwmem_stats_t stats;
  lwmem_get_stats(&stats);
#elif SHOWHEAP4
  HeapStats_t stats;
  vPortGetHeapStats(&stats);
#elif SHOWRTTHREAD
  rt_uint32_t total = 0;
  rt_uint32_t used = 0;
  rt_uint32_t max_used = 0;
  rt_memory_info(&total, &used, &max_used);
#endif
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
      TT_FmtStr(al, f1, f2, "%.3f Mhz", (float)m_tick_clk / 1000000), sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "After Boot"),
      TT_FmtStr(al, f1, f2, "%.2fs", (float)m_time_us() / 1000000), sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Build Time"),
                    TT_Str(al, f1, f2, __DATE__ " " __TIME__), sep);
#if MOD_CFG_USE_OS_KLITE  // klite
  TT_AddTitle(
      tt,
      TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ KLite RTOS Info ]"),
      '-');
  kv = TT_AddKVPair(tt, 0);
#if SHOWKLITE
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Thread Num"),
                    TT_FmtStr(al, f1, f2, "%d", ulist_len(thread_list)), sep);
#endif
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
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "System Usage"),
                    TT_FmtStr(al, f1, f2, "%.2f%%", usage * 100), sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "System Avg Usage"),
                    TT_FmtStr(al, f1, f2, "%.2f%%", usage_avg * 100), sep);
#endif
#if SCH_CFG_ENABLE_TASK || SCH_CFG_ENABLE_EVENT || SCH_CFG_ENABLE_COROUTINE
  TT_AddTitle(
      tt,
      TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ Scheduler Info ]"),
      '-');
  kv = TT_AddKVPair(tt, 0);
#if SCH_CFG_ENABLE_TASK
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Task Num"),
                    TT_FmtStr(al, f1, f2, "%d", Sch_GetTaskNum()), sep);
#endif  // SCH_CFG_ENABLE_TASK
#if SCH_CFG_ENABLE_EVENT
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Event Num"),
                    TT_FmtStr(al, f1, f2, "%d", Sch_GetEventNum()), sep);
#endif  // SCH_CFG_ENABLE_EVENT
#if SCH_CFG_ENABLE_COROUTINE
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Coroutine Num"),
                    TT_FmtStr(al, f1, f2, "%d", Sch_GetCortnNum()), sep);
#endif  // SCH_CFG_ENABLE_COROUTINE
#endif  // SCHEDULER
#if SHOWLWMEM
  TT_AddTitle(
      tt, TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ LwMem Info ]"),
      '-');
  kv = TT_AddKVPair(tt, 0);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Total"),
      TT_FmtStr(al, f1, f2, "%d Bytes (%.1f KB)", stats.mem_size_bytes,
                (float)stats.mem_size_bytes / 1024),
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
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Allocation"),
                    TT_FmtStr(al, f1, f2, "%d blocks", stats.nr_alloc), sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Free"),
                    TT_FmtStr(al, f1, f2, "%d blocks", stats.nr_free), sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Alive"),
      TT_FmtStr(al, f1, f2, "%d blocks", stats.nr_alloc - stats.nr_free), sep);
#elif SHOWHEAP4
  TT_AddTitle(
      tt, TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ Heap-4 Info ]"),
      '-');
  kv = TT_AddKVPair(tt, 0);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Total"),
      TT_FmtStr(al, f1, f2, "%d Bytes (%.1f KB)", xPortGetTotalHeapSize(),
                (float)xPortGetTotalHeapSize() / 1024),
      sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Avail"),
                    TT_FmtStr(al, f1, f2, "%d Bytes (%.4f%%)",
                              stats.xAvailableHeapSpaceInBytes,
                              (float)(stats.xAvailableHeapSpaceInBytes) /
                                  (float)xPortGetTotalHeapSize() * 100),
                    sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Min Avail"),
                    TT_FmtStr(al, f1, f2, "%d Bytes (%.4f%%)",
                              stats.xMinimumEverFreeBytesRemaining,
                              (float)(stats.xMinimumEverFreeBytesRemaining) /
                                  (float)xPortGetTotalHeapSize() * 100),
                    sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Free Block"),
      TT_FmtStr(al, f1, f2, "%d blocks", stats.xNumberOfFreeBlocks), sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "- Largest"),
      TT_FmtStr(al, f1, f2, "%d Bytes", stats.xSizeOfLargestFreeBlockInBytes),
      sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "- Smallest"),
      TT_FmtStr(al, f1, f2, "%d Bytes", stats.xSizeOfSmallestFreeBlockInBytes),
      sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Allocation"),
      TT_FmtStr(al, f1, f2, "%d blocks", stats.xNumberOfSuccessfulAllocations),
      sep);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Free"),
      TT_FmtStr(al, f1, f2, "%d blocks", stats.xNumberOfSuccessfulFrees), sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Alive"),
                    TT_FmtStr(al, f1, f2, "%d blocks",
                              stats.xNumberOfSuccessfulAllocations -
                                  stats.xNumberOfSuccessfulFrees),
                    sep);
#elif SHOWRTTHREAD
  TT_AddTitle(tt,
              TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD,
                     "[ RT-Thread Heap Info ]"),
              '-');
  kv = TT_AddKVPair(tt, 0);
  TT_KVPair_AddItem(
      kv, 2, TT_Str(al, f1, f2, "Total"),
      TT_FmtStr(al, f1, f2, "%d Bytes (%.1f KB)", total, (float)total / 1024),
      sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Used"),
                    TT_FmtStr(al, f1, f2, "%d Bytes (%.4f%%)", used,
                              (float)used / total * 100),
                    sep);
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Max Used"),
                    TT_FmtStr(al, f1, f2, "%d Bytes (%.4f%%)", max_used,
                              (float)max_used / total * 100),
                    sep);
#endif
  TT_AddSeparator(tt, TT_FMT1_GREEN, TT_FMT2_BOLD, '-');
  TT_Print(tt);
  TT_FreeTable(tt);
}

// Public Functions -------------------------

void CliUtils_AddCmdToCli(EmbeddedCli *cli) {
  static CliCommandBinding sysinfo_cmd = {
      .name = "sysinfo",
      .usage = "sysinfo",
      .help = "Show system info",
      .context = NULL,
      .autoTokenizeArgs = 0,
      .func = sysinfo_cmd_func,
  };
  embeddedCliAddBinding(cli, sysinfo_cmd);
#if SHOWKLITE
  static CliCommandBinding klite_cmd = {
      .name = "klite",
      .usage =
          "klite [-l list | -s suspend | -r resume | -d delete | -p priority] "
          "[thread id] [priority]",
      .help = "KLite RTOS control command",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = klite_cmd_func,
  };
  embeddedCliAddBinding(cli, klite_cmd);
#endif
}

// Source Code End --------------------------

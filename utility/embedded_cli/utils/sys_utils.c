/**
 * @file system_utils.c
 * @brief 系统命令行工具
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0.0
 * @date 2024-03-02
 *
 * THINK DIFFERENTLY
 */

#include "sys_utils.h"

#define LOG_MODULE "sys"
#include "log.h"
#include "scheduler.h"
#include "term_table.h"
// Private Defines --------------------------

#if (MOD_CFG_HEAP_MATHOD_LWMEM)
#include "lwmem.h"
#define SHOWLWMEM 1
#elif (MOD_CFG_HEAP_MATHOD_HEAP4)
#include "heap4.h"
#define SHOWHEAP4 1
#elif (MOD_CFG_HEAP_MATHOD_RTT)
#define SHOWRTTHREAD 1
#endif
#if (MOD_CFG_USE_OS_KLITE)
#include "klite.h"
#define SHOWKLITE 1
#endif

#if __FPU_PRESENT
#if /* ARMCC */ (                                                   \
    (defined(__CC_ARM) && defined(__TARGET_FPU_VFP)) /* Clang */ || \
    (defined(__CLANG_ARM) && defined(__VFP_FP__) &&                 \
     !defined(__SOFTFP__)) /* IAR */                                \
    || (defined(__ICCARM__) && defined(__ARMVFP__)) /* GNU */ ||    \
    (defined(__GNUC__) && defined(__VFP_FP__) && !defined(__SOFTFP__)))
#define USE_FPU 1
#else
#define USE_FPU 0
#endif
#endif

// Private Typedefs -------------------------

// Private Macros ---------------------------

// Private Variables ------------------------

// Public Variables -------------------------

// Private Functions ------------------------
#if SHOWKLITE
static void list_klite(void) {
    TT tt = TT_NewTable(-1);
    TT_FMT1 f1 = TT_FMT1_GREEN;
    TT_FMT2 f2 = TT_FMT2_BOLD;
    TT_ALIGN al = TT_ALIGN_LEFT;
    TT_ITEM_GRID grid = TT_AddGrid(tt, 0);
    TT_ITEM_GRID_LINE line =
        TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
    const char* head[] = {"ID",    "Pri",        "State", "Entry",
                          "Usage", "Free Stack", "Err"};
    for (int i = 0; i < sizeof(head) / sizeof(char*); i++)
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head[i]));
    size_t sfree, ssize;
    f2 = TT_FMT2_NONE;
    kl_thread_t thread = NULL;
    uint8_t flags = 0;
    while ((thread = kl_thread_iter(thread)) != NULL) {
        line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " "));
        double usage =
            (double)kl_thread_time(thread) / (double)kl_kernel_tick64();
        flags = kl_thread_flags(thread);
        kl_thread_stack_info(thread, &sfree, &ssize);
        TT_GridLine_AddItem(line,
                            TT_FmtStr(al, f1, f2, "%d", kl_thread_id(thread)));
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%d", kl_thread_priority(thread)));
        TT_GridLine_AddItem(
            line, TT_Str(al, f1, f2,
                         (flags & KL_THREAD_FLAGS_SUSPEND)
                             ? "SUSPEND"
                             : ((flags & KL_THREAD_FLAGS_WAIT)
                                    ? "WAIT"
                                    : ((flags & KL_THREAD_FLAGS_SLEEP)
                                           ? "SLEEP"
                                           : ((flags & KL_THREAD_FLAGS_READY)
                                                  ? "READY"
                                                  : "RUNNING")))));
        if (kl_thread_priority(thread) == 0)
            TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "[Idle]"));
        else
            TT_GridLine_AddItem(line,
                                TT_FmtStr(al, f1, f2, "%p", thread->entry));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%.4f%%", usage * 100));
        TT_GridLine_AddItem(line,
                            TT_FmtStr(al, f1, f2, "%d / %d", sfree, ssize));
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%d", kl_thread_errno(thread)));
    }
    TT_Print(tt);
    TT_FreeTable(tt);
}

static void klite_cmd_func(EmbeddedCli* cli, char* args, void* context) {
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
        PRINTLN(T_FMT(T_BOLD, T_RED) "Thread ID is required" T_RST);
        return;
    }
    int id = atoi(embeddedCliGetToken(args, -1));
    kl_thread_t thread = kl_thread_find(id);
    if (!thread) {
        PRINTLN(T_FMT(T_BOLD, T_RED) "Thread ID not found: %d" T_RST, id);
        return;
    }
    if (kl_thread_priority(thread) == 0) {
        PRINTLN(T_FMT(T_BOLD, T_RED) "Cannot operate on idle thread" T_RST);
        return;
    }
    if (embeddedCliCheckToken(args, "-s", 1)) {
        kl_thread_suspend(thread);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "Thread %d suspended" T_RST, id);
    } else if (embeddedCliCheckToken(args, "-r", 1)) {
        kl_thread_resume(thread);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "Thread %d resumed" T_RST, id);
    } else if (embeddedCliCheckToken(args, "-k", 1)) {
        kl_thread_delete(thread);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "Thread %d killed" T_RST, id);
    } else if (embeddedCliCheckToken(args, "-p", 1)) {
        if (argc < 3) {
            PRINTLN(T_FMT(T_BOLD, T_RED) "Priority is required" T_RST);
            return;
        }
        int pri = atoi(embeddedCliGetToken(args, 2));
        if (pri < 0 || pri > KLITE_CFG_MAX_PRIO) {
            PRINTLN(T_FMT(T_BOLD, T_RED) "Priority must be 0-%d",
                    KLITE_CFG_MAX_PRIO);
            return;
        }
        kl_thread_set_priority(thread, pri);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "Thread %d priority set to %d" T_RST, id,
                pri);
    } else {
        PRINTLN(T_FMT(T_BOLD, T_RED) "Invalid command" T_RST);
    }
}
#endif

static void sysinfo_cmd_func(EmbeddedCli* cli, char* args, void* context) {
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
#elif SHOWKLITE
    struct kl_heap_stats stats;
    kl_heap_stats(&stats);
#endif
    TT tt = TT_NewTable(-1);
    TT_AddTitle(
        tt,
        TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ System Info ]"),
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
        TT_FmtStr(al, f1, f2, "%.2fs", (float)((uint64_t)m_time_ms()) / 1000),
        sep);
#if __FPU_PRESENT
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "FPU"),
                      TT_Str(al, f1, f2, USE_FPU ? "Enabled" : "Disabled"),
                      sep);
#endif
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Build Time"),
                      TT_Str(al, f1, f2, __DATE__ " " __TIME__), sep);
#if SCH_CFG_ENABLE_TASK || SCH_CFG_ENABLE_EVENT || SCH_CFG_ENABLE_COROUTINE
    TT_AddTitle(tt,
                TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD,
                       "[ Scheduler Info ]"),
                '-');
    kv = TT_AddKVPair(tt, 0);
#if SCH_CFG_ENABLE_TASK
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Task Num"),
                      TT_FmtStr(al, f1, f2, "%d", sch_task_get_num()), sep);
#endif
#if SCH_CFG_ENABLE_EVENT
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Event Num"),
                      TT_FmtStr(al, f1, f2, "%d", sch_event_get_num()), sep);
#endif
#if SCH_CFG_ENABLE_COROUTINE
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Coroutine Num"),
                      TT_FmtStr(al, f1, f2, "%d", sch_cortn_get_num()), sep);
#endif
#endif  // SCHEDULER
#if SHOWLWMEM
    TT_AddTitle(
        tt,
        TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ LwMem Info ]"),
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
    TT_KVPair_AddItem(
        kv, 2, TT_Str(al, f1, f2, "Min Avail"),
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
        TT_FmtStr(al, f1, f2, "%d blocks", stats.nr_alloc - stats.nr_free),
        sep);
#elif SHOWHEAP4
    TT_AddTitle(
        tt,
        TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD, "[ Heap-4 Info ]"),
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
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "- Smallest"),
                      TT_FmtStr(al, f1, f2, "%d Bytes",
                                stats.xSizeOfSmallestFreeBlockInBytes),
                      sep);
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Allocation"),
                      TT_FmtStr(al, f1, f2, "%d blocks",
                                stats.xNumberOfSuccessfulAllocations),
                      sep);
    TT_KVPair_AddItem(
        kv, 2, TT_Str(al, f1, f2, "Free"),
        TT_FmtStr(al, f1, f2, "%d blocks", stats.xNumberOfSuccessfulFrees),
        sep);
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
#elif SHOWKLITE
    TT_AddTitle(tt,
                TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD,
                       "[ KLite RTOS Info ]"),
                '-');
    kv = TT_AddKVPair(tt, 0);
    uint16_t kl_thread_num = 0;
    kl_thread_t thread = NULL;
    while ((thread = kl_thread_iter(thread)) != NULL)
        kl_thread_num++;
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Thread Num"),
                      TT_FmtStr(al, f1, f2, "%d", kl_thread_num), sep);
    static uint64_t last_kernel_tick = 0;
    static uint64_t last_idle_time = 0;
    uint64_t tick = kl_kernel_tick64();
    uint64_t idle_time = kl_kernel_idle_time();
    float usage =
        (float)((tick - last_kernel_tick) - (idle_time - last_idle_time)) /
        (float)(tick - last_kernel_tick);
    float usage_avg = (float)(tick - idle_time) / (float)tick;
    last_kernel_tick = tick;
    last_idle_time = idle_time;
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "System Usage"),
                      TT_FmtStr(al, f1, f2, "%.2f%%", usage * 100), sep);
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "System Avg Usage"),
                      TT_FmtStr(al, f1, f2, "%.2f%%", usage_avg * 100), sep);
    TT_AddTitle(tt,
                TT_Str(TT_ALIGN_LEFT, TT_FMT1_GREEN, TT_FMT2_BOLD,
                       "[ KLite Heap Info ]"),
                '-');
    kv = TT_AddKVPair(tt, 0);
    TT_KVPair_AddItem(
        kv, 2, TT_Str(al, f1, f2, "Total"),
        TT_FmtStr(al, f1, f2, "%d Bytes (%.1f KB)", stats.total_size,
                  (float)stats.total_size / 1024),
        sep);
    TT_KVPair_AddItem(
        kv, 2, TT_Str(al, f1, f2, "Avail"),
        TT_FmtStr(al, f1, f2, "%d Bytes (%.4f%%)", stats.avail_size,
                  (float)(stats.avail_size) / (float)stats.total_size * 100),
        sep);
    TT_KVPair_AddItem(
        kv, 2, TT_Str(al, f1, f2, "Min Avail"),
        TT_FmtStr(
            al, f1, f2, "%d Bytes (%.4f%%)", stats.minimum_ever_avail,
            (float)(stats.minimum_ever_avail) / (float)stats.total_size * 100),
        sep);
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Free Block"),
                      TT_FmtStr(al, f1, f2, "%d blocks", stats.free_blocks),
                      sep);
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "- Largest"),
                      TT_FmtStr(al, f1, f2, "%d Bytes", stats.largest_free),
                      sep);
    TT_KVPair_AddItem(
        kv, 2, TT_Str(al, f1, f2, "- 2nd Largest"),
        TT_FmtStr(al, f1, f2, "%d Bytes", stats.second_largest_free), sep);
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "- Smallest"),
                      TT_FmtStr(al, f1, f2, "%d Bytes", stats.smallest_free),
                      sep);
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Allocation"),
                      TT_FmtStr(al, f1, f2, "%d blocks", stats.alloc_count),
                      sep);
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Free"),
                      TT_FmtStr(al, f1, f2, "%d blocks", stats.free_count),
                      sep);
    TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Alive"),
                      TT_FmtStr(al, f1, f2, "%d blocks",
                                stats.alloc_count - stats.free_count),
                      sep);
#endif
    TT_AddSeparator(tt, TT_FMT1_GREEN, TT_FMT2_BOLD, '-');
    TT_Print(tt);
    TT_FreeTable(tt);
}

static void memdump_cmd_func(EmbeddedCli* cli, char* args, void* context) {
    int len = 64;
    int width = 8;
    int argc = embeddedCliGetTokenCount(args);
    if (argc < 1) {
        embeddedCliPrintCurrentHelp(cli);
        return;
    }
    uint8_t* addr = (uint8_t*)strtoul(embeddedCliGetToken(args, -1), NULL, 16);
    int pos;
    if ((pos = embeddedCliFindToken(args, "-l")) != 0 && argc >= pos + 1) {
        len = atoi(embeddedCliGetToken(args, pos + 1));
    }
    if ((pos = embeddedCliFindToken(args, "-w")) != 0 && argc >= pos + 1) {
        width = atoi(embeddedCliGetToken(args, pos + 1));
    }
    int n;
    PRINTLN(T_FMT(T_BOLD, T_BLUE) "Memory Dump of 0x%X:" T_RST, addr);
    while (1) {
        n = len > width ? width : len;
        if (n <= 0)
            break;
        len -= n;
        PRINT(T_FMT(T_GREEN) "0x%08X" T_RST, addr);
        for (int i = 0; i < n; i++) {  // print hex
            if (i % 8 == 0)
                PRINT(" ");
            PRINT("%02X ", addr[i]);
        }
        for (int i = n; i < width; i++) {  // padding
            PRINT("   ");
        }
        PRINT(T_FMT(T_GREEN) "| " T_RST);
        for (int i = 0; i < n; i++) {  // print ascii
            if (addr[i] >= 32 && addr[i] <= 126) {
                PRINT("%c", addr[i]);
            } else {
                PRINT(T_FMT(T_BLUE) "." T_RST);
            }
        }
        PRINTLN("");
        addr += n;
    }
}

#if KLITE_CFG_HEAP_TRACE_OWNER
#include "klite.h"

static void memtrace_cmd_func(EmbeddedCli* cli, char* args, void* context) {
    int fpid = -1;
    kl_size_t fsize = 0;
    int argc = embeddedCliGetTokenCount(args);
    int pos;
    bool frag_only = false;
    if ((pos = embeddedCliFindToken(args, "-p")) != 0 && argc >= pos + 1) {
        fpid = atoi(embeddedCliGetToken(args, pos + 1));
    }
    if ((pos = embeddedCliFindToken(args, "-s")) != 0 && argc >= pos + 1) {
        fsize = atoi(embeddedCliGetToken(args, pos + 1));
    }
    if ((pos = embeddedCliFindToken(args, "-f")) != 0) {
        frag_only = true;
    }
    PRINTLN(T_FMT(T_BOLD, T_BLUE) "Memory Trace Info:" T_FMT(T_RESET, T_BLUE));
    PRINTLN("Addr(Used/Space) -> PID" T_FMT(T_GREEN));
    void* iter_tmp = NULL;
    kl_thread_t owner = NULL;
    kl_size_t addr = 0, used = 0, size = 0;
    while (kl_heap_iter_nodes(&iter_tmp, &owner, &addr, &used, &size)) {
        if ((fpid != -1 && kl_thread_id(owner) != fpid) || (size < fsize))
            continue;
        if (frag_only && used == size)
            continue;
        if (used == size) {
            PRINT(T_FMT(T_GREEN));
        } else {
            PRINT(T_FMT(T_YELLOW));
        }
        PRINTLN("0x%X - %d/%d -> %d", addr, used, size, kl_thread_id(owner));
    }
}
#endif

// Public Functions -------------------------

void system_utils_add_command_to_cli(EmbeddedCli* cli) {
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
            "klite [-l list | -s suspend | -r resume | -k kill | -p "
            "<priority>] "
            "<thread id>",
        .help = "KLite RTOS control command",
        .context = NULL,
        .autoTokenizeArgs = 1,
        .func = klite_cmd_func,
    };
    embeddedCliAddBinding(cli, klite_cmd);
#endif
    static CliCommandBinding memdump_cmd = {
        .name = "memdump",
        .usage = "memdump [-l <length>] [-w <width>] <address>",
        .help = "Dump memory content",
        .context = NULL,
        .autoTokenizeArgs = 1,
        .func = memdump_cmd_func,
    };
    embeddedCliAddBinding(cli, memdump_cmd);
#if KLITE_CFG_HEAP_TRACE_OWNER
    static CliCommandBinding memtrace_cmd = {
        .name = "memtrace",
        .usage = "memtrace [-f only fragmentation | -p <pid> | -s <min size>]",
        .help = "Trace memory allocations",
        .context = NULL,
        .autoTokenizeArgs = 1,
        .func = memtrace_cmd_func,
    };
    embeddedCliAddBinding(cli, memtrace_cmd);
#endif
}

// Source Code End --------------------------

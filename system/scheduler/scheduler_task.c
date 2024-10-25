#include "scheduler_task.h"

#include "scheduler_internal.h"
#if SCH_CFG_ENABLE_TASK
typedef struct {           // 用户任务结构
    ID_NAME_VAR(name);     // 任务名
    sch_task_func_t task;  // 任务函数指针
    uint64_t period;       // 任务调度周期(Tick)
    uint64_t pendTime;     // 下次执行时间(Tick)
    uint8_t enable;        // 是否使能
    uint8_t priority;      // 优先级
    void* args;            // 任务参数
#if SCH_CFG_DEBUG_REPORT
    uint64_t max_cost;    // 任务最大执行时间(Tick)
    uint64_t total_cost;  // 任务总执行时间(Tick)
    uint64_t max_lat;     // 任务调度延迟(Tick)
    uint64_t total_lat;   // 任务调度延迟总和(Tick)
    uint32_t run_cnt;     // 任务执行次数
    float last_usage;     // 任务上次执行占用率
    uint8_t unsync;       // 丢失同步
#endif
} scheduler_task_t;

static ulist_t tasklist = {.data = NULL,
                           .cap = 0,
                           .num = 0,
                           .elfree = NULL,
                           .isize = sizeof(scheduler_task_t),
                           .opt = ULIST_OPT_CLEAR_DIRTY_REGION};

static scheduler_task_t* pending_task = NULL;

static int taskcmp(const void* a, const void* b) {
    int priority1 = ((scheduler_task_t*)a)->priority;
    int priority2 = ((scheduler_task_t*)b)->priority;
#if SCH_CFG_PRI_ORDER_ASC
    return priority2 - priority1;  // 高优先级在前
#else
    return priority1 - priority2;  // 低优先级在前
#endif
}

static scheduler_task_t* find_task(const char* name) {
    ulist_foreach(&tasklist, scheduler_task_t, task) {
        if (fast_strcmp(task->name, name))
            return task;
    }
    return NULL;
}

static void resort_task(void) {
    if (tasklist.num <= 1)
        return;
    ulist_sort(&tasklist, taskcmp, SLICE_START, SLICE_END);
}

static inline scheduler_task_t* get_next_task(void) {
    if (!tasklist.num)
        return NULL;
    uint64_t now = get_sys_tick();
    scheduler_task_t* next = NULL;
    ulist_foreach(&tasklist, scheduler_task_t, task) {
        if (!task->enable)
            continue;  // 跳过禁用任务
        if (now >= task->pendTime)
            return task;  // 高优先级在前
        if (next == NULL || task->pendTime < next->pendTime)
            next = task;
    }
    return next;
}

_INLINE uint64_t task_runner(void) {
    if (!pending_task)
        return UINT64_MAX;
    uint64_t now = get_sys_tick();
    if (now < pending_task->pendTime) {
        return tick_to_us(pending_task->pendTime - now);
    }
    scheduler_task_t* task = pending_task;
    uint64_t latency = now - task->pendTime;
    if (latency <= us_to_tick(SCH_CFG_COMP_RANGE_US)) {
        task->pendTime += task->period;
    } else {
        task->pendTime = now + task->period;
#if SCH_CFG_DEBUG_REPORT
        task->unsync = 1;
#endif
    }
#if SCH_CFG_DEBUG_REPORT
    uint64_t _sch_debug_task_tick = get_sys_tick();
    task->task(task->args);
    _sch_debug_task_tick = get_sys_tick() - _sch_debug_task_tick;
    if (task->max_cost < _sch_debug_task_tick)
        task->max_cost = _sch_debug_task_tick;
    if (latency > task->max_lat)
        task->max_lat = latency;
    task->total_cost += _sch_debug_task_tick;
    task->total_lat += latency;
    task->run_cnt++;
#else
    task->task(task->args);
#endif  // SCH_CFG_DEBUG_REPORT
    pending_task = get_next_task();
    return 0;
}

uint8_t sch_task_create(const char* name, sch_task_func_t func, float freq_hz,
                        uint8_t enable, uint8_t priority, void* args) {
    scheduler_task_t task = {
        .task = func,
        .enable = enable,
        .priority = priority,
        .period = (double)get_sys_freq() / (double)freq_hz,
        .pendTime = get_sys_tick(),
        .args = args,
    };
    ID_NAME_SET(task.name, name);
    if (!task.period)
        task.period = 1;
    if (!ulist_append_copy(&tasklist, &task))
        return 0;
    resort_task();
    pending_task = get_next_task();
    return 1;
}

uint8_t sch_task_delete(const char* name) {
    scheduler_task_t* p = find_task(name);
    if (p == NULL)
        return 0;
    ulist_remove(&tasklist, p);
    pending_task = get_next_task();
    return 1;
}

uint8_t sch_task_get_exist(const char* name) {
    return find_task(name) != NULL;
}

uint8_t sch_task_get_enabled(const char* name) {
    scheduler_task_t* p = find_task(name);
    if (p == NULL)
        return 0;
    return p->enable;
}

uint8_t sch_task_set_priority(const char* name, uint8_t priority) {
    scheduler_task_t* p = find_task(name);
    if (p == NULL)
        return 0;
    p->priority = priority;
    resort_task();
    pending_task = get_next_task();
    return 1;
}

uint8_t sch_task_set_args(const char* name, void* args) {
    scheduler_task_t* p = find_task(name);
    if (p == NULL)
        return 0;
    p->args = args;
    return 1;
}

uint8_t sch_task_delay(const char* name, uint64_t delay_us, uint8_t from_now) {
    scheduler_task_t* p = find_task(name);
    if (p == NULL)
        return 0;
    if (from_now)
        p->pendTime = us_to_tick(delay_us) + get_sys_tick();
    else
        p->pendTime += us_to_tick(delay_us);
    pending_task = get_next_task();
    return 1;
}

uint16_t sch_task_get_num(void) {
    return tasklist.num;
}

uint8_t sch_task_set_enabled(const char* name, uint8_t enable) {
    scheduler_task_t* p = find_task(name);
    if (p == NULL)
        return 0;
    p->enable = enable;
    if (p->enable)
        p->pendTime = get_sys_tick();
    pending_task = get_next_task();
    return 1;
}

uint8_t sch_task_set_freq(const char* name, float freq_hz) {
    scheduler_task_t* p = find_task(name);
    if (p == NULL)
        return 0;
    p->period = (double)get_sys_freq() / (double)freq_hz;
    if (!p->period)
        p->period = 1;
    p->pendTime = get_sys_tick();
    pending_task = get_next_task();
    return 1;
}

#if SCH_CFG_DEBUG_REPORT
void sch_task_add_debug(TT tt, uint64_t period, uint64_t* other) {
    if (tasklist.num) {
        TT_FMT1 f1 = TT_FMT1_BLUE;
        TT_FMT2 f2 = TT_FMT2_BOLD;
        TT_ALIGN al = TT_ALIGN_LEFT;
        TT_AddTitle(
            tt,
            TT_FmtStr(al, f1, f2, "[ Task Report / %d ]", sch_task_get_num()),
            '-');
        TT_ITEM_GRID grid = TT_AddGrid(tt, 0);
        TT_ITEM_GRID_LINE line =
            TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
        const char* head1[] = {"No",    "Pri",   "Run",   "Tmax/us",
                               "Usage/%", "LTavg(max)/us", "Name"};
        for (int i = 0; i < sizeof(head1) / sizeof(char*); i++)
            TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head1[i]));
        int i = 0;
        ulist_foreach(&tasklist, scheduler_task_t, task) {
            if (i >= SCH_CFG_DEBUG_MAXLINE) {
                TT_AddString(
                    tt,
                    TT_Str(TT_ALIGN_CENTER, TT_FMT1_NONE, TT_FMT2_NONE, "..."),
                    0);
                break;
            }
            line = TT_Grid_AddLine(grid, TT_Str(al, f1, f2, " "));
            if (task->enable) {
                float usage = (float)task->total_cost / period * 100;
                f1 = TT_FMT1_GREEN;
                f2 = TT_FMT2_NONE;
                TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
                TT_GridLine_AddItem(
                    line, TT_FmtStr(al, f1, f2, "%d", task->priority));

                if (task->unsync) {
                    f1 = TT_FMT1_RED;
                    f2 = TT_FMT2_BOLD;
                }
                TT_GridLine_AddItem(line,
                                    TT_FmtStr(al, f1, f2, "%d", task->run_cnt));
                f1 = TT_FMT1_GREEN;
                f2 = TT_FMT2_NONE;
                TT_GridLine_AddItem(
                    line,
                    TT_FmtStr(al, f1, f2, "%.2f", tick_to_us(task->max_cost)));
                if ((task->last_usage != 0 && usage / task->last_usage > 2) ||
                    usage > 20) {  // 任务占用率大幅度增加或者超过20%
                    f1 = TT_FMT1_YELLOW;
                    f2 = TT_FMT2_BOLD;
                }
                TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%.3f", usage));
                f1 = TT_FMT1_GREEN;
                f2 = TT_FMT2_NONE;
                TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%.2f(%.2f)",
                                                    tick_to_us(task->total_lat /
                                                               task->run_cnt),
                                                    tick_to_us(task->max_lat)));
                TT_GridLine_AddItem(line,
                                    TT_FmtStr(al, f1, f2, "%s", task->name));
                task->last_usage = usage;
                *other -= task->total_cost;
            } else {
                f1 = TT_FMT1_WHITE;
                f2 = TT_FMT2_NONE;
                TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
                TT_GridLine_AddItem(
                    line, TT_FmtStr(al, f1, f2, "%d", task->priority));
                TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
                TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
                TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
                TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
                TT_GridLine_AddItem(line,
                                    TT_FmtStr(al, f1, f2, "%s", task->name));
                task->last_usage = 0;
            }
            i++;
        }
    }
}

void sch_task_finish_debug(uint8_t first_print, uint64_t offset) {
    ulist_foreach(&tasklist, scheduler_task_t, task) {
        if (first_print)
            task->pendTime = get_sys_tick();
        else
            task->pendTime += offset;
        task->max_cost = 0;
        task->total_cost = 0;
        task->run_cnt = 0;
        task->max_lat = 0;
        task->total_lat = 0;
        task->unsync = 0;
    }
    pending_task = get_next_task();
}
#endif  // SCH_CFG_DEBUG_REPORT

#if SCH_CFG_ENABLE_TERMINAL
void task_cmd_func(EmbeddedCli* cli, char* args, void* context) {
    size_t argc = embeddedCliGetTokenCount(args);
    if (!argc) {
        embeddedCliPrintCurrentHelp(cli);
        return;
    }
    if (embeddedCliCheckToken(args, "-l", 1)) {
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "tasks list:" T_FMT(T_RESET, T_GREEN));
        uint16_t max_len = 0;
        uint16_t temp;
        ulist_foreach(&tasklist, scheduler_task_t, task) {
            temp = strlen(task->name);
            if (temp > max_len)
                max_len = temp;
        }
        ulist_foreach(&tasklist, scheduler_task_t, task) {
            PRINTLN("  %-*s | entry:%p pri:%d en:%d freq:%.1f", max_len,
                    task->name, task->task, task->priority, task->enable,
                    (float)get_sys_freq() / task->period);
        }
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "Total %d tasks" T_RST, tasklist.num);
        return;
    }
    if (argc < 2) {
        PRINTLN(T_FMT(T_BOLD, T_RED) "task name is required" T_RST);
        return;
    }
    const char* name = embeddedCliGetToken(args, -1);
    scheduler_task_t* p = find_task(name);
    if (p == NULL) {
        PRINTLN(T_FMT(T_BOLD, T_RED) "task: %s not found" T_RST, name);
        return;
    }
    if (embeddedCliCheckToken(args, "-e", 1)) {
        sch_task_set_enabled(name, ENABLE);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "task: %s enabled" T_RST, name);
    } else if (embeddedCliCheckToken(args, "-d", 1)) {
        sch_task_set_enabled(name, DISABLE);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "task: %s disabled" T_RST, name);
    } else if (embeddedCliCheckToken(args, "-r", 1)) {
        sch_task_delete(name);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "task: %s deleted" T_RST, name);
    } else if (embeddedCliCheckToken(args, "-f", 1)) {
        if (argc < 3) {
            PRINTLN(T_FMT(T_BOLD, T_RED) "_frequency is required" T_RST);
            return;
        }
        float freq = atof(embeddedCliGetToken(args, 2));
        if (freq < 0.00001) {
            PRINTLN(T_FMT(T_BOLD, T_RED) "_frequency too low" T_RST);
            return;
        }
        sch_task_set_freq(name, freq);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "task: %s frequency set to %.2fHz" T_RST,
                name, freq);
    } else if (embeddedCliCheckToken(args, "-p", 1)) {
        if (argc < 3) {
            PRINTLN(T_FMT(T_BOLD, T_RED) "_priority is required" T_RST);
            return;
        }
        uint8_t pri = atoi(embeddedCliGetToken(args, 2));
        sch_task_set_priority(name, pri);
        PRINTLN(T_FMT(T_BOLD, T_GREEN) "task: %s priority set to %d" T_RST,
                name, pri);
    } else if (embeddedCliCheckToken(args, "-E", 1)) {
        PRINTLN(T_FMT(T_BOLD, T_YELLOW) "Force excuting task: %s" T_RST, name);
        p->task(p->args);
    } else {
        PRINTLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
    }
}
#endif  // SCH_CFG_ENABLE_TERMINAL

#endif  // SCH_CFG_ENABLE_TASK

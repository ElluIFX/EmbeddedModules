#include "scheduler_task.h"

#include "scheduler_internal.h"
#if _SCH_ENABLE_TASK
#pragma pack(1)
typedef struct {     // 用户任务结构
  sch_func_t task;   // 任务函数指针
  uint64_t period;   // 任务调度周期(Tick)
  uint64_t lastRun;  // 上次执行时间(Tick)
  uint8_t enable;    // 是否使能
  uint8_t priority;  // 优先级
  void *args;        // 任务参数
#if _SCH_DEBUG_REPORT
  uint64_t max_cost;    // 任务最大执行时间(Tick)
  uint64_t total_cost;  // 任务总执行时间(Tick)
  uint64_t max_lat;     // 任务调度延迟(Tick)
  uint64_t total_lat;   // 任务调度延迟总和(Tick)
  uint32_t run_cnt;     // 任务执行次数
  float last_usage;     // 任务上次执行占用率
  uint8_t unsync;       // 丢失同步
#endif
  const char *name;  // 任务名
} scheduler_task_t;
#pragma pack()

static ulist_t tasklist = {.data = NULL,
                           .cap = 0,
                           .num = 0,
                           .elfree = NULL,
                           .isize = sizeof(scheduler_task_t),
                           .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

static uint8_t task_changed = 0;

static int taskcmp(const void *a, const void *b) {
  uint8_t priority1 = ((scheduler_task_t *)a)->priority;
  uint8_t priority2 = ((scheduler_task_t *)b)->priority;
  return priority1 > priority2 ? 1 : -1;
}

static scheduler_task_t *get_task(const char *name) {
  ulist_foreach(&tasklist, scheduler_task_t, task) {
    if (fast_strcmp(task->name, name)) return task;
  }
  return NULL;
}

static void resort_task(void) {
  if (tasklist.num <= 1) return;
  ulist_sort(&tasklist, taskcmp, SLICE_START, SLICE_END);
  task_changed = 1;
}

_INLINE uint64_t Task_Runner(void) {
  if (!tasklist.num) return UINT64_MAX;
  uint64_t latency;
  uint64_t now;
  uint64_t sleep_us = UINT64_MAX;
  if (tasklist.num) {
    now = get_sys_tick();
    ulist_foreach(&tasklist, scheduler_task_t, task) {
      if (!task->enable) continue;  // 跳过禁用任务
      if (now >= task->lastRun + task->period) {
        latency = now - (task->lastRun + task->period);
        if (latency <= us_to_tick(_SCH_COMP_RANGE_US)) {
          task->lastRun += task->period;
        } else {
          task->lastRun = now;
#if _SCH_DEBUG_REPORT
          task->unsync = 1;
#endif
        }
#if _SCH_DEBUG_REPORT
        task_changed = 0;
        uint64_t _sch_debug_task_tick = get_sys_tick();
        task->task(task->args);
        _sch_debug_task_tick = get_sys_tick() - _sch_debug_task_tick;
        if (task_changed) return 0;  // 列表已改变
        if (task->max_cost < _sch_debug_task_tick)
          task->max_cost = _sch_debug_task_tick;
        if (latency > task->max_lat) task->max_lat = latency;
        task->total_cost += _sch_debug_task_tick;
        task->total_lat += latency;
        task->run_cnt++;
#else
        task->task(task->args);
#endif             // _SCH_DEBUG_REPORT
        return 0;  // 有任务被执行，直接返回
      } else {
        // 计算最小休眠时间
        latency = tick_to_us(task->lastRun + task->period - now);
        if (latency < sleep_us) sleep_us = latency;
      }
    }
  }
  return sleep_us;
}

uint8_t Sch_CreateTask(const char *name, sch_func_t func, float freqHz,
                       uint8_t enable, uint8_t priority, void *args) {
  scheduler_task_t *p = (scheduler_task_t *)ulist_append(&tasklist);
  if (p == NULL) return 0;
  p->task = func;
  p->enable = enable;
  p->priority = priority;
  p->period = (double)get_sys_freq() / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = get_sys_tick() - p->period;
  p->name = name;
  p->args = args;
  task_changed = 1;
  resort_task();
  return 1;
}

uint8_t Sch_DeleteTask(const char *name) {
  if (tasklist.num == 0) return 0;
  ulist_foreach(&tasklist, scheduler_task_t, task) {
    if (fast_strcmp(task->name, name)) {
      ulist_delete(&tasklist, task - (scheduler_task_t *)tasklist.data);
      task_changed = 1;
      return 1;
    }
  }
  return 0;
}

uint8_t Sch_IsTaskExist(const char *name) { return get_task(name) != NULL; }

uint8_t Sch_GetTaskState(const char *name) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return 0;
  return p->enable;
}

uint8_t Sch_SetTaskPriority(const char *name, uint8_t priority) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return 0;
  p->priority = priority;
  resort_task();
  return 1;
}

uint8_t Sch_DelayTask(const char *name, uint64_t delayUs, uint8_t fromNow) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return 0;
  if (fromNow)
    p->lastRun = us_to_tick(delayUs) + get_sys_tick();
  else
    p->lastRun += us_to_tick(delayUs);
  return 1;
}

uint16_t Sch_GetTaskNum(void) { return tasklist.num; }

uint8_t Sch_SetTaskState(const char *name, uint8_t enable) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return 0;
  p->enable = enable;
  if (p->enable) p->lastRun = get_sys_tick() - p->period;
  return 1;
}

uint8_t Sch_SetTaskFreq(const char *name, float freqHz) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return 0;
  p->period = (double)get_sys_freq() / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = get_sys_tick() - p->period;
  return 1;
}

#if _SCH_DEBUG_REPORT
void sch_task_add_debug(TT tt, uint64_t period, uint64_t *other) {
  if (tasklist.num) {
    TT_FMT1 f1 = TT_FMT1_BLUE;
    TT_FMT2 f2 = TT_FMT2_BOLD;
    TT_ALIGN al = TT_ALIGN_LEFT;
    TT_AddTitle(tt, TT_Str(al, f1, f2, "[ Task Report ]"), '-');
    TT_ITEM_GRID grid = TT_AddGrid(tt, 0);
    TT_ITEM_GRID_LINE line =
        TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
    const char *head1[] = {"No",    "Pri",   "Run",   "Tmax",
                           "Usage", "LTavg", "LTmax", "Name"};
    for (int i = 0; i < sizeof(head1) / sizeof(char *); i++)
      TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head1[i]));
    int i = 0;
    ulist_foreach(&tasklist, scheduler_task_t, task) {
      line = TT_Grid_AddLine(grid, TT_Str(al, f1, f2, " "));
      if (task->enable) {
        float usage = (float)task->total_cost / period * 100;
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", task->priority));

        if (task->unsync) {
          f1 = TT_FMT1_RED;
          f2 = TT_FMT2_BOLD;
        }
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", task->run_cnt));
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%.2f", tick_to_us(task->max_cost)));
        if ((task->last_usage != 0 && usage / task->last_usage > 2) ||
            usage > 20) {  // 任务占用率大幅度增加或者超过20%
          f1 = TT_FMT1_YELLOW;
          f2 = TT_FMT2_BOLD;
        }
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%.3f", usage));
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%.2f",
                            tick_to_us(task->total_lat / task->run_cnt)));
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%.2f", tick_to_us(task->max_lat)));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%s", task->name));
        task->last_usage = usage;
        *other -= task->total_cost;
      } else {
        f1 = TT_FMT1_WHITE;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", task->priority));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%s", task->name));
        task->last_usage = 0;
      }
      i++;
      if (i >= _SCH_DEBUG_MAXLINE) {
        TT_AddString(
            tt, TT_Str(TT_ALIGN_CENTER, TT_FMT1_NONE, TT_FMT2_NONE, "..."), 0);
        break;
      }
    }
  }
}
void sch_task_finish_debug(uint8_t first_print, uint64_t offset) {
  ulist_foreach(&tasklist, scheduler_task_t, task) {
    if (first_print)
      task->lastRun = get_sys_tick();
    else
      task->lastRun += offset;
    task->max_cost = 0;
    task->total_cost = 0;
    task->run_cnt = 0;
    task->max_lat = 0;
    task->total_lat = 0;
    task->unsync = 0;
  }
}
#endif  // _SCH_DEBUG_REPORT

#if _SCH_ENABLE_TERMINAL
void task_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (embeddedCliCheckToken(args, "list", 1)) {
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Tasks list:" T_FMT(T_RESET, T_GREEN));
    ulist_foreach(&tasklist, scheduler_task_t, task) {
      LOG_RAWLN("  %s: 0x%p pri:%d freq:%.1f en:%d", task->name, task->task,
                task->priority, (float)get_sys_freq() / task->period,
                task->enable);
    }
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Total %d tasks" T_RST, tasklist.num);
    return;
  }
  if (argc < 2) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Task name is required" T_RST);
    return;
  }
  const char *name = embeddedCliGetToken(args, 2);
  scheduler_task_t *p = get_task(name);
  if (p == NULL) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Task: %s not found" T_RST, name);
    return;
  }
  if (embeddedCliCheckToken(args, "enable", 1)) {
    Sch_SetTaskState(name, ENABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task: %s enabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "disable", 1)) {
    Sch_SetTaskState(name, DISABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task: %s disabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "delete", 1)) {
    Sch_DeleteTask(name);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task: %s deleted" T_RST, name);
  } else if (embeddedCliCheckToken(args, "setfreq", 1)) {
    if (argc < 3) {
      LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Frequency is required" T_RST);
      return;
    }
    float freq = atof(embeddedCliGetToken(args, 3));
    Sch_SetTaskFreq(name, freq);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task: %s frequency set to %.2fHz" T_RST,
              name, freq);
  } else if (embeddedCliCheckToken(args, "setpri", 1)) {
    if (argc < 3) {
      LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Priority is required" T_RST);
      return;
    }
    uint8_t pri = atoi(embeddedCliGetToken(args, 3));
    Sch_SetTaskPriority(name, pri);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task: %s priority set to %d" T_RST, name,
              pri);
  } else if (embeddedCliCheckToken(args, "excute", 1)) {
    LOG_RAWLN(T_FMT(T_BOLD, T_YELLOW) "Force excuting Task: %s" T_RST, name);
    p->task(p->args);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
#endif  // _SCH_ENABLE_TERMINAL

#endif  // _SCH_ENABLE_TASK

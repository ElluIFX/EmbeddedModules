#include "scheduler_event.h"

#include "scheduler_internal.h"
#if _SCH_ENABLE_EVENT
#pragma pack(1)
typedef struct {     // 事件任务结构
  sch_func_t task;   // 任务函数指针
  uint8_t enable;    // 是否使能
  uint8_t trigger;   // 是否已触发
  void *args;        // 任务参数
  uint8_t needFree;  // 是否需要释放参数内存
#if _SCH_DEBUG_REPORT
  uint64_t trigger_time;  // 触发时间(Tick)
  uint64_t max_cost;      // 任务最大执行时间(Tick)
  uint64_t total_cost;    // 任务总执行时间(Tick)
  uint64_t max_lat;       // 任务调度延迟(Tick)
  uint64_t total_lat;     // 任务调度延迟总和(Tick)
  uint32_t run_cnt;       // 任务执行次数
  uint32_t trigger_cnt;   // 触发次数
  float last_usage;       // 任务上次执行占用率
#endif
  const char *name;  // 事件名
} scheduler_event_t;
#pragma pack()

static ulist_t eventlist = {.data = NULL,
                            .cap = 0,
                            .num = 0,
                            .elfree = NULL,
                            .isize = sizeof(scheduler_event_t),
                            .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};
static uint8_t event_modified = 0;

_INLINE void Event_Runner(void) {
  if (!eventlist.num) return;
  event_modified = 0;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (event->enable && event->trigger) {
      event->trigger = 0;
#if !_SCH_DEBUG_REPORT
      event->task(event->args);
      if (event_modified) return;
      if (event->args != NULL && event->needFree) m_free(event->args);
      event->args = NULL;
#else
      uint64_t _sch_debug_task_tick = get_sys_tick();
      uint64_t _late = _sch_debug_task_tick - event->trigger_time;
      event->task(event->args);
      _sch_debug_task_tick = get_sys_tick() - _sch_debug_task_tick;
      if (event_modified) return;
      if (event->max_cost < _sch_debug_task_tick)
        event->max_cost = _sch_debug_task_tick;
      event->total_cost += _sch_debug_task_tick;
      if (event->max_lat < _late) event->max_lat = _late;
      event->total_lat += _late;
      event->run_cnt++;
#endif  // !_SCH_DEBUG_REPORT
    }
  }
}

uint8_t Sch_CreateEvent(const char *name, sch_func_t callback, uint8_t enable) {
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) return 0;
  }
  scheduler_event_t *p = (scheduler_event_t *)ulist_append(&eventlist);
  if (p == NULL) return 0;
  p->task = callback;
  p->enable = enable;
  p->trigger = 0;
  p->args = NULL;
  p->name = name;
  p->needFree = 0;
  event_modified = 1;
  return 1;
}

__STATIC_INLINE scheduler_event_t *get_event(const char *name) {
  if (eventlist.num == 0) return NULL;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      return event;
    }
  }
  return NULL;
}

uint8_t Sch_DeleteEvent(const char *name) {
  scheduler_event_t *event = get_event(name);
  if (event == NULL) return 0;
  if (event->args != NULL && event->needFree) m_free(event->args);
  ulist_delete(&eventlist, ulist_index(&eventlist, event));
  event_modified = 1;
  return 1;
}

uint8_t Sch_SetEventState(const char *name, uint8_t enable) {
  scheduler_event_t *event = get_event(name);
  if (event == NULL) return 0;
  event->enable = enable;
  event->trigger = 0;
  return 1;
}

uint8_t Sch_TriggerEvent(const char *name, void *args) {
  scheduler_event_t *event = get_event(name);
  if (event == NULL) return 0;
  if (!event->enable) return 0;
  event->trigger = 0;
  if (event->args != NULL && event->needFree) m_free(event->args);
  event->args = args;
  event->needFree = 0;
  return 1;
}

uint8_t Sch_TriggerEventEx(const char *name, const void *arg_ptr,
                           uint16_t arg_size) {
  scheduler_event_t *event = get_event(name);
  if (event == NULL) return 0;
  if (!event->enable) return 0;
  event->trigger = 0;
  if (event->args != NULL && event->needFree) m_free(event->args);
  event->args = m_alloc(arg_size);
  if (event->args == NULL) return 0;
  memcpy(event->args, arg_ptr, arg_size);
  event->needFree = 1;
#if _SCH_DEBUG_REPORT
  event->trigger_time = get_sys_tick();
  event->trigger_cnt++;
#endif
  event->trigger = 1;
  return 1;
}

uint8_t Sch_IsEventExist(const char *name) {
  return get_event(name) == NULL ? 0 : 1;
}

uint8_t Sch_GetEventState(const char *name) {
  scheduler_event_t *event = get_event(name);
  if (event == NULL) return 0;
  return event->enable;
}

uint16_t Sch_GetEventNum(void) { return eventlist.num; }

#if _SCH_DEBUG_REPORT
void sch_event_add_debug(TT tt, uint64_t period, uint64_t *other) {
  if (eventlist.num) {
    TT_FMT1 f1 = TT_FMT1_BLUE;
    TT_FMT2 f2 = TT_FMT2_BOLD;
    TT_ALIGN al = TT_ALIGN_LEFT;
    TT_AddTitle(tt, TT_Str(al, f1, f2, "[ Event Report ]"), '-');
    TT_ITEM_GRID grid = TT_AddGrid(tt, 0);
    TT_ITEM_GRID_LINE line =
        TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
    const char *head2[] = {"No",    "Tri",   "Run",   "Tmax",
                           "Usage", "LTavg", "LTmax", "Event"};
    for (int i = 0; i < sizeof(head2) / sizeof(char *); i++)
      TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head2[i]));
    int i = 0;
    ulist_foreach(&eventlist, scheduler_event_t, event) {
      line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " "));
      if (event->enable && event->run_cnt) {
        float usage = (float)event->total_cost / period * 100;
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
        TT_GridLine_AddItem(line,
                            TT_FmtStr(al, f1, f2, "%d", event->trigger_cnt));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", event->run_cnt));
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%.2f", tick_to_us(event->max_cost)));
        if ((event->last_usage != 0 && usage / event->last_usage > 2) ||
            usage > 20) {  // 任务占用率大幅度增加或者超过20%
          f1 = TT_FMT1_YELLOW;
          f2 = TT_FMT2_BOLD;
        }
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%.3f", usage));
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%.3f",
                            tick_to_us(event->total_lat / event->run_cnt)));
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%.2f", tick_to_us(event->max_lat)));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%s", event->name));
        event->last_usage = usage;
        *other -= event->total_cost;
      } else {
        if (!event->enable) {
          f1 = TT_FMT1_WHITE;
          f2 = TT_FMT2_NONE;
        } else {
          f1 = TT_FMT1_GREEN;
          f2 = TT_FMT2_NONE;
        }
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
        TT_GridLine_AddItem(line,
                            TT_FmtStr(al, f1, f2, "%d", event->trigger_cnt));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", event->run_cnt));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%s", event->name));
        event->last_usage = 0;
      }
      i++;
    }
  }
}
void sch_event_finish_debug(uint8_t first_print, uint64_t offset) {
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (first_print)
      event->trigger_time = get_sys_tick();
    else
      event->trigger_time += offset;
    event->max_cost = 0;
    event->total_cost = 0;
    event->run_cnt = 0;
    event->trigger_cnt = 0;
    event->max_lat = 0;
    event->total_lat = 0;
    event->trigger = 0;
  }
}
#endif  // _SCH_DEBUG_REPORT

#if _SCH_ENABLE_TERMINAL
void event_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (embeddedCliCheckToken(args, "list", 1)) {
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Events list:" T_FMT(T_RESET, T_GREEN));
    ulist_foreach(&eventlist, scheduler_event_t, event) {
      LOG_RAWLN("  %s: 0x%p en:%d", event->name, event->task, event->enable);
    }
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Total %d events" T_RST, eventlist.num);
    return;
  }
  if (argc < 2) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Event name is required" T_RST);
    return;
  }
  const char *name = embeddedCliGetToken(args, 2);
  scheduler_event_t *p = NULL;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      p = event;
      break;
    }
  }
  if (p == NULL) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Event: %s not found" T_RST, name);
    return;
  }
  if (embeddedCliCheckToken(args, "enable", 1)) {
    Sch_SetEventState(name, ENABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Event: %s enabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "disable", 1)) {
    Sch_SetEventState(name, DISABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Event: %s disabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "delete", 1)) {
    Sch_DeleteEvent(name);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Event: %s deleted" T_RST, name);
  } else if (embeddedCliCheckToken(args, "trigger", 1)) {
    Sch_TriggerEvent(name, NULL);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Event: %s triggered" T_RST, name);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
#endif  // _SCH_ENABLE_TERMINAL

#endif  // _SCH_ENABLE_EVENT

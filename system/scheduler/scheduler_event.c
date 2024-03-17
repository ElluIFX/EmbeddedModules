#include "scheduler_event.h"

#include "scheduler_internal.h"
#if SCH_CFG_ENABLE_EVENT
#pragma pack(1)
typedef struct {      // 事件结构
  ID_NAME_VAR(name);  // 事件名
  event_func_t task;  // 事件回调函数指针
  uint8_t enable;     // 是否使能
#if SCH_CFG_DEBUG_REPORT
  uint64_t max_cost;     // 事件最大执行时间(Tick)
  uint64_t total_cost;   // 事件总执行时间(Tick)
  uint64_t max_lat;      // 事件调度延迟(Tick)
  uint64_t total_lat;    // 事件调度延迟总和(Tick)
  uint32_t run_cnt;      // 事件执行次数
  uint32_t trigger_cnt;  // 触发次数
  float last_usage;      // 事件上次执行占用率
#endif
} scheduler_event_t;
typedef struct {              // 事件触发结构
  event_func_t task;          // 事件回调函数指针
  scheduler_event_arg_t arg;  // 事件参数
  uint8_t allocated;          // 动态分配的参数内存
#if SCH_CFG_DEBUG_REPORT
  uint64_t trigger_time;     // 触发时间(Tick)
  scheduler_event_t *event;  // 源事件指针
#endif
} scheduler_triggered_event_t;
#pragma pack()

static ulist_t eventlist = {.data = NULL,
                            .cap = 0,
                            .num = 0,
                            .elfree = NULL,
                            .isize = sizeof(scheduler_event_t),
                            .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

static ulist_t triggered_eventlist = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .elfree = NULL,
    .isize = sizeof(scheduler_triggered_event_t),
    .cfg = ULIST_CFG_NO_SHRINK | ULIST_CFG_NO_AUTO_FREE};

_INLINE void Event_Runner(void) {
  static uint64_t last_event_us = 0;
  if (!triggered_eventlist.num) {
    if (triggered_eventlist.cap &&
        get_sys_us() - last_event_us > 10000000) {  // 10s无事件触发，释放内存
      ulist_mem_shrink(&triggered_eventlist);
    }
    return;
  }
  uint16_t cnt = 0;
  ulist_foreach(&triggered_eventlist, scheduler_triggered_event_t, triggered) {
#if !SCH_CFG_DEBUG_REPORT
    triggered->task(triggered->arg);
#else
    scheduler_event_t *event = triggered->event;
    uint64_t now = get_sys_tick();
    uint64_t _late = now - triggered->trigger_time;
    triggered->task(triggered->arg);
    now = get_sys_tick() - now;
    if (event->max_cost < now) event->max_cost = now;
    event->total_cost += now;
    if (event->max_lat < _late) event->max_lat = _late;
    event->total_lat += _late;
    event->run_cnt++;
#endif  // !SCH_CFG_DEBUG_REPORT
    if (triggered->allocated) m_free(triggered->arg.ptr);
    cnt++;
  }
  ulist_delete_multi(&triggered_eventlist, 0, cnt);
  last_event_us = get_sys_us();
}

uint8_t Sch_CreateEvent(const char *name, event_func_t callback,
                        uint8_t enable) {
  if (!name || !callback) return 0;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) return 0;
  }
  scheduler_event_t event = {
      .task = callback,
      .enable = enable,
  };
  ID_NAME_SET(event.name, name);
  return ulist_append_copy(&eventlist, &event);
}

__STATIC_INLINE scheduler_event_t *find_event(const char *name) {
  if (eventlist.num == 0) return NULL;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      return event;
    }
  }
  return NULL;
}

uint8_t Sch_DeleteEvent(const char *name) {
  scheduler_event_t *event = find_event(name);
  if (event == NULL) return 0;
  ulist_remove(&eventlist, event);
  return 1;
}

uint8_t Sch_SetEventEnabled(const char *name, uint8_t enable) {
  scheduler_event_t *event = find_event(name);
  if (event == NULL) return 0;
  event->enable = enable;
  return 1;
}

uint8_t Sch_TriggerEvent(const char *name, uint8_t arg_type, void *arg_ptr,
                         size_t arg_size) {
  scheduler_event_t *event = find_event(name);
  if (event == NULL) return 0;
  if (!event->enable) return 0;
  scheduler_triggered_event_t triggered = {.task = event->task,
                                           .arg =
                                               {
                                                   .type = arg_type,
                                                   .ptr = arg_ptr,
                                                   .size = arg_size,
                                               },
                                           .allocated = 0};
#if SCH_CFG_DEBUG_REPORT
  triggered.trigger_time = get_sys_tick();
  triggered.event = event;
  event->trigger_cnt++;
#endif
  return ulist_append_copy(&triggered_eventlist, &triggered);
  return 1;
}

uint8_t Sch_TriggerEventEx(const char *name, uint8_t arg_type,
                           const void *arg_ptr, size_t arg_size) {
  scheduler_event_t *event = find_event(name);
  if (event == NULL) return 0;
  if (!event->enable) return 0;
  void *args = m_alloc(arg_size);
  if (args == NULL) return 0;
  memcpy(args, arg_ptr, arg_size);
  scheduler_triggered_event_t triggered = {
      .task = event->task,
      .arg = {.type = arg_type, .ptr = args, .size = arg_size},
      .allocated = 1};
#if SCH_CFG_DEBUG_REPORT
  triggered.trigger_time = get_sys_tick();
  triggered.event = event;
  event->trigger_cnt++;
#endif
  uint8_t ret = ulist_append_copy(&triggered_eventlist, &triggered);
  if (!ret) m_free(args);
  return ret;
}

uint8_t Sch_IsEventExist(const char *name) {
  return find_event(name) == NULL ? 0 : 1;
}

uint8_t Sch_GetEventEnabled(const char *name) {
  scheduler_event_t *event = find_event(name);
  if (event == NULL) return 0;
  return event->enable;
}

uint16_t Sch_GetEventNum(void) { return eventlist.num; }

#if SCH_CFG_DEBUG_REPORT
void sch_event_add_debug(TT tt, uint64_t period, uint64_t *other) {
  if (eventlist.num) {
    TT_FMT1 f1 = TT_FMT1_BLUE;
    TT_FMT2 f2 = TT_FMT2_BOLD;
    TT_ALIGN al = TT_ALIGN_LEFT;
    TT_AddTitle(
        tt, TT_FmtStr(al, f1, f2, "[ Event Report / %d ]", Sch_GetEventNum()),
        '-');
    TT_ITEM_GRID grid = TT_AddGrid(tt, 0);
    TT_ITEM_GRID_LINE line =
        TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
    const char *head2[] = {"No",    "Tri",   "Run",   "Tmax",
                           "Usage", "LTavg", "LTmax", "Event"};
    for (int i = 0; i < sizeof(head2) / sizeof(char *); i++)
      TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head2[i]));
    int i = 0;
    ulist_foreach(&eventlist, scheduler_event_t, event) {
      if (i >= SCH_CFG_DEBUG_MAXLINE) {
        TT_AddString(
            tt, TT_Str(TT_ALIGN_CENTER, TT_FMT1_NONE, TT_FMT2_NONE, "..."), 0);
        break;
      }
      line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " "));
      if (event->enable && event->run_cnt) {
        float usage = (float)event->total_cost / period * 100;
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
        if (event->trigger_cnt != event->run_cnt) {
          f1 = TT_FMT1_RED;
          f2 = TT_FMT2_BOLD;
        }
        TT_GridLine_AddItem(line,
                            TT_FmtStr(al, f1, f2, "%d", event->trigger_cnt));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", event->run_cnt));
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%.2f", tick_to_us(event->max_cost)));
        if ((event->last_usage != 0 && usage / event->last_usage > 2) ||
            usage > 20) {  // 事件占用率大幅度增加或者超过20%
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
    event->max_cost = 0;
    event->total_cost = 0;
    event->run_cnt = 0;
    event->trigger_cnt = 0;
    event->max_lat = 0;
    event->total_lat = 0;
  }
}
#endif  // SCH_CFG_DEBUG_REPORT

#if SCH_CFG_ENABLE_TERMINAL
void event_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (embeddedCliCheckToken(args, "-l", 1)) {
    PRINTLN(T_FMT(T_BOLD, T_GREEN) "Events list:" T_FMT(T_RESET, T_GREEN));
    uint16_t max_len = 0;
    uint16_t temp;
    ulist_foreach(&eventlist, scheduler_event_t, event) {
      temp = strlen(event->name);
      if (temp > max_len) max_len = temp;
    }
    ulist_foreach(&eventlist, scheduler_event_t, event) {
      PRINTLN("  %-*s | entry:%p en:%d", max_len, event->name, event->task,
              event->enable);
    }
    PRINTLN(T_FMT(T_BOLD, T_GREEN) "Total %d events" T_RST, eventlist.num);
    return;
  }
  if (argc < 2) {
    PRINTLN(T_FMT(T_BOLD, T_RED) "Event name is required" T_RST);
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
    PRINTLN(T_FMT(T_BOLD, T_RED) "Event: %s not found" T_RST, name);
    return;
  }
  if (embeddedCliCheckToken(args, "-e", 1)) {
    Sch_SetEventEnabled(name, ENABLE);
    PRINTLN(T_FMT(T_BOLD, T_GREEN) "Event: %s enabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "-d", 1)) {
    Sch_SetEventEnabled(name, DISABLE);
    PRINTLN(T_FMT(T_BOLD, T_GREEN) "Event: %s disabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "-r", 1)) {
    Sch_DeleteEvent(name);
    PRINTLN(T_FMT(T_BOLD, T_GREEN) "Event: %s deleted" T_RST, name);
  } else if (embeddedCliCheckToken(args, "-t", 1)) {
    if (argc < 4) {
      PRINTLN(T_FMT(T_BOLD, T_RED) "Event need argument (type/content)" T_RST,
              name);
      return;
    }
    Sch_TriggerEvent(name, atoi(embeddedCliGetToken(args, 3)),
                     (void *)embeddedCliGetToken(args, 4),
                     strlen(embeddedCliGetToken(args, 4)));
    PRINTLN(T_FMT(T_BOLD, T_GREEN) "Event: %s triggered" T_RST, name);
  } else {
    PRINTLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
#endif  // SCH_CFG_ENABLE_TERMINAL

#endif  // SCH_CFG_ENABLE_EVENT

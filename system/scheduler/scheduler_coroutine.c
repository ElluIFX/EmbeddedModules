#include "scheduler_coroutine.h"

#include "scheduler_internal.h"

#if _SCH_ENABLE_COROUTINE

typedef struct {        // 协程任务结构
  const char *name;     // 协程名
  sch_func_t task;      // 任务函数指针
  uint8_t enable;       // 是否使能
  uint8_t mode;         // 模式
  void *args;           // 协程主函数参数
  __cortn_handle_t hd;  // 协程句柄
#if _SCH_DEBUG_REPORT
  uint64_t max_cost;    // 协程最大执行时间(Tick)
  uint64_t total_cost;  // 协程总执行时间(Tick)
  float last_usage;     // 协程上次执行占用率
#endif
} scheduler_cortn_t;
#pragma pack()

static ulist_t cortnlist = {.data = NULL,
                            .cap = 0,
                            .num = 0,
                            .isize = sizeof(scheduler_cortn_t),
                            .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

#pragma pack(1)
typedef struct {     // 协程互斥锁结构
  const char *name;  // 锁名
  uint8_t locked;    // 锁状态
  ulist_t waitlist;  // 等待的协程列表
} scheduler_cortn_mutex_t;

typedef struct {     // 协程事件结构
  const char *name;  // 事件名
  ulist_t waitlist;  // 等待的协程列表
} scheduler_cortn_event_t;

static ulist_t mutexlist = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .isize = sizeof(scheduler_cortn_mutex_t),
    .cfg = ULIST_CFG_CLEAR_DIRTY_REGION | ULIST_CFG_NO_ALLOC_EXTEND};

static ulist_t eventlist = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .isize = sizeof(scheduler_cortn_event_t),
    .cfg = ULIST_CFG_CLEAR_DIRTY_REGION | ULIST_CFG_NO_ALLOC_EXTEND};

__cortn_handle_t *__chd = {0};
static uint16_t __chd_idx = 0;
static uint8_t cr_modified = 0;

_INLINE uint64_t Cortn_Runner(void) {
  if (!cortnlist.num) return UINT64_MAX;
  cr_modified = 0;
  uint64_t sleep_us = UINT64_MAX;
  uint64_t now = get_sys_us();
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (!cortn->enable || !cortn->hd.yieldUntil) continue;  // 跳过禁用协程
    if (now >= cortn->hd.yieldUntil) {
      __chd = &cortn->hd;
      __chd_idx = cortn - (scheduler_cortn_t *)cortnlist.data;
      __chd->depth = 0;
#if _SCH_DEBUG_REPORT
      uint64_t _sch_debug_task_tick = get_sys_tick();
      cortn->task(cortn->args);
      _sch_debug_task_tick = get_sys_tick() - _sch_debug_task_tick;
      if (cortn->max_cost < _sch_debug_task_tick)
        cortn->max_cost = _sch_debug_task_tick;
      cortn->total_cost += _sch_debug_task_tick;
#else
      cortn->task(cortn->args);
#endif
      __chd = NULL;
      __chd_idx = 0;
      if (!cortn->hd.data[0].ptr) {  // 最外层协程已结束
        cortn->hd.yieldUntil = 1;    // 准备下一次执行
        if (cortn->mode == CR_MODE_AUTODEL) {
          Sch_DeleteCortn(cortn->name);
          return 0;  // 指针已被释放
        } else if (cortn->mode == CR_MODE_ONESHOT) {
          cortn->enable = 0;
          continue;
        }
      }
    }
    if (cr_modified) return 0;  // 有协程被修改，不确定
    if (cortn->hd.yieldUntil - now < sleep_us) {
      sleep_us = cortn->hd.yieldUntil - now;
    } else {
      sleep_us = 0;  // 直接yield的协程，不休眠
    }
  }
  return sleep_us;
}

uint8_t Sch_CreateCortn(const char *name, sch_func_t func, uint8_t enable,
                        CR_MODE mode, void *args) {
  scheduler_cortn_t *p = (scheduler_cortn_t *)ulist_append(&cortnlist);
  if (__chd != NULL) {  // 列表添加可能会导致旧指针失效
    __chd = &ulist_get_ptr(&cortnlist, scheduler_cortn_t, __chd_idx)->hd;
  }
  if (p == NULL) return 0;
  p->task = func;
  p->enable = enable;
  p->mode = mode;
  p->args = args;
  p->name = name;
  p->hd.dataList.cfg = ULIST_CFG_CLEAR_DIRTY_REGION |
                       ULIST_CFG_NO_ALLOC_EXTEND | ULIST_CFG_NO_SHRINK;
  p->hd.dataList.isize = sizeof(__cortn_data_t);
  p->hd.dataList.num = 0;
  p->hd.dataList.cap = 0;
  ulist_append(&p->hd.dataList);
  p->hd.data = (__cortn_data_t *)p->hd.dataList.data;
  p->hd.data[0].local = NULL;
  p->hd.data[0].ptr = NULL;
  p->hd.yieldUntil = get_sys_us();
  p->hd.msg = NULL;
  p->hd.maxDepth = 0;
  p->hd.depth = 0;
  cr_modified = 1;
  return 1;
}

uint8_t Sch_DeleteCortn(const char *name) {
  if (cortnlist.num == 0) return 0;
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      if (__chd == &cortn->hd) {
        // 在协程中删除自身
        cortn->hd.data[0].ptr = NULL;
        cortn->mode = CR_MODE_AUTODEL;
        return 1;
      }
      ulist_foreach(&cortn->hd.dataList, __cortn_data_t, data) {
        if (data->local != NULL) m_free(data->local);
      }
      ulist_delete(&cortnlist, cortn - (scheduler_cortn_t *)cortnlist.data);
      cr_modified = 1;
      return 1;
    }
  }
  return 0;
}

uint8_t Sch_SetCortnState(const char *name, uint8_t enable,
                          uint8_t clearState) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      cortn->enable = enable;
      if (clearState) {
        memset(&cortn->hd, 0, sizeof(__cortn_handle_t));
      }
      cortn->hd.yieldUntil = get_sys_us();
      return 1;
    }
  }
  return 0;
}

uint16_t Sch_GetCortnNum(void) { return cortnlist.num; }

uint8_t Sch_IsCortnExist(const char *name) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      return 1;
    }
  }
  return 0;
}

uint8_t Sch_GetCortnState(const char *name) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      return cortn->enable;
    }
  }
  return 0;
}

uint8_t Sch_IsCortnWaitForMsg(const char *name) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      return cortn->hd.yieldUntil == 0;
    }
  }
  return 0;
}

uint8_t Sch_SendMsgToCortn(const char *name, void *msg) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      cortn->hd.msg = msg;
      cortn->hd.yieldUntil = get_sys_us();
      return 1;
    }
  }
  return 0;
}
/**
 * @brief (内部函数)初始化协程本地变量存储区指针
 * @param  size 存储区大小
 * @return 存储区指针
 */
_INLINE void *__Sch_CrInitLocal(uint16_t size) {
  if (size == 0) return NULL;
  if (__chd->data[__chd->depth].local == NULL) {
    __chd->data[__chd->depth].local = m_alloc(size);
    memset(__chd->data[__chd->depth].local, 0, size);
  }
  return __chd->data[__chd->depth].local;
}

/**
 * @brief (内部函数)协程嵌套调用准备
 * @return 是否允许进行调用
 */
_INLINE uint8_t __Sch_CrAwaitEnter(void) {
  __chd->depth++;
  if (__chd->depth > __chd->maxDepth) {
    if (!ulist_append(&__chd->dataList)) {
      __chd->depth--;
      return 0;
    }
    __chd->data = __chd->dataList.data;
    __chd->data[__chd->depth].local = NULL;
    __chd->data[__chd->depth].ptr = NULL;
    __chd->maxDepth++;
  }
  return 1;
}

/**
 * @brief (内部函数)协程嵌套调用返回
 * @return 嵌套协程已结束
 */
_INLINE uint8_t __Sch_CrAwaitReturn(void) {
  __chd->depth--;
  if (__chd->data[__chd->depth + 1].ptr != NULL) {
    return 0;
  }
  if (__chd->data[__chd->depth + 1].local != NULL) {
    m_free(__chd->data[__chd->depth + 1].local);
    __chd->data[__chd->depth + 1].local = NULL;
  }
  ulist_delete(&__chd->dataList, -1);
  __chd->data = __chd->dataList.data;
  __chd->maxDepth--;
  return 1;
}

/**
 * @brief (内部函数)协程延时
 * @param  delayUs 延时时间(us)
 */
_INLINE void __Sch_CrDelay(uint64_t delayUs) {
  __chd->yieldUntil = get_sys_us() + delayUs;
}

#if _SCH_DEBUG_REPORT
void sch_cortn_add_debug(TT tt, uint64_t period, uint64_t *other) {
  if (cortnlist.num) {
    TT_FMT1 f1 = TT_FMT1_BLUE;
    TT_FMT2 f2 = TT_FMT2_BOLD;
    TT_ALIGN al = TT_ALIGN_LEFT;
    TT_AddTitle(tt, TT_Str(al, f1, f2, "[ Coroutine Report ]"), '-');
    TT_ITEM_GRID grid = TT_AddGrid(tt, 0);
    TT_ITEM_GRID_LINE line =
        TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
    const char *head3[] = {"No", "State", "Depth", "Tmax", "Usage", "Name"};
    for (int i = 0; i < sizeof(head3) / sizeof(char *); i++)
      TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head3[i]));
    int i = 0;
    ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
      line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " "));
      if (cortn->enable) {
        float usage = (float)cortn->total_cost / period * 100;
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
        TT_GridLine_AddItem(
            line,
            TT_Str(al, f1, f2,
                   cortn->hd.yieldUntil == 0
                       ? "await"
                       : (cortn->hd.yieldUntil > get_sys_us() ? "sleep"
                                                              : "yield")));
        TT_GridLine_AddItem(line,
                            TT_FmtStr(al, f1, f2, "%d", cortn->hd.maxDepth));
        TT_GridLine_AddItem(
            line, TT_FmtStr(al, f1, f2, "%.2f", tick_to_us(cortn->max_cost)));
        if ((cortn->last_usage != 0 && usage / cortn->last_usage > 2) ||
            usage > 20) {  // 任务占用率大幅度增加或者超过20%
          f1 = TT_FMT1_YELLOW;
          f2 = TT_FMT2_BOLD;
        }
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%.3f", usage));
        f1 = TT_FMT1_GREEN;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%s", cortn->name));

        cortn->last_usage = usage;
        *other -= cortn->total_cost;
      } else {
        f1 = TT_FMT1_WHITE;
        f2 = TT_FMT2_NONE;
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
        TT_GridLine_AddItem(
            line,
            TT_Str(al, f1, f2,
                   cortn->hd.yieldUntil == 0
                       ? "await"
                       : (cortn->hd.yieldUntil > get_sys_us() ? "sleep"
                                                              : "yield")));
        TT_GridLine_AddItem(line,
                            TT_FmtStr(al, f1, f2, "%d", cortn->hd.maxDepth));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
        TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%s", cortn->name));
        cortn->last_usage = 0;
      }
      i++;
    }
  }
}
void sch_cortn_finish_debug(uint8_t first_print, uint64_t offset) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    cortn->max_cost = 0;
    cortn->total_cost = 0;
  }
}
#endif  // _SCH_DEBUG_REPORT

#if _SCH_ENABLE_TERMINAL
void cortn_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (embeddedCliCheckToken(args, "list", 1)) {
    LOG_RAWLN(
        T_FMT(T_BOLD, T_GREEN) "Coroutines list:" T_FMT(T_RESET, T_GREEN));
    ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
      LOG_RAWLN(
          "  %s: 0x%p depth:%d en:%d state:%s", cortn->name, cortn->task,
          cortn->hd.maxDepth, cortn->enable,
          cortn->hd.yieldUntil == 0
              ? "await"
              : (cortn->hd.yieldUntil > get_sys_us() ? "sleep" : "yield"));
    }
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Total %d coroutines" T_RST,
              cortnlist.num);
    return;
  }
  if (argc < 2) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Coroutine name is required" T_RST);
    return;
  }
  const char *name = embeddedCliGetToken(args, 2);
  scheduler_cortn_t *p = NULL;
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      p = cortn;
      break;
    }
  }
  if (p == NULL) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Coroutine: %s not found" T_RST, name);
    return;
  }
  if (embeddedCliCheckToken(args, "enable", 1)) {
    Sch_SetCortnState(name, ENABLE, 1);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Coroutine: %s enabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "disable", 1)) {
    Sch_SetCortnState(name, DISABLE, 1);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Coroutine: %s disabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "delete", 1)) {
    Sch_DeleteCortn(name);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Coroutine: %s deleted" T_RST, name);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
#endif  // _SCH_ENABLE_TERMINAL

#endif  // _SCH_ENABLE_COROUTINE

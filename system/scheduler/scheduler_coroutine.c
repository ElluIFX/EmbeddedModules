#include "scheduler_coroutine.h"

#include "scheduler_internal.h"

#if _SCH_ENABLE_COROUTINE

#pragma pack(1)
typedef struct {        // 协程任务结构
  const char *name;     // 协程名
  cortn_func_t task;    // 任务函数指针
  void *args;           // 协程主函数参数
  __cortn_handle_t hd;  // 协程句柄
#if _SCH_DEBUG_REPORT
  uint64_t max_cost;    // 协程最大执行时间(Tick)
  uint64_t total_cost;  // 协程总执行时间(Tick)
  float last_usage;     // 协程上次执行占用率
#endif
} scheduler_cortn_t;

typedef struct {     // 协程互斥锁结构
  const char *name;  // 锁名
  uint8_t locked;    // 锁状态
  ulist_t waitlist;  // 等待的协程列表
} scheduler_cortn_mutex_t;

typedef struct {     // 协程屏障结构
  const char *name;  // 屏障名
  uint16_t target;   // 目标协程数
  ulist_t waitlist;  // 等待的协程列表
} scheduler_cortn_barrier_t;
#pragma pack()

static ulist_t cortnlist = {.data = NULL,
                            .cap = 0,
                            .num = 0,
                            .elfree = NULL,
                            .isize = sizeof(scheduler_cortn_t),
                            .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

static ulist_t mutexlist = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .elfree = NULL,
    .isize = sizeof(scheduler_cortn_mutex_t),
    .cfg = ULIST_CFG_CLEAR_DIRTY_REGION | ULIST_CFG_NO_ALLOC_EXTEND};

static ulist_t barrierlist = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .elfree = NULL,
    .isize = sizeof(scheduler_cortn_barrier_t),
    .cfg = ULIST_CFG_CLEAR_DIRTY_REGION | ULIST_CFG_NO_ALLOC_EXTEND};

static __cortn_handle_t *cortn_handle_now = NULL;

_INLINE uint64_t Cortn_Runner(void) {
  if (!cortnlist.num) return UINT64_MAX;
  uint64_t sleep_us = UINT64_MAX;
  uint64_t now = get_sys_us();
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (cortn->hd.state == _CR_STATE_STOPPED) {
      Sch_StopCortn(cortn->name);
      return 0;  // 指针已被释放
    } else if (cortn->hd.state == _CR_STATE_READY) {
      cortn->hd.state = _CR_STATE_RUNNING;  // 就绪态转运行态
      cortn->hd.sleepUntil = 0;
    } else if (cortn->hd.state == _CR_STATE_AWAITING) {
      continue;  // 跳过等待态协程
    }
    cortn_handle_now = &cortn->hd;
    // 检查睡眠态协程或者运行态协程
    if (cortn_handle_now->state == _CR_STATE_RUNNING ||
        (cortn_handle_now->state == _CR_STATE_SLEEPING &&
         now >= cortn_handle_now->sleepUntil)) {
      cortn_handle_now->depth = 0;
      cortn_handle_now->sleepUntil = 0;
#if _SCH_DEBUG_REPORT
      uint64_t _sch_debug_task_tick = get_sys_tick();
      cortn->task(cortn_handle_now, cortn->args);
      _sch_debug_task_tick = get_sys_tick() - _sch_debug_task_tick;
      if (cortn->max_cost < _sch_debug_task_tick)
        cortn->max_cost = _sch_debug_task_tick;
      cortn->total_cost += _sch_debug_task_tick;
#else
      cortn->task(cortn_handle_now, cortn->args);
#endif
      if (cortn_handle_now->data[0].ptr == NULL) {
        cortn_handle_now->state = _CR_STATE_STOPPED;
        cortn_handle_now = NULL;
        sleep_us = 0;
        continue;  // 协程已结束
      }
    }
    // 计算等待时间
    if (cortn_handle_now->sleepUntil < now) {
      sleep_us = 0;  // 直接yield的协程，不休眠
    } else if (cortn_handle_now->sleepUntil - now < sleep_us) {
      sleep_us = cortn_handle_now->sleepUntil - now;
    }
    cortn_handle_now = NULL;
  }
  return sleep_us;
}

uint8_t Sch_RunCortn(const char *name, cortn_func_t func, void *args) {
  scheduler_cortn_t cortn = {
      .name = name,
      .task = func,
      .args = args,
      .hd =
          {
              .state = _CR_STATE_READY,
              .depth = 0,
              .callDepth = 0,
              .sleepUntil = 0,
              .msg = NULL,
              .name = name,
          },
  };
  if (!ulist_init(&cortn.hd.dataList, sizeof(__cortn_data_t), 1,
                  ULIST_CFG_CLEAR_DIRTY_REGION | ULIST_CFG_NO_ALLOC_EXTEND |
                      ULIST_CFG_NO_SHRINK,
                  NULL)) {
    return 0;
  }
  cortn.hd.data = (__cortn_data_t *)cortn.hd.dataList.data;
  cortn.hd.data[0].local = NULL;
  cortn.hd.data[0].ptr = NULL;
  uint16_t __chd_idx;
  if (cortn_handle_now != NULL) {  // 列表添加可能会导致旧指针失效
    __chd_idx = cortn_handle_now - &(((scheduler_cortn_t *)cortnlist.data)->hd);
  }
  if (!ulist_append_copy(&cortnlist, &cortn)) {
    ulist_free(&cortn.hd.dataList);
    return 0;
  }
  if (cortn_handle_now != NULL) {
    cortn_handle_now =
        &ulist_get_ptr(&cortnlist, scheduler_cortn_t, __chd_idx)->hd;
  }
  return 1;
}

scheduler_cortn_t *get_cortn(const char *name) {
  if (cortnlist.num == 0) return NULL;
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      return cortn;
    }
  }
  return NULL;
}

uint8_t Sch_StopCortn(const char *name) {
  scheduler_cortn_t *cortn = get_cortn(name);
  if (cortn == NULL) return 0;
  // 不允许在协程中删除自身
  if (cortn_handle_now == &cortn->hd) return 0;
  ulist_foreach(&cortn->hd.dataList, __cortn_data_t, data) {
    if (data->local != NULL) m_free(data->local);
  }
  ulist_free(&cortn->hd.dataList);
  ulist_remove(&cortnlist, cortn);
  return 1;
}

uint16_t Sch_GetCortnNum(void) { return cortnlist.num; }

uint8_t Sch_IsCortnRunning(const char *name) { return get_cortn(name) != NULL; }

uint8_t Sch_IsCortnWaitingMsg(const char *name) {
  scheduler_cortn_t *cortn = get_cortn(name);
  if (cortn == NULL) return 0;
  if (cortn->hd.state == _CR_STATE_STOPPED) return 0;
  return cortn->hd.state == _CR_STATE_AWAITING;
}

uint8_t Sch_SendMsgToCortn(const char *name, void *msg) {
  scheduler_cortn_t *cortn = get_cortn(name);
  if (cortn == NULL) return 0;
  if (cortn->hd.state == _CR_STATE_STOPPED) return 0;
  if (msg != NULL) cortn->hd.msg = msg;
  cortn->hd.state = _CR_STATE_READY;
  return 1;
}

/**
 * @brief (内部函数)获取当前协程名
 * @return 协程名
 */
_INLINE const char *__Internal_GetName(void) {
  static const char main_name[] = "__main__";
  if (cortn_handle_now == NULL) return main_name;
  return cortn_handle_now->name;
}

/**
 * @brief (内部函数)初始化协程本地变量存储区指针
 * @param  size 存储区大小
 * @return 存储区指针
 */
_INLINE void *__Internal_InitLocal(size_t size) {
  if (size == 0) return NULL;
  if (cortn_handle_now->data[cortn_handle_now->depth].local == NULL) {
    // 初始化局部变量存储区
    cortn_handle_now->data[cortn_handle_now->depth].local = m_alloc(size);
    if (cortn_handle_now->data[cortn_handle_now->depth].local == NULL)
      return NULL;
    memset(cortn_handle_now->data[cortn_handle_now->depth].local, 0, size);
  }
  return cortn_handle_now->data[cortn_handle_now->depth].local;
}

/**
 * @brief (内部函数)协程嵌套调用准备
 * @return 是否允许进行调用
 */
_INLINE uint8_t __Internal_AwaitEnter(void) {
  cortn_handle_now->depth++;
  if (cortn_handle_now->depth > cortn_handle_now->callDepth) {
    // 嵌套层级+1
    if (!ulist_append(&cortn_handle_now->dataList)) {
      cortn_handle_now->depth--;
      return 0;
    }
    // 更新指针，初始化
    cortn_handle_now->data = cortn_handle_now->dataList.data;
    cortn_handle_now->data[cortn_handle_now->depth].local = NULL;
    cortn_handle_now->data[cortn_handle_now->depth].ptr = NULL;
    cortn_handle_now->callDepth++;
  }
  return 1;
}

/**
 * @brief (内部函数)协程嵌套调用返回
 * @return 嵌套协程已结束
 */
_INLINE uint8_t __Internal_AwaitReturn(void) {
  cortn_handle_now->depth--;
  if (cortn_handle_now->data[cortn_handle_now->depth + 1].ptr != NULL) {
    // 嵌套协程未结束
    return 0;
  }
  if (cortn_handle_now->data[cortn_handle_now->depth + 1].local != NULL) {
    // 释放局部变量存储区
    m_free(cortn_handle_now->data[cortn_handle_now->depth + 1].local);
    cortn_handle_now->data[cortn_handle_now->depth + 1].local = NULL;
  }
  // 嵌套层级-1
  ulist_delete(&cortn_handle_now->dataList, -1);
  cortn_handle_now->data = cortn_handle_now->dataList.data;
  cortn_handle_now->callDepth--;
  return 1;
}

/**
 * @brief (内部函数)协程延时
 * @param  delayUs 延时时间(us)
 */
_INLINE void __Internal_Delay(uint64_t delayUs) {
  cortn_handle_now->sleepUntil = get_sys_us() + delayUs;
  cortn_handle_now->state = _CR_STATE_SLEEPING;
}

/**
 * @brief (内部函数)协程消息等待
 * @param  msgPtr 消息指针
 */
_INLINE void __Internal_AwaitMsg(__async__, void **msgPtr) {
  ASYNC_NOLOCAL
  if (__chd__->msg == NULL) {
    __chd__->state = _CR_STATE_AWAITING;
    YIELD();
  }
  if (msgPtr != NULL) *msgPtr = __chd__->msg;
  __chd__->msg = NULL;
}

_STATIC_INLINE scheduler_cortn_mutex_t *get_mutex(const char *name) {
  ulist_foreach(&mutexlist, scheduler_cortn_mutex_t, mutex) {
    if (fast_strcmp(mutex->name, name)) {
      return mutex;
    }
  }
  scheduler_cortn_mutex_t *ret = ulist_append(&mutexlist);
  if (ret == NULL) return NULL;
  ret->name = name;
  ret->locked = 0;
  ulist_init(&ret->waitlist, sizeof(char *), 0, NULL, NULL);
  return ret;
}

/**
 * @brief (内部函数)协程互斥锁获取
 * @param  name 锁名
 * @return 1: 获取成功跳过等待, 0: 需要等待
 */
_INLINE uint8_t __Internal_AcquireMutex(const char *name) {
  scheduler_cortn_mutex_t *mutex = get_mutex(name);
  if (mutex == NULL) return 0;
  if (mutex->locked) {  // 锁已被占用, 添加到等待队列
    const char **ptr = ulist_append(&mutex->waitlist);
    if (ptr == NULL) return 0;
    *ptr = cortn_handle_now->name;
    return 0;
  } else {  // 锁未被占用, 直接占用
    mutex->locked = 1;
    return 1;
  }
}

/**
 * @brief (内部函数)协程互斥锁释放
 * @param  name 锁名
 */
_INLINE void __Internal_ReleaseMutex(const char *name) {
  scheduler_cortn_t *cortn = NULL;
  scheduler_cortn_mutex_t *mutex = get_mutex(name);
  if (mutex == NULL) return;
  do {
    if (mutex->waitlist.num) {  // 等待队列不为空, 唤醒第一个协程
      const char **ptr = ulist_get(&mutex->waitlist, 0);
      cortn = get_cortn(*ptr);
      if (cortn != NULL) cortn->hd.state = _CR_STATE_READY;
      ulist_delete(&mutex->waitlist, 0);
    } else {  // 等待队列为空, 释放锁
      mutex->locked = 0;
      cortn = (scheduler_cortn_t *)0x01;
    }
  } while (cortn == NULL);
}

_STATIC_INLINE scheduler_cortn_barrier_t *get_barrier(const char *name,
                                                      uint8_t create) {
  ulist_foreach(&barrierlist, scheduler_cortn_barrier_t, barrier) {
    if (fast_strcmp(barrier->name, name)) {
      return barrier;
    }
  }
  if (!create) return NULL;
  scheduler_cortn_barrier_t *ret = ulist_append(&barrierlist);
  if (ret == NULL) return NULL;
  ret->name = name;
  ret->target = 0xffff;
  ulist_init(&ret->waitlist, sizeof(char *), 0, NULL, NULL);
  return ret;
}

_STATIC_INLINE void release_barrier(scheduler_cortn_barrier_t *barrier) {
  scheduler_cortn_t *cortn;
  if (barrier->waitlist.num) {  // 等待队列不为空, 唤醒所有协程
    ulist_foreach(&barrier->waitlist, const char *, ptr) {
      cortn = get_cortn(*ptr);
      if (cortn == NULL) continue;
      cortn->hd.state = _CR_STATE_READY;
    }
    ulist_clear(&barrier->waitlist);
  }
}

uint8_t Sch_CortnBarrierRelease(const char *name) {
  scheduler_cortn_barrier_t *barrier = get_barrier(name, 0);
  if (barrier == NULL) return 0;
  release_barrier(barrier);
  return 1;
}

uint8_t Sch_SetCortnBarrierTarget(const char *name, uint16_t target) {
  scheduler_cortn_barrier_t *barrier = get_barrier(name, 1);
  if (barrier == NULL) return 0;
  barrier->target = target;
  if (barrier->waitlist.num >= target) {
    release_barrier(barrier);
  }
  return 1;
}

uint16_t Sch_GetCortnBarrierWaitingNum(const char *name) {
  scheduler_cortn_barrier_t *barrier = get_barrier(name, 0);
  if (barrier == NULL) return 0;
  return barrier->waitlist.num;
}

/**
 * @brief (内部函数)协程等待屏障
 * @param  name 屏障名
 * @return 1: 到达屏障, 0: 等待屏障
 */
uint8_t __Internal_WaitBarrier(const char *name) {
  scheduler_cortn_barrier_t *barrier = get_barrier(name, 1);
  if (barrier == NULL) return 0;
  if (barrier->waitlist.num + 1 >= barrier->target) {
    release_barrier(barrier);
    return 1;
  }
  const char **ptr = ulist_append(&barrier->waitlist);
  if (ptr == NULL) return 0;
  *ptr = cortn_handle_now->name;
  return 0;
}

static const char *get_cortn_state_str(uint8_t state) {
  switch (state) {
    case _CR_STATE_STOPPED:
      return "stopped";
    case _CR_STATE_READY:
      return "ready";
    case _CR_STATE_RUNNING:
      return "running";
    case _CR_STATE_AWAITING:
      return "await";
    case _CR_STATE_SLEEPING:
      return "sleep";
    default:
      return "unknown";
  }
}

#if _SCH_DEBUG_REPORT
void sch_cortn_add_debug(TT tt, uint64_t period, uint64_t *other) {
  if (cortnlist.num) {
    TT_FMT1 f1 = TT_FMT1_BLUE;
    TT_FMT2 f2 = TT_FMT2_BOLD;
    TT_ALIGN al = TT_ALIGN_LEFT;
    TT_AddTitle(
        tt,
        TT_FmtStr(al, f1, f2, "[ Coroutine Report / %d ]", Sch_GetCortnNum()),
        '-');
    TT_ITEM_GRID grid = TT_AddGrid(tt, 0);
    TT_ITEM_GRID_LINE line =
        TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
    const char *head3[] = {"No", "State", "Depth", "Tmax", "Usage", "Name"};
    for (int i = 0; i < sizeof(head3) / sizeof(char *); i++)
      TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head3[i]));
    int i = 0;
    ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
      if (i >= _SCH_DEBUG_MAXLINE) {
        TT_AddString(
            tt, TT_Str(TT_ALIGN_CENTER, TT_FMT1_NONE, TT_FMT2_NONE, "..."), 0);
        break;
      }
      line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " "));
      float usage = (float)cortn->total_cost / period * 100;
      f1 = TT_FMT1_GREEN;
      f2 = TT_FMT2_NONE;
      TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
      TT_GridLine_AddItem(
          line, TT_Str(al, f1, f2, get_cortn_state_str(cortn->hd.state)));
      TT_GridLine_AddItem(line,
                          TT_FmtStr(al, f1, f2, "%d", cortn->hd.callDepth));
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
  if (embeddedCliCheckToken(args, "-l", 1)) {
    LOG_RAWLN(
        T_FMT(T_BOLD, T_GREEN) "Coroutines list:" T_FMT(T_RESET, T_GREEN));
    ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
      LOG_RAWLN("  %s: %p depth:%d state:%s", cortn->name, cortn->task,
                cortn->hd.callDepth, get_cortn_state_str(cortn->hd.state));
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
  if (embeddedCliCheckToken(args, "-s", 1)) {
    Sch_StopCortn(name);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Coroutine: %s stopped" T_RST, name);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
#endif  // _SCH_ENABLE_TERMINAL

#endif  // _SCH_ENABLE_COROUTINE

#include "scheduler.h"

#include <string.h>

#include "log.h"
#include "ulist.h"

#define _INLINE inline __attribute__((always_inline))
#define _STATIC_INLINE static _INLINE

_STATIC_INLINE void Event_Runner(void);
_STATIC_INLINE void SoftInt_Runner(void);
_STATIC_INLINE uint8_t DebugInfo_Runner(uint64_t sleep_us);
_STATIC_INLINE uint64_t Task_Runner(void);
_STATIC_INLINE uint64_t Cortn_Runner(void);
_STATIC_INLINE uint64_t CallLater_Runner(void);

_STATIC_INLINE uint8_t fast_strcmp(const char *str1, const char *str2) {
  while (*str1 && *str2) {
    if (*str1++ != *str2++) return 0;
  }
  return (!*str1) && (!*str2);
}
_STATIC_INLINE uint64_t get_sys_tick(void) { return (uint64_t)m_tick(); }
_STATIC_INLINE uint64_t get_sys_clock(void) { return SystemCoreClock; }
_STATIC_INLINE float tick_to_us(uint64_t tick) {
  static float tick2us = 0;
  if (!tick2us) tick2us = (float)1000000 / (float)get_sys_clock();
  return tick2us * (float)tick;
}
_STATIC_INLINE uint64_t us_to_tick(float us) {
  static uint64_t us2tick = 0;
  if (!us2tick) us2tick = get_sys_clock() / 1000000;
  return us * us2tick;
}
_STATIC_INLINE uint64_t get_sys_us(void) {
  static uint64_t div = 0;
  if (!div) div = get_sys_clock() / 1000000;
  return get_sys_tick() / div;
}

__weak void Scheduler_Idle_Callback(uint64_t idleTimeUs) {
  if (idleTimeUs > 100000) idleTimeUs = 100000;  // 限幅
#if _MOD_USE_OS > 0  // 如果使用操作系统, 则直接释放CPU
  m_delay_us(idleTimeUs);
#else  // 简单的低功耗实现
  uint64_t start = get_sys_us();
  while (get_sys_us() - start < idleTimeUs) {
    __WFI();  // 关闭CPU直到下一次systick中断
  }
#endif
}

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
    if (mslp == UINT64_MAX) mslp = 100;  // 没有任何任务
#if _SCH_DEBUG_REPORT
    if (DebugInfo_Runner(mslp)) continue;
#endif
    if (block && mslp) {
      Scheduler_Idle_Callback(mslp);
    }
  } while (block);
  return mslp;
}

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
                           .isize = sizeof(scheduler_task_t),
                           .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

static int taskcmp(const void *a, const void *b) {
  int priority1 = ((scheduler_task_t *)a)->priority;
  int priority2 = ((scheduler_task_t *)b)->priority;
  return priority2 - priority1;
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
}

_STATIC_INLINE uint64_t Task_Runner(void) {
  if (!tasklist.num) return UINT64_MAX;
  uint64_t latency;
  uint64_t now;
  uint64_t sleep_us = UINT64_MAX;
#if _SCH_DEBUG_REPORT
  __IO const char *old_name;
  __IO uint16_t p_idx;
#endif  // _SCH_DEBUG_REPORT
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
        old_name = task->name;
        p_idx = task - (scheduler_task_t *)tasklist.data;
        uint64_t _sch_debug_task_tick = get_sys_tick();
        task->task(task->args);
        _sch_debug_task_tick = get_sys_tick() - _sch_debug_task_tick;
        task = (scheduler_task_t *)tasklist.data + p_idx;
        if (task->name != old_name) {
          LOG_W("Task list changed during excute");
          return 0;
        }  // 列表已改变
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
  p->period = (double)get_sys_clock() / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = get_sys_tick() - p->period;
  p->name = name;
  p->args = args;
  resort_task();
  return 1;
}

uint8_t Sch_DeleteTask(const char *name) {
  if (tasklist.num == 0) return 0;
  ulist_foreach(&tasklist, scheduler_task_t, task) {
    if (fast_strcmp(task->name, name)) {
      ulist_delete(&tasklist, task - (scheduler_task_t *)tasklist.data);
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
  p->period = (double)get_sys_clock() / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = get_sys_tick() - p->period;
  return 1;
}
#endif  // _SCH_ENABLE_TASK

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
                            .isize = sizeof(scheduler_event_t),
                            .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};
static uint8_t event_modified = 0;

_STATIC_INLINE void Event_Runner(void) {
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
#if !_SCH_EVENT_ALLOW_DUPLICATE
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) return 0;
  }
#endif  // !_SCH_EVENT_ALLOW_DUPLICATE
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

uint8_t Sch_DeleteEvent(const char *name) {
  if (eventlist.num == 0) return 0;
  uint8_t ret = 0;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      if (event->args != NULL && event->needFree) m_free(event->args);
      ulist_delete(&eventlist, event - (scheduler_event_t *)eventlist.data);
      event_end--;
      event--;
      ret = 1;
      event_modified = 1;
#if !_SCH_EVENT_ALLOW_DUPLICATE
      break;
#endif  // !_SCH_EVENT_ALLOW_DUPLICATE
      continue;
    }
  }
  return ret;
}

uint8_t Sch_SetEventState(const char *name, uint8_t enable) {
  uint8_t ret = 0;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      event->enable = enable;
      event->trigger = 0;
      ret = 1;
#if !_SCH_EVENT_ALLOW_DUPLICATE
      break;
#endif  // !_SCH_EVENT_ALLOW_DUPLICATE
    }
  }
  return ret;
}

uint8_t Sch_TriggerEvent(const char *name, void *args) {
  uint8_t ret = 0;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (event->enable && fast_strcmp(event->name, name)) {
      event->trigger = 0;
      if (event->args != NULL && event->needFree) m_free(event->args);
      event->args = args;
      event->needFree = 0;
#if _SCH_DEBUG_REPORT
      event->trigger_time = get_sys_tick();
      event->trigger_cnt++;
#endif
      event->trigger = 1;
#if !_SCH_EVENT_ALLOW_DUPLICATE
      break;
#endif
    }
  }
  return ret;
}

uint8_t Sch_TriggerEventEx(const char *name, const void *arg_ptr,
                           uint16_t arg_size) {
  uint8_t ret = 0;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (event->enable && fast_strcmp(event->name, name)) {
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
#if !_SCH_EVENT_ALLOW_DUPLICATE
      break;
#endif
    }
  }
  return ret;
}

uint8_t Sch_IsEventExist(const char *name) {
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      return 1;
    }
  }
  return 0;
}

uint8_t Sch_GetEventState(const char *name) {
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      return event->enable;
    }
  }
  return 0;
}

uint16_t Sch_GetEventNum(void) { return eventlist.num; }
#endif  // _SCH_ENABLE_EVENT

#if _SCH_ENABLE_COROUTINE
#pragma pack(1)
typedef struct {        // 协程任务结构
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
  const char *name;  // 协程名
} scheduler_cortn_t;
#pragma pack()

static ulist_t cortnlist = {.data = NULL,
                            .cap = 0,
                            .num = 0,
                            .isize = sizeof(scheduler_cortn_t),
                            .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

__cortn_handle_t *__chd = {0};
uint16_t __chd_idx = 0;
static uint8_t cr_modified = 0;

_STATIC_INLINE uint64_t Cortn_Runner(void) {
  if (!cortnlist.num) return UINT64_MAX;
  cr_modified = 0;
  uint64_t sleep_us = UINT64_MAX;
  uint64_t now = get_sys_us();
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (!cortn->enable || cortn->hd.yieldUntil) continue;  // 跳过禁用协程
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
  p->hd.dataList.cfg = ULIST_CFG_CLEAR_DIRTY_REGION | ULIST_CFG_NO_ALLOC_EXTEND;
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
 * @brief (内部函数)释放协程本地变量存储区指针
 */
_INLINE void __Sch_CrFreeLocal(void) {
  if (__chd->data[__chd->depth].local != NULL) {
    m_free(__chd->data[__chd->depth].local);
    __chd->data[__chd->depth].local = NULL;
  }
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

#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_CALLLATER
#pragma pack(1)
typedef struct {       // 延时调用任务结构
  sch_func_t task;     // 任务函数指针
  uint64_t runTimeUs;  // 执行时间(us)
  void *args;          // 任务参数
} scheduler_call_later_t;
#pragma pack()

static ulist_t clist = {.data = NULL,
                        .cap = 0,
                        .num = 0,
                        .isize = sizeof(scheduler_call_later_t),
                        .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

_STATIC_INLINE uint64_t CallLater_Runner(void) {
  if (!clist.num) return UINT64_MAX;
  uint64_t sleep_us = UINT64_MAX;
  uint64_t now = get_sys_us();
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (now >= callLater_p->runTimeUs) {
      callLater_p->task(callLater_p->args);
      ulist_delete(&clist, callLater_p - (scheduler_call_later_t *)clist.data);
      return 0;  // 有任务被执行，不确定
    }
    if (callLater_p->runTimeUs - now < sleep_us) {
      sleep_us = callLater_p->runTimeUs - now;
    }
  }
  return sleep_us;
}

uint8_t Sch_CallLater(sch_func_t func, uint64_t delayUs, void *args) {
  scheduler_call_later_t *p = (scheduler_call_later_t *)ulist_append(&clist);
  if (p == NULL) return 0;
  p->task = func;
  p->runTimeUs = delayUs + get_sys_us();
  p->args = args;
  return 1;
}

void Sch_CancelCallLater(sch_func_t func) {
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (callLater_p->task == func) {
      ulist_delete(&clist, callLater_p - (scheduler_call_later_t *)clist.data);
      return;
    }
  }
}
#endif  // _SCH_ENABLE_CALLLATER

#if _SCH_ENABLE_SOFTINT

static __IO uint8_t imm = 0;
static __IO uint8_t ism[8] = {0};

_INLINE void Sch_TriggerSoftInt(uint8_t mainChannel, uint8_t subChannel) {
  if (mainChannel > 7 || subChannel > 7) return;
  imm |= 1 << mainChannel;
  ism[mainChannel] |= 1 << subChannel;
}

__weak void Scheduler_SoftInt_Handler(uint8_t mainChannel, uint8_t subMask) {
  LOG_LIMIT(100, "SoftInt %d:%d", mainChannel, subMask);
}

_STATIC_INLINE void SoftInt_Runner(void) {
  if (imm) {
    uint8_t _ism;
    for (uint8_t i = 0; i < 8; i++) {
      if (imm & (1 << i)) {
        _ism = ism[i];
        ism[i] = 0;
        imm &= ~(1 << i);
        Scheduler_SoftInt_Handler(i, _ism);
      }
    }
  }
}

#endif  // _SCH_ENABLE_SOFTINT

#if _SCH_DEBUG_REPORT
#warning Scheduler Debug-Report is on, expect performance degradation and increased memory usage of task handles
#include "lwmem.h"
#include "term_table.h"

_STATIC_INLINE uint8_t DebugInfo_Runner(uint64_t sleep_us) {
  static uint8_t first_print = 1;
  static uint64_t last_print = 0;
  static uint64_t sleep_sum = 0;
  static uint16_t sleep_cnt = 0;
  float usage;
  uint16_t i;
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
    TT_ITEM_GRID grid;
    TT_ITEM_GRID_LINE line;
    TT_FMT1 f1;
    TT_FMT2 f2;
    TT_ALIGN al;
#if _SCH_ENABLE_TASK
    if (tasklist.num) {
      f1 = TT_FMT1_BLUE;
      f2 = TT_FMT2_BOLD;
      al = TT_ALIGN_LEFT;
      TT_AddTitle(tt, TT_Str(al, f1, f2, "[ Task Report ]"), '-');
      grid = TT_AddGrid(tt, 0);
      line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
      const char *head1[] = {"No",    "Pri",   "Run",   "Tmax",
                             "Usage", "LTavg", "LTmax", "Name"};
      for (int i = 0; i < sizeof(head1) / sizeof(char *); i++)
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head1[i]));
      i = 0;
      ulist_foreach(&tasklist, scheduler_task_t, task) {
        line = TT_Grid_AddLine(grid, TT_Str(al, f1, f2, " "));
        if (task->enable) {
          usage = (float)task->total_cost / period * 100;
          f1 = TT_FMT1_GREEN;
          f2 = TT_FMT2_NONE;
          TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
          TT_GridLine_AddItem(line,
                              TT_FmtStr(al, f1, f2, "%d", task->priority));

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
          other -= task->total_cost;
        } else {
          f1 = TT_FMT1_WHITE;
          f2 = TT_FMT2_NONE;
          TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
          TT_GridLine_AddItem(line,
                              TT_FmtStr(al, f1, f2, "%d", task->priority));
          TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
          TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
          TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
          TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
          TT_GridLine_AddItem(line, TT_Str(al, f1, f2, "-"));
          TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%s", task->name));
          task->last_usage = 0;
        }
        i++;
      }
    }
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
    if (eventlist.num) {
      f1 = TT_FMT1_BLUE;
      f2 = TT_FMT2_BOLD;
      al = TT_ALIGN_LEFT;
      TT_AddTitle(tt, TT_Str(al, f1, f2, "[ Event Report ]"), '-');
      grid = TT_AddGrid(tt, 0);
      line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
      const char *head2[] = {"No",    "Tri",   "Run",   "Tmax",
                             "Usage", "LTavg", "LTmax", "Event"};
      for (int i = 0; i < sizeof(head2) / sizeof(char *); i++)
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head2[i]));
      i = 0;
      ulist_foreach(&eventlist, scheduler_event_t, event) {
        line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " "));
        if (event->enable && event->run_cnt) {
          usage = (float)event->total_cost / period * 100;
          f1 = TT_FMT1_GREEN;
          f2 = TT_FMT2_NONE;
          TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
          TT_GridLine_AddItem(line,
                              TT_FmtStr(al, f1, f2, "%d", event->trigger_cnt));
          TT_GridLine_AddItem(line,
                              TT_FmtStr(al, f1, f2, "%d", event->run_cnt));
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
          other -= event->total_cost;
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
          TT_GridLine_AddItem(line,
                              TT_FmtStr(al, f1, f2, "%d", event->run_cnt));
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
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
    if (cortnlist.num) {
      f1 = TT_FMT1_BLUE;
      f2 = TT_FMT2_BOLD;
      al = TT_ALIGN_LEFT;
      TT_AddTitle(tt, TT_Str(al, f1, f2, "[ Coroutine Report ]"), '-');
      grid = TT_AddGrid(tt, 0);
      line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " | "));
      const char *head3[] = {"No", "Depth", "Tmax", "Usage", "Name"};
      for (int i = 0; i < sizeof(head3) / sizeof(char *); i++)
        TT_GridLine_AddItem(line, TT_Str(al, f1, f2, head3[i]));
      i = 0;
      ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
        line = TT_Grid_AddLine(grid, TT_Str(TT_ALIGN_CENTER, f1, f2, " "));
        if (cortn->enable) {
          usage = (float)cortn->total_cost / period * 100;
          f1 = TT_FMT1_GREEN;
          f2 = TT_FMT2_NONE;
          TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
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
          other -= cortn->total_cost;
        } else {
          f1 = TT_FMT1_WHITE;
          f2 = TT_FMT2_NONE;
          TT_GridLine_AddItem(line, TT_FmtStr(al, f1, f2, "%d", i));
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
#endif  // _SCH_ENABLE_COROUTINE
    TT_AddTitle(
        tt,
        TT_Str(TT_ALIGN_LEFT, TT_FMT1_BLUE, TT_FMT2_BOLD, "[ System Info ]"),
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
    TT_LineBreak();
    TT_Print(tt);
    TT_FreeTable(tt);
  }
  now = get_sys_tick() - now;  // 补偿打印LOG的时间
#if _SCH_ENABLE_TASK
  ulist_foreach(&tasklist, scheduler_task_t, task) {
    if (first_print)
      task->lastRun = get_sys_tick();
    else
      task->lastRun += now;
    task->max_cost = 0;
    task->total_cost = 0;
    task->run_cnt = 0;
    task->max_lat = 0;
    task->total_lat = 0;
    task->unsync = 0;
  }
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (first_print)
      event->trigger_time = get_sys_tick();
    else
      event->trigger_time += now;
    event->max_cost = 0;
    event->total_cost = 0;
    event->run_cnt = 0;
    event->trigger_cnt = 0;
    event->max_lat = 0;
    event->total_lat = 0;
    event->trigger = 0;
  }
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    cortn->max_cost = 0;
    cortn->total_cost = 0;
  }
#endif
  last_print = get_sys_tick();
  first_print = 0;
  return 1;
}
#endif  // _SCH_DEBUG_REPORT

#if _SCH_ENABLE_TERMINAL
#include "lwmem.h"
#include "stdlib.h"
#include "string.h"

#if _SCH_ENABLE_TASK
static void task_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (embeddedCliCheckToken(args, "list", 1)) {
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Tasks list:" T_FMT(T_RESET, T_GREEN));
    ulist_foreach(&tasklist, scheduler_task_t, task) {
      LOG_RAWLN("  %s: 0x%p pri:%d freq:%.1f en:%d", task->name, task->task,
                task->priority, (float)get_sys_clock() / task->period,
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
#endif  // _SCH_ENABLE_TASK

#if _SCH_ENABLE_EVENT
static void event_cmd_func(EmbeddedCli *cli, char *args, void *context) {
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
#endif  // _SCH_ENABLE_EVENT

#if _SCH_ENABLE_COROUTINE
static void cortn_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (embeddedCliCheckToken(args, "list", 1)) {
    LOG_RAWLN(
        T_FMT(T_BOLD, T_GREEN) "Coroutines list:" T_FMT(T_RESET, T_GREEN));
    ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
      LOG_RAWLN("  %s: 0x%p en:%d", cortn->name, cortn->task, cortn->enable);
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
#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_SOFTINT
static void softint_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (argc < 2) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "2 parameters are required" T_RST);
    return;
  }
  uint8_t ch = atoi(embeddedCliGetToken(args, 1));
  uint8_t sub = atoi(embeddedCliGetToken(args, 2));
  if (ch > 7 || sub > 7) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "(Sub-)Channel must be 0~7" T_RST);
    return;
  }
  Sch_TriggerSoftInt(ch, sub);
  LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "SoftInt: %d-%d triggered" T_RST, ch, sub);
}
#endif  // _SCH_ENABLE_SOFTINT

#include "term_table.h"

static void sysinfo_print(void *arg) {
  if (arg != NULL) LOG_RAWLN();
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
      TT_FmtStr(al, f1, f2, "%.3f Mhz", (float)get_sys_clock() / 1000000), sep);
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
                    TT_FmtStr(al, f1, f2, "%d", tasklist.num), sep);
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Event Num"),
                    TT_FmtStr(al, f1, f2, "%d", eventlist.num), sep);
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Coroutine Num"),
                    TT_FmtStr(al, f1, f2, "%d", cortnlist.num), sep);
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
  TT_KVPair_AddItem(kv, 2, TT_Str(al, f1, f2, "Used"),
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

static void sysinfo_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    sysinfo_print(NULL);
    return;
  }
#if _SCH_ENABLE_TASK
  float freq = 2;
  if (argc > 1) {
    freq = atof(embeddedCliGetToken(args, 2));
  }
  if (embeddedCliCheckToken(args, "start", 1)) {
    if (Sch_IsTaskExist("sysinfo")) {
      LOG_RAWLN(
          T_FMT(T_BOLD, T_YELLOW) "System Info printing already started" T_RST);
      return;
    }
    Sch_CreateTask("sysinfo", sysinfo_print, freq, 1, 0, (void *)1);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "System Info printing started" T_RST);

  } else if (embeddedCliCheckToken(args, "stop", 1)) {
    Sch_DeleteTask("sysinfo");
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "System Info printing stopped" T_RST);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
#else
  LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Task module is not supported" T_RST);
#endif
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
      .usage = "sysinfo [start|stop] [freq]",
      .help = "Show system info (Scheduler)",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = sysinfo_cmd_func,
  };

  embeddedCliAddBinding(cli, sysinfo_cmd);
}
#endif  // _SCH_ENABLE_TERMINAL

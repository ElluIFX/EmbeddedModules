#include "scheduler.h"

#include <string.h>

#include "log.h"
#include "ulist.h"

#define _INLINE inline __attribute__((always_inline))
#define _STATIC_INLINE static _INLINE

_STATIC_INLINE void Event_Runner(void);
_STATIC_INLINE void SoftInt_Runner(void);
_STATIC_INLINE bool DebugInfo_Runner(void);
_STATIC_INLINE m_time_t Task_Runner(void);
_STATIC_INLINE m_time_t Cortn_Runner(void);
_STATIC_INLINE m_time_t CallLater_Runner(void);

_STATIC_INLINE bool fast_strcmp(const char *str1, const char *str2) {
  while (*str1 && *str2) {
    if (*str1++ != *str2++) return false;
  }
  return (!*str1) && (!*str2);
}

#define SCH_LARGE_TIME 0x7FFFFFFF

__weak void Scheduler_Idle_Callback(m_time_t idleTimeUs) {
  if (idleTimeUs <= 0) return;
  if (idleTimeUs > 1000000) idleTimeUs = 1000000;  // 限制最大休眠时间为1s
#if _MOD_USE_OS > 0  // 如果使用操作系统, 则直接释放CPU
  m_delay_us(idleTimeUs);
#else  // 简单的低功耗实现
  m_time_t start = m_time_us();
  while (m_time_us() - start < idleTimeUs) {
    __WFI();  // 关闭CPU直到下一次systick中断
  }
#endif
}

m_time_t _INLINE Scheduler_Run(const uint8_t block) {
  m_time_t mslp, rslp;
  do {
    mslp = SCH_LARGE_TIME;
#if _SCH_ENABLE_SOFTINT
    SoftInt_Runner();
#endif
#if _SCH_ENABLE_TASK
    rslp = Task_Runner();
    if (rslp < mslp) mslp = rslp;
#endif
#if _SCH_ENABLE_COROUTINE
    rslp = Cortn_Runner();
    if (rslp < mslp) mslp = rslp;
#endif
#if _SCH_ENABLE_CALLLATER
    rslp = CallLater_Runner();
    if (rslp < mslp) mslp = rslp;
#endif
#if _SCH_ENABLE_EVENT
    Event_Runner();
#endif
#if _SCH_DEBUG_REPORT
    if (DebugInfo_Runner()) continue;
#endif
    if (block && mslp > 0) {
      Scheduler_Idle_Callback(mslp - m_time_us());
    }
  } while (block);
  return mslp;
}

#if _SCH_ENABLE_TASK

#pragma pack(1)
typedef struct {     // 用户任务结构
  sch_func_t task;   // 任务函数指针
  m_time_t period;   // 任务调度周期(Tick)
  m_time_t lastRun;  // 上次执行时间(Tick)
  uint8_t enable;    // 是否使能
  uint8_t priority;  // 优先级
  void *args;        // 任务参数
#if _SCH_DEBUG_REPORT
  m_time_t max_cost;    // 任务最大执行时间(Tick)
  m_time_t total_cost;  // 任务总执行时间(Tick)
  m_time_t max_lat;     // 任务调度延迟(Tick)
  m_time_t total_lat;   // 任务调度延迟总和(Tick)
  uint16_t run_cnt;     // 任务执行次数
  float last_usage;     // 任务上次执行占用率
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

_STATIC_INLINE m_time_t Task_Runner(void) {
  m_time_t latency;
  m_time_t now;
  m_time_t sleep_tick = SCH_LARGE_TIME;
#if _SCH_DEBUG_REPORT
  __IO const char *old_name;
#endif  // _SCH_DEBUG_REPORT
  if (tasklist.num) {
    now = m_tick();
    ulist_foreach(&tasklist, scheduler_task_t, task) {
      if (!task->enable) continue;  // 跳过禁用任务
      if (now >= task->lastRun + task->period) {
        latency = now - (task->lastRun + task->period);
        if (latency <= _SCH_COMP_RANGE)
          task->lastRun += task->period;
        else
          task->lastRun = now;
#if _SCH_DEBUG_REPORT
        old_name = task->name;
        m_time_t _sch_debug_task_tick = m_tick();
        task->task(task->args);
        _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
        if (task->name != old_name) {  // 任务已被修改, 重新查找
          task = get_task((const char *)old_name);
          if (task == NULL) return 0;  // 任务已被删除
        }
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
      }
      // 计算最小休眠时间
      latency = (task->lastRun + task->period) / m_tick_per_us(m_time_t);
      if (latency < sleep_tick) sleep_tick = latency;
    }
  }
  return sleep_tick;
}

bool Sch_CreateTask(const char *name, sch_func_t func, float freqHz,
                    uint8_t enable, uint8_t priority, void *args) {
  scheduler_task_t *p = (scheduler_task_t *)ulist_append(&tasklist, 1);
  if (p == NULL) return false;
  p->task = func;
  p->enable = enable;
  p->priority = priority;
  p->period = m_tick_clk(double) / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = m_tick() - p->period;
  p->name = name;
  p->args = args;
  resort_task();
  return true;
}

bool Sch_DeleteTask(const char *name) {
  if (tasklist.num == 0) return false;
  ulist_foreach(&tasklist, scheduler_task_t, task) {
    if (fast_strcmp(task->name, name)) {
      ulist_delete(&tasklist, task - (scheduler_task_t *)tasklist.data, 1);
      return true;
    }
  }
  return false;
}

bool Sch_IsTaskExist(const char *name) { return get_task(name) != NULL; }

bool Sch_SetTaskPriority(const char *name, uint8_t priority) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return false;
  p->priority = priority;
  resort_task();
  return true;
}

bool Sch_DelayTask(const char *name, m_time_t delayUs, uint8_t fromNow) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return false;
  if (fromNow)
    p->lastRun = delayUs * m_tick_per_us(m_time_t) + m_tick();
  else
    p->lastRun += delayUs * m_tick_per_us(m_time_t);
  return true;
}

uint16_t Sch_GetTaskNum(void) { return tasklist.num; }

bool Sch_SetTaskState(const char *name, uint8_t enable) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return false;
  if (enable == TOGGLE)
    p->enable = !p->enable;
  else
    p->enable = enable;
  if (p->enable) p->lastRun = m_tick() - p->period;
  return true;
}

bool Sch_SetTaskFreq(const char *name, float freqHz) {
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return false;
  p->period = m_tick_clk(double) / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = m_tick() - p->period;
  return true;
}
#endif  // _SCH_ENABLE_TASK

#if _SCH_ENABLE_EVENT
#pragma pack(1)
typedef struct {    // 事件任务结构
  sch_func_t task;  // 任务函数指针
  uint8_t enable;   // 是否使能
  uint8_t trigger;  // 是否已触发
  void *args;       // 任务参数
#if _SCH_DEBUG_REPORT
  m_time_t trigger_time;  // 触发时间(Tick)
  m_time_t max_cost;      // 任务最大执行时间(Tick)
  m_time_t total_cost;    // 任务总执行时间(Tick)
  m_time_t max_lat;       // 任务调度延迟(Tick)
  m_time_t total_lat;     // 任务调度延迟总和(Tick)
  uint16_t run_cnt;       // 任务执行次数
  uint16_t trigger_cnt;   // 触发次数
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
  event_modified = 0;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (event->enable && event->trigger) {
      event->trigger = 0;
#if !_SCH_DEBUG_REPORT
      event->task(event->args);
#else
      m_time_t _sch_debug_task_tick = m_tick();
      m_time_t _late = _sch_debug_task_tick - event->trigger_time;
      event->task(event->args);
      _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
      if (event->max_cost < _sch_debug_task_tick)
        event->max_cost = _sch_debug_task_tick;
      event->total_cost += _sch_debug_task_tick;
      if (event->max_lat < _late) event->max_lat = _late;
      event->total_lat += _late;
      event->run_cnt++;
#endif  // !_SCH_DEBUG_REPORT
      if (event_modified) return;
    }
  }
}

bool Sch_CreateEvent(const char *name, sch_func_t callback, uint8_t enable) {
#if !_SCH_EVENT_ALLOW_DUPLICATE
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) return false;
  }
#endif  // !_SCH_EVENT_ALLOW_DUPLICATE
  scheduler_event_t *p = (scheduler_event_t *)ulist_append(&eventlist, 1);
  if (p == NULL) return false;
  p->task = callback;
  p->enable = enable;
  p->trigger = 0;
  p->args = NULL;
  p->name = name;
  event_modified = 1;
  return true;
}

bool Sch_DeleteEvent(const char *name) {
  if (eventlist.num == 0) return false;
  bool ret = false;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      ulist_delete(&eventlist, event - (scheduler_event_t *)eventlist.data, 1);
      event_end--;
      event--;
      ret = true;
      event_modified = 1;
#if !_SCH_EVENT_ALLOW_DUPLICATE
      break;
#endif  // !_SCH_EVENT_ALLOW_DUPLICATE
      continue;
    }
  }
  return ret;
}

bool Sch_SetEventState(const char *name, uint8_t enable) {
  bool ret = false;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      if (enable == TOGGLE) {
        event->enable = !event->enable;
      } else {
        event->enable = enable;
      }
      event->trigger = 0;
      ret = true;
#if !_SCH_EVENT_ALLOW_DUPLICATE
      break;
#endif  // !_SCH_EVENT_ALLOW_DUPLICATE
    }
  }
  return ret;
}

bool Sch_TriggerEvent(const char *name, void *args) {
  bool ret = false;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (event->enable && fast_strcmp(event->name, name)) {
      event->trigger = 1;
      event->args = args;
#if _SCH_DEBUG_REPORT
      event->trigger_time = m_tick();
      event->trigger_cnt++;
#endif
#if !_SCH_EVENT_ALLOW_DUPLICATE
      break;
#endif  // !_SCH_EVENT_ALLOW_DUPLICATE
    }
  }
  return ret;
}

bool Sch_IsEventExist(const char *name) {
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_strcmp(event->name, name)) {
      return true;
    }
  }
  return false;
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
  m_time_t max_cost;    // 协程最大执行时间(Tick)
  m_time_t total_cost;  // 协程总执行时间(Tick)
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

__cortn_handle_t *__cortn_hp = {0};
static uint8_t cr_modified = 0;

_STATIC_INLINE m_time_t Cortn_Runner(void) {
  cr_modified = 0;
  m_time_t sleep_us = SCH_LARGE_TIME;
  m_time_t now = m_time_us();
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (!cortn->enable) continue;  // 跳过禁用协程
    if (cortn->hd.yieldUntil && now >= cortn->hd.yieldUntil) {
      __cortn_hp = &cortn->hd;
      __cortn_hp->depth = 0;
#if _SCH_DEBUG_REPORT
      m_time_t _sch_debug_task_tick = m_tick();
      cortn->task(cortn->args);
      _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
      if (cortn->max_cost < _sch_debug_task_tick)
        cortn->max_cost = _sch_debug_task_tick;
      cortn->total_cost += _sch_debug_task_tick;
#else
      cortn->task(cortn->args);
#endif
      __cortn_hp = NULL;
      if (!cortn->hd.ptr[0]) {     // 最外层协程已结束
        cortn->hd.yieldUntil = 1;  // 准备下一次执行
        if (cortn->mode == CR_MODE_AUTODEL) {
          Sch_DeleteCortn(cortn->name);
          return 0;  // 指针已被释放
        } else if (cortn->mode == CR_MODE_ONESHOT) {
          cortn->enable = 0;
          continue;
        }
      }
    }
    if (cortn->hd.yieldUntil) {
      if (cortn->hd.yieldUntil < sleep_us) {
        sleep_us = cortn->hd.yieldUntil;
      }
    }
    if (cr_modified) return 0;  // 有协程被修改，不确定
  }
  return sleep_us;
}

bool Sch_CreateCortn(const char *name, sch_func_t func, uint8_t enable,
                     CR_MODE mode, void *args) {
  scheduler_cortn_t *p = (scheduler_cortn_t *)ulist_append(&cortnlist, 1);
  if (p == NULL) return false;
  p->task = func;
  p->enable = enable;
  p->mode = mode;
  p->args = args;
  p->name = name;
  memset(&p->hd, 0, sizeof(__cortn_handle_t));
  p->hd.yieldUntil = m_time_us();
  cr_modified = 1;
  return true;
}

bool Sch_DeleteCortn(const char *name) {
  if (cortnlist.num == 0) return false;
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      ulist_delete(&cortnlist, cortn - (scheduler_cortn_t *)cortnlist.data, 1);
      cr_modified = 1;
      return true;
    }
  }
  return false;
}

bool Sch_SetCortnState(const char *name, uint8_t enable, uint8_t clearState) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      if (enable == TOGGLE) {
        cortn->enable = !cortn->enable;
      } else {
        cortn->enable = enable;
      }
      if (clearState) {
        memset(&cortn->hd, 0, sizeof(__cortn_handle_t));
      }
      cortn->hd.yieldUntil = m_time_us();
      return true;
    }
  }
  return false;
}

uint16_t Sch_GetCortnNum(void) { return cortnlist.num; }

bool Sch_IsCortnExist(const char *name) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      return true;
    }
  }
  return false;
}
bool Sch_IsCortnWaitForMsg(const char *name) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      return cortn->hd.yieldUntil == 0;
    }
  }
  return false;
}

bool Sch_SendMsgToCortn(const char *name, void *msg) {
  ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
    if (fast_strcmp(cortn->name, name)) {
      cortn->hd.msg = msg;
      cortn->hd.yieldUntil = m_time_us();
      return true;
    }
  }
  return false;
}

#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_CALLLATER
#pragma pack(1)
typedef struct {       // 延时调用任务结构
  sch_func_t task;     // 任务函数指针
  m_time_t runTimeUs;  // 执行时间(us)
  void *args;          // 任务参数
} scheduler_call_later_t;
#pragma pack()

static ulist_t clist = {.data = NULL,
                        .cap = 0,
                        .num = 0,
                        .isize = sizeof(scheduler_call_later_t),
                        .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

_STATIC_INLINE m_time_t CallLater_Runner(void) {
  m_time_t sleep_us = SCH_LARGE_TIME;
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (m_time_us() >= callLater_p->runTimeUs) {
      callLater_p->task(callLater_p->args);
      ulist_delete(&clist, callLater_p - (scheduler_call_later_t *)clist.data,
                   1);
      return 0;  // 有任务被执行，不确定
    }
    if (callLater_p->runTimeUs < sleep_us) {
      sleep_us = callLater_p->runTimeUs;
    }
  }
  return sleep_us;
}

bool Sch_CallLater(sch_func_t func, m_time_t delayUs, void *args) {
  scheduler_call_later_t *p = (scheduler_call_later_t *)ulist_append(&clist, 1);
  if (p == NULL) return false;
  p->task = func;
  p->runTimeUs = delayUs + m_time_us();
  p->args = args;
  return true;
}

void Sch_CancelCallLater(sch_func_t func) {
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (callLater_p->task == func) {
      ulist_delete(&clist, callLater_p - (scheduler_call_later_t *)clist.data,
                   1);
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

_STATIC_INLINE bool DebugInfo_Runner(void) {
  static m_time_t last_print = 0;
  static uint8_t first_print = 1;
  float usage;
  char op;
  uint16_t i;
  m_time_t now = m_tick();
  m_time_t offset = now;
  if (first_print) {  // 因为初始化耗时等原因，第一次的数据无参考价值，不打印
    first_print = 0;
  } else {
    if (now - last_print <= _SCH_DEBUG_PERIOD * m_tick_clk(m_time_t))
      return false;
    double temp = m_tick_per_us(double);  // 统一转换到us
    m_time_t period = now - last_print;
    m_time_t other = period;
    LOG_RAWLN("");
#if _SCH_ENABLE_TASK
    if (tasklist.num) {
      LOG_RAW(T_FMT(T_RESET, T_BOLD, T_BLUE));
      LOG_RAWLN("[ Task Report ]------------------------------------------");
      LOG_RAWLN(" No | Pri | Run | Tmax  | Usage | LTavg | LTmax | Name");
      i = 0;
      ulist_foreach(&tasklist, scheduler_task_t, task) {
        if (task->enable) {
          usage = (double)task->total_cost / period * 100;
          if ((task->last_usage != 0 && usage / task->last_usage > 2) ||
              usage > 20) {  // 任务占用率大幅度增加或者超过20%
            LOG_RAW(T_FMT(T_RESET, T_BOLD, T_YELLOW));
            op = '!';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN("%c%-4d %-5d %-5d %-7.2f %-7.3f %-7.2f %-7.2f %s ", op, i,
                    task->priority, task->run_cnt,
                    (double)task->max_cost / temp, usage,
                    (double)task->total_lat / task->run_cnt / temp,
                    (double)task->max_lat / temp, task->name);
          task->last_usage = usage;
          other -= task->total_cost;
        } else {
          LOG_RAW(T_FMT(T_RESET, T_WHITE));  // 禁用
          op = '-';
          LOG_RAWLN("%c%-4d %-5d %-5s %-7s %-7s %-7s %-7s %s ", op, i,
                    task->priority, "-", "-", "-", "-", "-", task->name);
          task->last_usage = 0;
        }
        i++;
      }
    }
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
    if (eventlist.num) {
      LOG_RAW(T_FMT(T_RESET, T_BOLD, T_BLUE));
      LOG_RAWLN("[ Event Report ]-----------------------------------------");
      LOG_RAWLN(" No | Tri | Run | Tmax  | Usage | LTavg | LTmax | Event");
      i = 0;
      ulist_foreach(&eventlist, scheduler_event_t, event) {
        if (event->enable && event->run_cnt) {
          usage = (double)event->total_cost / period * 100;
          if ((event->last_usage != 0 && usage / event->last_usage > 2) ||
              usage > 20) {  // 任务占用率大幅度增加或者超过20%
            LOG_RAW(T_FMT(T_RESET, T_BOLD, T_YELLOW));
            op = '!';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN("%c%-4d %-5d %-5d %-7.2f %-7.3f %-7.2f %-7.2f %s ", op, i,
                    event->trigger_cnt, event->run_cnt,
                    (double)event->max_cost / temp, usage,
                    (double)event->total_lat / event->run_cnt / temp,
                    (double)event->max_lat / temp, event->name);
          event->last_usage = usage;
          other -= event->total_cost;
        } else {
          if (!event->enable) {
            LOG_RAW(T_FMT(T_RESET, T_WHITE));  // 禁用
            op = '-';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN("%c%-4d %-5d %-5d %-7s %-7s %-7s %-7s %s ", op, i,
                    event->trigger_cnt, event->run_cnt, "-", "-", "-", "-",
                    event->name);
          event->last_usage = 0;
        }
        i++;
      }
    }
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
    if (cortnlist.num) {
      LOG_RAW(T_FMT(T_RESET, T_BOLD, T_BLUE));
      LOG_RAWLN("[ Coroutine Report ]-------------------------------------");
      LOG_RAWLN(" No | Tmax  | Usage | Name");
      i = 0;
      ulist_foreach(&cortnlist, scheduler_cortn_t, cortn) {
        if (cortn->enable) {
          usage = (double)cortn->total_cost / period * 100;
          if ((cortn->last_usage != 0 && usage / cortn->last_usage > 2) ||
              usage > 20) {  // 任务占用率大幅度增加或者超过20%
            LOG_RAW(T_FMT(T_RESET, T_BOLD, T_YELLOW));
            op = '!';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN("%c%-4d %-7.2f %-7.3f %s ", op, i,
                    (double)cortn->max_cost / temp, usage, cortn->name);
          cortn->last_usage = usage;
          other -= cortn->total_cost;
        } else {
          LOG_RAW(T_FMT(T_RESET, T_WHITE));  // 禁用
          LOG_RAWLN("-%-4d -       -       %s ", i, cortn->name);
          cortn->last_usage = 0;
        }
        i++;
      }
    }
#endif  // _SCH_ENABLE_COROUTINE
    LOG_RAW(T_FMT(T_BOLD, T_BLUE));
    LOG_RAWLN("[ System Stats ]-----------------------------------------");
    LOG_RAW(T_FMT(T_RESET, T_GREEN));
    LOG_RAW(" Core: %.0fMhz / Run: %.2fs / Idle: %.2f%%", temp,
            (double)m_time_ms() / 1000, (double)other / period * 100);
#if _MOD_HEAP_MATHOD == 1 || (_MOD_HEAP_MATHOD == 2 && HEAP_USE_LWMEM)
    lwmem_stats_t stats;
    lwmem_get_stats(&stats);
    LOG_RAW(" / Mem: %.2f%%",
            (float)(stats.mem_size_bytes - stats.mem_available_bytes) /
                (float)stats.mem_size_bytes * 100);
#endif
    LOG_RAWLN(T_FMT(T_BOLD, T_BLUE));
    LOG_RAWLN(
        "---------------------------------------------------------" T_RST);
  }
  offset = m_tick() - offset;  // 补偿打印LOG的时间
#if _SCH_ENABLE_TASK
  ulist_foreach(&tasklist, scheduler_task_t, task) {
    task->lastRun += offset;
    task->max_cost = 0;
    task->total_cost = 0;
    task->run_cnt = 0;
    task->max_lat = 0;
    task->total_lat = 0;
  }
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    event->trigger_time += offset;
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
  last_print = now;
  return true;
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
                task->priority, m_tick_clk(double) / task->period,
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

static void sysinfo_print(void *arg) {
  if (arg != NULL) LOG_RAWLN();
  LOG_RAWLN(
      T_FMT(T_BOLD, T_GREEN) "[ System Info ]-------------------------" T_FMT(
          T_RESET, T_GREEN));
  LOG_RAWLN("  Core Clock: %.3f Mhz", m_tick_per_us(float));
  LOG_RAWLN("  After Boot: %.2fs", (double)m_time_ms() / 1000);
#if _MOD_USE_OS == 1  // klite
  static uint64_t last_kernel_tick = 0;
  static uint64_t last_idle_time = 0;
  uint64_t kernel_tick = kernel_tick_count64();
  uint64_t idle_time = kernel_idle_time();
  double usage = (double)((kernel_tick - last_kernel_tick) -
                          (idle_time - last_idle_time)) /
                 (double)(kernel_tick - last_kernel_tick);
  double usage_avg = (double)(kernel_tick - idle_time) / (double)kernel_tick;
  last_kernel_tick = kernel_tick;
  last_idle_time = idle_time;
  LOG_RAWLN("  RTOS Usage: %.2f%% (avg:%.2f%%)", usage * 100, usage_avg * 100);
#endif
#if _SCH_ENABLE_TASK
  LOG_RAWLN("  Task      : %d", tasklist.num);
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
  LOG_RAWLN("  Event     : %d", eventlist.num);
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
  LOG_RAWLN("  Coroutine : %d", cortnlist.num);
#endif  // _SCH_ENABLE_COROUTINE
#if _MOD_HEAP_MATHOD == 1 || (_MOD_HEAP_MATHOD == 2 && HEAP_USE_LWMEM)
  LOG_RAWLN(
      T_FMT(T_BOLD, T_GREEN) "[ LwMem Info ]--------------------------" T_FMT(
          T_RESET, T_GREEN));
  lwmem_stats_t stats;
  lwmem_get_stats(&stats);
  LOG_RAWLN("  Total     : %d Bytes", stats.mem_size_bytes);
  LOG_RAWLN(
      "  Avail     : %d Bytes (%.4f%%)", stats.mem_available_bytes,
      (double)(stats.mem_available_bytes) / (double)stats.mem_size_bytes * 100);
  LOG_RAWLN("  Min Avail : %d Bytes (%.4f%%)",
            stats.minimum_ever_mem_available_bytes,
            (double)(stats.minimum_ever_mem_available_bytes) /
                (double)stats.mem_size_bytes * 100);
  LOG_RAWLN("  Allocated : %d blocks", stats.nr_alloc);
  LOG_RAWLN("  Freed     : %d blocks", stats.nr_free);
#endif
  LOG_RAWLN(
      T_FMT(T_BOLD, T_GREEN) "----------------------------------------" T_RST);
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
      .autoTokenizeArgs = true,
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
      .autoTokenizeArgs = true,
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
      .autoTokenizeArgs = true,
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
      .autoTokenizeArgs = true,
      .func = softint_cmd_func,
  };

  embeddedCliAddBinding(cli, softint_cmd);
#endif  // _SCH_ENABLE_SOFTINT

  static CliCommandBinding sysinfo_cmd = {
      .name = "sysinfo",
      .usage = "sysinfo [start|stop] [freq]",
      .help = "Show system info (Scheduler)",
      .context = NULL,
      .autoTokenizeArgs = true,
      .func = sysinfo_cmd_func,
  };

  embeddedCliAddBinding(cli, sysinfo_cmd);
}
#endif  // _SCH_ENABLE_TERMINAL

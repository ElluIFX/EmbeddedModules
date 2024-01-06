#include "scheduler.h"

#include <string.h>

#include "log.h"
#include "ulist.h"

#define _INLINE inline __attribute__((always_inline))
#define _STATIC_INLINE static _INLINE

_STATIC_INLINE bool fast_str_check(const char *str1, const char *str2) {
  while (*str1 != '\0' && *str2 != '\0') {
    if (*str1 != *str2) return false;
    str1++;
    str2++;
  }
  return (!*str1) && (!*str2);
}

_STATIC_INLINE void DebugInfo_Runner(void);
_STATIC_INLINE void Task_Runner(void);
_STATIC_INLINE void Event_Runner(void);
_STATIC_INLINE void Coron_Runner(void);
_STATIC_INLINE void CallLater_Runner(void);
_STATIC_INLINE void SoftInt_Runner(void);

void _INLINE Scheduler_Run(const uint8_t block, const m_time_t sleep_us) {
  do {
#if _SCH_ENABLE_TASK
    Task_Runner();
#endif
#if _SCH_ENABLE_SOFTINT
    SoftInt_Runner();
#endif
#if _SCH_ENABLE_EVENT
    Event_Runner();
#endif
#if _SCH_ENABLE_COROUTINE
    Coron_Runner();
#endif
#if _SCH_ENABLE_CALLLATER
    CallLater_Runner();
#endif
#if _SCH_DEBUG_REPORT
    DebugInfo_Runner();
#endif
    if (block && sleep_us) m_delay_us(sleep_us);
  } while (block);
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
    if (fast_str_check(task->name, name)) return task;
  }
  return NULL;
}

static void resort_task(void) {
  if (tasklist.num <= 1) return;
  ulist_sort(&tasklist, taskcmp, SLICE_START, SLICE_END);
}

_STATIC_INLINE void Task_Runner(void) {
  m_time_t latency;
  m_time_t now;
#if _SCH_DEBUG_REPORT
  __IO const char *old_name;
#endif  // _SCH_DEBUG_REPORT
  if (tasklist.num) {
    now = m_tick();
    ulist_foreach(&tasklist, scheduler_task_t, task) {
      if (task->enable && (now >= task->lastRun + task->period)) {
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
          if (task == NULL) return;  // 任务已被删除
        }
        if (task->max_cost < _sch_debug_task_tick)
          task->max_cost = _sch_debug_task_tick;
        if (latency > task->max_lat) task->max_lat = latency;
        task->total_cost += _sch_debug_task_tick;
        task->total_lat += latency;
        task->run_cnt++;
#else
        task->task(task->args);
#endif  // _SCH_DEBUG_REPORT
        return;
      }
    }
  }
}

bool Sch_CreateTask(const char *name, sch_func_t func, float freqHz,
                    uint8_t enable, uint8_t priority, void *args) {
  scheduler_task_t *p = (scheduler_task_t *)ulist_append(&tasklist, 1);
  if (p == NULL) return false;
  p->task = func;
  p->enable = enable;
  p->priority = priority;
  p->period = (double)m_tick_clk / (double)freqHz;
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
    if (fast_str_check(task->name, name)) {
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
    p->lastRun = delayUs * m_tick_per_us + m_tick();
  else
    p->lastRun += delayUs * m_tick_per_us;
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
  p->period = (double)m_tick_clk / (double)freqHz;
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
static __IO uint8_t event_modified = 0;

_STATIC_INLINE void Event_Runner(void) {
  event_modified = 0;
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (event->enable && event->trigger) {
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
      event->trigger = 0;
    }
    if (event_modified) return;
  }
}

bool Sch_CreateEvent(const char *name, sch_func_t callback, uint8_t enable) {
#if !_SCH_EVENT_ALLOW_DUPLICATE
  ulist_foreach(&eventlist, scheduler_event_t, event) {
    if (fast_str_check(event->name, name)) {
      event->task = callback;
      event->enable = enable;
      event->trigger = 0;
      event->args = NULL;
      return true;
    }
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
    if (fast_str_check(event->name, name)) {
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
    if (fast_str_check(event->name, name)) {
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
    if (event->enable && fast_str_check(event->name, name)) {
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
    if (fast_str_check(event->name, name)) {
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
  __coron_handle_t hd;  // 协程句柄
#if _SCH_DEBUG_REPORT
  m_time_t max_cost;    // 协程最大执行时间(Tick)
  m_time_t total_cost;  // 协程总执行时间(Tick)
  float last_usage;     // 协程上次执行占用率
#endif
  const char *name;  // 协程名
} scheduler_coron_t;
#pragma pack()

static ulist_t coronlist = {.data = NULL,
                            .cap = 0,
                            .num = 0,
                            .isize = sizeof(scheduler_coron_t),
                            .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

__coron_handle_t *__coron_hp = {0};
static __IO uint8_t cr_modified = 0;

_STATIC_INLINE void Coron_Runner(void) {
  cr_modified = 0;
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    if (coron->enable && coron->hd.yieldUntil &&
        m_time_us() >= coron->hd.yieldUntil) {
      __coron_hp = &coron->hd;
      __coron_hp->depth = 0;
#if _SCH_DEBUG_REPORT
      m_time_t _sch_debug_task_tick = m_tick();
      coron->task(coron->args);
      _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
      if (coron->max_cost < _sch_debug_task_tick)
        coron->max_cost = _sch_debug_task_tick;
      coron->total_cost += _sch_debug_task_tick;
#else
      coron->task(coron->args);
#endif
      __coron_hp = NULL;
      if (!coron->hd.ptr[0]) {  // 最外层协程已结束
        if (coron->mode == CR_MODE_AUTODEL) {
          Sch_DeleteCoron(coron->name);
          return;  // 指针已被释放
        } else if (coron->mode == CR_MODE_ONESHOT) {
          coron->enable = 0;
        }
      }
    }
    if (cr_modified) return;
  }
}

bool Sch_CreateCoron(const char *name, sch_func_t func, uint8_t enable,
                     CR_MODE mode, void *args) {
  scheduler_coron_t *p = (scheduler_coron_t *)ulist_append(&coronlist, 1);
  if (p == NULL) return false;
  p->task = func;
  p->enable = enable;
  p->mode = mode;
  p->args = args;
  p->name = name;
  memset(&p->hd, 0, sizeof(__coron_handle_t));
  p->hd.yieldUntil = m_time_us();
  cr_modified = 1;
  return true;
}

bool Sch_DeleteCoron(const char *name) {
  if (coronlist.num == 0) return false;
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    if (fast_str_check(coron->name, name)) {
      ulist_delete(&coronlist, coron - (scheduler_coron_t *)coronlist.data, 1);
      cr_modified = 1;
      return true;
    }
  }
  return false;
}

bool Sch_SetCoronState(const char *name, uint8_t enable, uint8_t clearState) {
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    if (fast_str_check(coron->name, name)) {
      if (enable == TOGGLE) {
        coron->enable = !coron->enable;
      } else {
        coron->enable = enable;
      }
      if (clearState) {
        memset(&coron->hd, 0, sizeof(__coron_handle_t));
      }
      coron->hd.yieldUntil = m_time_us();
      return true;
    }
  }
  return false;
}

uint16_t Sch_GetCoronNum(void) { return coronlist.num; }

bool Sch_IsCoronExist(const char *name) {
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    if (fast_str_check(coron->name, name)) {
      return true;
    }
  }
  return false;
}

bool Sch_IsCoronWaitForWakeUp(const char *name) {
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    if (fast_str_check(coron->name, name)) {
      return coron->hd.yieldUntil == 0;
    }
  }
  return false;
}

bool Sch_WakeUpCoron(const char *name) {
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    if (fast_str_check(coron->name, name)) {
      coron->hd.yieldUntil = m_time_us();
      return true;
    }
  }
  return false;
}

bool Sch_SendMsgToCoron(const char *name, void *msg) {
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    if (fast_str_check(coron->name, name)) {
      coron->hd.msg = msg;
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

_STATIC_INLINE void CallLater_Runner(void) {
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (m_time_us() >= callLater_p->runTimeUs) {
      callLater_p->task(callLater_p->args);
      ulist_delete(&clist, callLater_p - (scheduler_call_later_t *)clist.data,
                   1);
      return;
    }
  }
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

static __IO uint8_t _imm = 0;
static __IO uint8_t _ism[8] = {0};

_INLINE void Sch_TriggerSoftInt(uint8_t mainChannel, uint8_t subChannel) {
  if (mainChannel > 7 || subChannel > 7) return;
  _imm |= 1 << mainChannel;
  _ism[mainChannel] |= 1 << subChannel;
}

__weak void Scheduler_SoftInt_Handler_Ch0(uint8_t subMask) {}
__weak void Scheduler_SoftInt_Handler_Ch1(uint8_t subMask) {}
__weak void Scheduler_SoftInt_Handler_Ch2(uint8_t subMask) {}
__weak void Scheduler_SoftInt_Handler_Ch3(uint8_t subMask) {}
__weak void Scheduler_SoftInt_Handler_Ch4(uint8_t subMask) {}
__weak void Scheduler_SoftInt_Handler_Ch5(uint8_t subMask) {}
__weak void Scheduler_SoftInt_Handler_Ch6(uint8_t subMask) {}
__weak void Scheduler_SoftInt_Handler_Ch7(uint8_t subMask) {}

_STATIC_INLINE void SoftInt_Runner(void) {
  __IO uint8_t ism;
  if (_imm) {
    __IO uint8_t imm = _imm;
    _imm = 0;
    if (imm & 0x01)
      ism = _ism[0], _ism[0] = 0, Scheduler_SoftInt_Handler_Ch0(ism);
    if (imm & 0x02)
      ism = _ism[1], _ism[1] = 0, Scheduler_SoftInt_Handler_Ch1(ism);
    if (imm & 0x04)
      ism = _ism[2], _ism[2] = 0, Scheduler_SoftInt_Handler_Ch2(ism);
    if (imm & 0x08)
      ism = _ism[3], _ism[3] = 0, Scheduler_SoftInt_Handler_Ch3(ism);
    if (imm & 0x10)
      ism = _ism[4], _ism[4] = 0, Scheduler_SoftInt_Handler_Ch4(ism);
    if (imm & 0x20)
      ism = _ism[5], _ism[5] = 0, Scheduler_SoftInt_Handler_Ch5(ism);
    if (imm & 0x40)
      ism = _ism[6], _ism[6] = 0, Scheduler_SoftInt_Handler_Ch6(ism);
    if (imm & 0x80)
      ism = _ism[7], _ism[7] = 0, Scheduler_SoftInt_Handler_Ch7(ism);
  }
}

#endif  // _SCH_ENABLE_SOFTINT

#if _SCH_DEBUG_REPORT
#warning Scheduler Debug-Report is on, expect performance degradation and increased memory usage of task handles

_STATIC_INLINE void DebugInfo_Runner(void) {
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
    if (now - last_print <= _SCH_DEBUG_PERIOD * m_tick_clk) return;
    double temp = m_tick_per_us;  // 统一转换到us
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
    if (coronlist.num) {
      LOG_RAW(T_FMT(T_RESET, T_BOLD, T_BLUE));
      LOG_RAWLN("[ Coroutine Report ]-------------------------------------");
      LOG_RAWLN(" No | Tmax  | Usage | Name");
      i = 0;
      ulist_foreach(&coronlist, scheduler_coron_t, coron) {
        if (coron->enable) {
          usage = (double)coron->total_cost / period * 100;
          if ((coron->last_usage != 0 && usage / coron->last_usage > 2) ||
              usage > 20) {  // 任务占用率大幅度增加或者超过20%
            LOG_RAW(T_FMT(T_RESET, T_BOLD, T_YELLOW));
            op = '!';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN("%c%-4d %-7.2f %-7.3f %s ", op, i,
                    (double)coron->max_cost / temp, usage, coron->name);
          coron->last_usage = usage;
          other -= coron->total_cost;
        } else {
          LOG_RAW(T_FMT(T_RESET, T_WHITE));  // 禁用
          LOG_RAWLN("-%-4d -       -       %s ", i, coron->name);
          coron->last_usage = 0;
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
#ifdef m_usage
    LOG_RAW(" / Mem: %.2f%%", m_usage() * 100);
#endif
    LOG_RAWLN(T_FMT(T_BOLD, T_BLUE));
    LOG_RAWLN(
        "---------------------------------------------------------" T_RST);
  }
  offset = m_tick() - offset;
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
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    coron->max_cost = 0;
    coron->total_cost = 0;
  }
#endif
  last_print = now;
}
#endif  // _SCH_DEBUG_REPORT

#if _SCH_ENABLE_TERMINAL
#include "stdlib.h"
#include "string.h"
#if _SCH_ENABLE_TASK
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
                task->priority, (double)m_tick_clk / task->period,
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
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Task-%s not found" T_RST, name);
    return;
  }
  if (embeddedCliCheckToken(args, "enable", 1)) {
    Sch_SetTaskState(name, ENABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task-%s enabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "disable", 1)) {
    Sch_SetTaskState(name, DISABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task-%s disabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "delete", 1)) {
    Sch_DeleteTask(name);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task-%s deleted" T_RST, name);
  } else if (embeddedCliCheckToken(args, "setfreq", 1)) {
    if (argc < 3) {
      LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Frequency is required" T_RST);
      return;
    }
    float freq = atof(embeddedCliGetToken(args, 3));
    Sch_SetTaskFreq(name, freq);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task-%s frequency set to %.2fHz" T_RST,
              name, freq);
  } else if (embeddedCliCheckToken(args, "setpri", 1)) {
    if (argc < 3) {
      LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Priority is required" T_RST);
      return;
    }
    uint8_t pri = atoi(embeddedCliGetToken(args, 3));
    Sch_SetTaskPriority(name, pri);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task-%s priority set to %d" T_RST, name,
              pri);
  } else if (embeddedCliCheckToken(args, "excute", 1)) {
    LOG_RAWLN(T_FMT(T_BOLD, T_YELLOW) "Force excuting Task-%s" T_RST, name);
    p->task(p->args);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
#endif  // _SCH_ENABLE_TASK

#if _SCH_ENABLE_EVENT
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
    if (fast_str_check(event->name, name)) {
      p = event;
      break;
    }
  }
  if (p == NULL) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Event-%s not found" T_RST, name);
    return;
  }
  if (embeddedCliCheckToken(args, "enable", 1)) {
    Sch_SetEventState(name, ENABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Event-%s enabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "disable", 1)) {
    Sch_SetEventState(name, DISABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Event-%s disabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "delete", 1)) {
    Sch_DeleteEvent(name);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Event-%s deleted" T_RST, name);
  } else if (embeddedCliCheckToken(args, "trigger", 1)) {
    Sch_TriggerEvent(name, NULL);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Event-%s triggered" T_RST, name);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
#endif  // _SCH_ENABLE_EVENT

#if _SCH_ENABLE_COROUTINE
void coron_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (embeddedCliCheckToken(args, "list", 1)) {
    LOG_RAWLN(
        T_FMT(T_BOLD, T_GREEN) "Coroutines list:" T_FMT(T_RESET, T_GREEN));
    ulist_foreach(&coronlist, scheduler_coron_t, coron) {
      LOG_RAWLN("  %s: 0x%p en:%d", coron->name, coron->task, coron->enable);
    }
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Total %d coroutines" T_RST,
              coronlist.num);
    return;
  }
  if (argc < 2) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Coroutine name is required" T_RST);
    return;
  }
  const char *name = embeddedCliGetToken(args, 2);
  scheduler_coron_t *p = NULL;
  ulist_foreach(&coronlist, scheduler_coron_t, coron) {
    if (fast_str_check(coron->name, name)) {
      p = coron;
      break;
    }
  }
  if (p == NULL) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Coroutine-%s not found" T_RST, name);
    return;
  }
  if (embeddedCliCheckToken(args, "enable", 1)) {
    Sch_SetCoronState(name, ENABLE, 1);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Coroutine-%s enabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "disable", 1)) {
    Sch_SetCoronState(name, DISABLE, 1);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Coroutine-%s disabled" T_RST, name);
  } else if (embeddedCliCheckToken(args, "delete", 1)) {
    Sch_DeleteCoron(name);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Coroutine-%s deleted" T_RST, name);
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_SOFTINT
void softint_cmd_func(EmbeddedCli *cli, char *args, void *context) {
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
  if (ch > 7) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Channel must be 0~7" T_RST);
    return;
  }
  if (sub > 7) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Sub-channel must be 0~7" T_RST);
    return;
  }
  Sch_TriggerSoftInt(ch, sub);
  LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "SoftInt-%d-%d triggered" T_RST, ch, sub);
}
#endif  // _SCH_ENABLE_SOFTINT

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
  static CliCommandBinding coron_cmd = {
      .name = "coron",
      .usage = "coron [list|enable|disable|delete] [name]",
      .help = "Coroutine control command (Scheduler)",
      .context = NULL,
      .autoTokenizeArgs = true,
      .func = coron_cmd_func,
  };

  embeddedCliAddBinding(cli, coron_cmd);
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
}
#endif  // _SCH_ENABLE_TERMINAL

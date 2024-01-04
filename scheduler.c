#include "scheduler.h"

#include <string.h>

#include "log.h"

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

static struct {
  scheduler_task_t *tasks;                 // 任务存储区
  uint16_t num;                            // 存储区总任务数量
  uint16_t size;                           // 存储区总大小
  uint16_t priOffset[_TASK_PRIORITY_NUM];  // 各优先级偏移量
} thd = {0};

static scheduler_task_t *insert_task(uint8_t priority) {
  scheduler_task_t *p;
  if (thd.tasks == NULL) {  // 初始化存储区
    // 每次创建两倍所需空间, 以减少内存操作
    m_alloc(thd.tasks, sizeof(scheduler_task_t) * 2);
    if (thd.tasks == NULL) return NULL;
    thd.size = 2;
  }
  if (thd.num + 1 > thd.size) {  // 扩容
    scheduler_task_t *old = thd.tasks;
    if (!m_realloc(thd.tasks, sizeof(scheduler_task_t) * thd.size * 2) ||
        !thd.tasks) {
      thd.tasks = old;
      return NULL;
    }
    thd.size *= 2;
  }
  if (priority == _TASK_PRIORITY_NUM - 1) {  // 直接添加到末尾
    p = &thd.tasks[thd.num];
  } else {  // 插入priOffset[priority+1]前
    p = &thd.tasks[thd.priOffset[priority + 1]];
    if (thd.priOffset[priority + 1] < thd.num)
      memmove(
          p + 1, p,
          sizeof(scheduler_task_t) * (thd.num - thd.priOffset[priority + 1]));
    for (uint16_t i = priority + 1; i < _TASK_PRIORITY_NUM; i++) {
      thd.priOffset[i]++;
    }
  }
  thd.num += 1;
  memset(p, 0, sizeof(scheduler_task_t));
  return p;
}

static scheduler_task_t *get_task(const char *name) {
  for (uint16_t i = 0; i < thd.num; i++) {
    if (fast_str_check(thd.tasks[i].name, name)) return &thd.tasks[i];
  }
  return NULL;
}

static void delete_task(uint16_t i, bool skip_mem_free) {
  if (i < thd.num - 1) {  // 不是最后一个, 需要移动
    memmove(&thd.tasks[i], &thd.tasks[i + 1],
            sizeof(scheduler_task_t) * (thd.num - i - 1));
    memset(&thd.tasks[thd.num - 1], 0, sizeof(scheduler_task_t));
  }
  for (uint8_t j = 0; j <= TASK_PRIORITY_LOWEST; j++) {
    if (thd.priOffset[j] > i) thd.priOffset[j]--;
  }
  thd.num -= 1;
  if (skip_mem_free) return;
  if (thd.num == 0) {  // 任务已清空
    m_free(thd.tasks);
    thd.tasks = NULL;
    thd.size = 0;
  } else if (thd.num < thd.size / 2) {  // 缩容
    scheduler_task_t *old = thd.tasks;
    if (!m_realloc(thd.tasks, sizeof(scheduler_task_t) * thd.size / 2)) {
      thd.tasks = old;
    } else {
      thd.size /= 2;
    }
  }
}

#if _SCH_DEBUG_REPORT
static inline void Debug_RunTask(scheduler_task_t *task_p, m_time_t latency) {
  const char *name = task_p->name;
  m_time_t _sch_debug_task_tick = m_tick();
  task_p->task(task_p->args);
  _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
  if (task_p->name != name) {  // 任务已被修改, 重新查找
    task_p = get_task(name);
    if (task_p == NULL) return;  // 任务已被删除
  }
  if (task_p->max_cost < _sch_debug_task_tick)
    task_p->max_cost = _sch_debug_task_tick;
  if (latency > task_p->max_lat) task_p->max_lat = latency;
  task_p->total_cost += _sch_debug_task_tick;
  task_p->total_lat += latency;
  task_p->run_cnt++;
}
#endif  // _SCH_DEBUG_REPORT

_STATIC_INLINE void Task_Runner(void) {
  m_time_t latency;
  m_time_t now = m_tick();
  if (thd.num) {
    for (uint16_t i = 0; i < thd.num; i++) {
      if (thd.tasks[i].enable &&
          (now >= thd.tasks[i].lastRun + thd.tasks[i].period)) {
        latency = now - (thd.tasks[i].lastRun + thd.tasks[i].period);
        if (latency <= _SCH_COMP_RANGE)
          thd.tasks[i].lastRun += thd.tasks[i].period;
        else
          thd.tasks[i].lastRun = now;
#if _SCH_DEBUG_REPORT
        Debug_RunTask(&thd.tasks[i], latency);
#else
        thd.tasks[i].task(thd.tasks[i].args);
#endif  // _SCH_DEBUG_REPORT
        return;
      }
    }
  }
}

bool Sch_CreateTask(const char *name, sch_func_t func, float freqHz,
                    uint8_t enable, TASK_PRIORITY priority, void *args) {
  if (thd.num == 0xFFFE) return false;
  if (priority >= _TASK_PRIORITY_NUM) priority = _TASK_PRIORITY_NUM - 1;
  scheduler_task_t *p = insert_task(priority);
  if (p == NULL) return false;
  p->task = func;
  p->enable = enable;
  p->priority = priority;
  p->period = (double)m_tick_clk / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = m_tick() - p->period;
  p->name = name;
  p->args = args;
  return true;
}

bool Sch_DeleteTask(const char *name) {
  if (thd.num == 0) return false;
  for (uint16_t i = 0; i < thd.num; i++) {
    if (fast_str_check(thd.tasks[i].name, name)) {
      delete_task(i, false);
      return true;
    }
  }
  return false;
}

bool Sch_IsTaskExist(const char *name) { return get_task(name) != NULL; }

bool Sch_SetTaskPriority(const char *name, TASK_PRIORITY priority) {
  if (priority >= _TASK_PRIORITY_NUM) priority = _TASK_PRIORITY_NUM - 1;
  scheduler_task_t *p = get_task(name);
  if (p == NULL) return false;
  scheduler_task_t temp = {0};
  memcpy(&temp, p, sizeof(scheduler_task_t));
  delete_task(p - thd.tasks, true);
  p = insert_task(priority);
  memcpy(p, &temp, sizeof(scheduler_task_t));
  p->priority = priority;
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

uint16_t Sch_GetTaskNum(void) { return thd.num; }

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

struct {
  scheduler_event_t *tasks;  // 任务存储区
  uint16_t num;              // 存储区总任务数量
  uint16_t size;             // 存储区总大小
} ehd = {0};

_STATIC_INLINE void Event_Runner(void) {
  for (uint16_t i = 0; i < ehd.num; i++) {
    if (ehd.tasks[i].enable && ehd.tasks[i].trigger) {
#if !_SCH_DEBUG_REPORT
      ehd.tasks[i].task(ehd.tasks[i].args);
#else
      m_time_t _sch_debug_task_tick = m_tick();
      m_time_t _late = _sch_debug_task_tick - ehd.tasks[i].trigger_time;
      ehd.tasks[i].task(ehd.tasks[i].args);
      _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
      if (ehd.tasks[i].max_cost < _sch_debug_task_tick)
        ehd.tasks[i].max_cost = _sch_debug_task_tick;
      ehd.tasks[i].total_cost += _sch_debug_task_tick;
      if (ehd.tasks[i].max_lat < _late) ehd.tasks[i].max_lat = _late;
      ehd.tasks[i].total_lat += _late;
      ehd.tasks[i].run_cnt++;
#endif  // !_SCH_DEBUG_REPORT
      ehd.tasks[i].trigger = 0;
    }
  }
}

bool Sch_CreateEvent(const char *name, sch_func_t callback, uint8_t enable) {
  if (ehd.num == 0xFFFE) return false;
  if (ehd.tasks == NULL) {
    m_alloc(ehd.tasks, sizeof(scheduler_event_t) * 2);
    if (ehd.tasks == NULL) return false;
    ehd.size = 2;
  } else {
    for (uint16_t i = 0; i < ehd.num; i++) {
      if (fast_str_check(ehd.tasks[i].name, name)) {
        ehd.tasks[i].task = callback;
        ehd.tasks[i].enable = enable;
        ehd.tasks[i].trigger = 0;
        ehd.tasks[i].args = NULL;
        return true;
      }
    }
  }
  if (ehd.num + 1 > ehd.size) {
    scheduler_event_t *old = ehd.tasks;
    if (!m_realloc(ehd.tasks, sizeof(scheduler_event_t) * ehd.size * 2)) {
      ehd.tasks = old;
      return false;
    }
    ehd.size *= 2;
  }
  scheduler_event_t *p = &ehd.tasks[ehd.num];
  ehd.num += 1;
  p->task = callback;
  p->enable = enable;
  p->trigger = 0;
  p->args = NULL;
  p->name = name;
  return true;
}

bool Sch_DeleteEvent(const char *name) {
  if (ehd.num == 0) return false;
  for (uint16_t i = 0; i < ehd.num; i++) {
    if (fast_str_check(ehd.tasks[i].name, name)) {
      if (i < ehd.num - 1) {  // 不是最后一个, 需要移动
        memmove(&ehd.tasks[i], &ehd.tasks[i + 1],
                sizeof(scheduler_event_t) * (ehd.num - i - 1));
      }
      ehd.num -= 1;
      if (ehd.num == 0) {  // 任务已清空
        m_free(ehd.tasks);
        ehd.tasks = NULL;
        ehd.size = 0;
      } else if (ehd.num < ehd.size / 2) {  // 缩容
        scheduler_event_t *old = ehd.tasks;
        if (!m_realloc(ehd.tasks, sizeof(scheduler_event_t) * ehd.size / 2)) {
          ehd.tasks = old;
        } else {
          ehd.size /= 2;
        }
      }
      return true;
    }
  }
  return false;
}

bool Sch_SetEventState(const char *name, uint8_t enable) {
  for (uint16_t i = 0; i < ehd.num; i++) {
    if (fast_str_check(ehd.tasks[i].name, name)) {
      if (enable == TOGGLE) {
        ehd.tasks[i].enable = !ehd.tasks[i].enable;
      } else {
        ehd.tasks[i].enable = enable;
      }
      ehd.tasks[i].trigger = 0;
      return true;
    }
  }
  return false;
}

bool Sch_TriggerEvent(const char *name, void *args) {
  for (uint16_t i = 0; i < ehd.num; i++) {
    if (ehd.tasks[i].enable && fast_str_check(ehd.tasks[i].name, name)) {
      ehd.tasks[i].trigger = 1;
      ehd.tasks[i].args = args;
#if _SCH_DEBUG_REPORT
      ehd.tasks[i].trigger_time = m_tick();
      ehd.tasks[i].trigger_cnt++;
#endif
      return true;
    }
  }
  return false;
}

bool Sch_IsEventExist(const char *name) {
  for (uint16_t i = 0; i < ehd.num; i++) {
    if (fast_str_check(ehd.tasks[i].name, name)) {
      return true;
    }
  }
  return false;
}

uint16_t Sch_GetEventNum(void) { return ehd.num; }
#endif  // _SCH_ENABLE_EVENT

#if _SCH_ENABLE_COROUTINE
#pragma pack(1)
typedef struct {      // 协程任务结构
  sch_func_t task;    // 任务函数指针
  uint8_t enable;     // 是否使能
  uint8_t mode;       // 模式
  void *args;         // 任务参数
  _cron_handle_t hd;  // 协程句柄
#if _SCH_DEBUG_REPORT
  m_time_t max_cost;    // 协程最大执行时间(Tick)
  m_time_t total_cost;  // 协程总执行时间(Tick)
  float last_usage;     // 协程上次执行占用率
#endif
  const char *name;  // 协程名
} scheduler_cron_t;
#pragma pack()

struct {
  scheduler_cron_t *tasks;  // 任务存储区
  uint16_t num;             // 存储区总任务数量
  uint16_t size;            // 存储区总大小
} chd = {0};

_cron_handle_t *_cron_hp = {0};

_STATIC_INLINE void Coron_Runner(void) {
  for (uint16_t i = 0; i < chd.num; i++) {
    if (chd.tasks[i].enable && m_time_us() >= chd.tasks[i].hd.yieldUntil) {
      _cron_hp = &chd.tasks[i].hd;
      _cron_hp->depth = 0;
#if _SCH_DEBUG_REPORT
      m_time_t _sch_debug_task_tick = m_tick();
      chd.tasks[i].task(chd.tasks[i].args);
      _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
      if (chd.tasks[i].max_cost < _sch_debug_task_tick)
        chd.tasks[i].max_cost = _sch_debug_task_tick;
      chd.tasks[i].total_cost += _sch_debug_task_tick;
#else
      chd.tasks[i].task(chd.tasks[i].args);
#endif
      _cron_hp = NULL;
      if (!chd.tasks[i].hd.ptr[0]) {  // 最外层协程已结束
        if (chd.tasks[i].mode == CR_MODE_AUTODEL) {
          Sch_DeleteCoron(chd.tasks[i].name);
          return;  // 指针已被释放
        } else if (chd.tasks[i].mode == CR_MODE_ONESHOT) {
          chd.tasks[i].enable = 0;
        }
      }
    }
  }
}

bool Sch_CreateCoron(const char *name, sch_func_t func, uint8_t enable,
                     CR_MODE mode, void *args) {
  if (chd.num == 0xFFFE) return false;
  if (chd.tasks == NULL) {
    m_alloc(chd.tasks, sizeof(scheduler_cron_t) * 2);
    if (chd.tasks == NULL) return false;
    chd.size = 2;
  }
  if (chd.num + 1 > chd.size) {
    scheduler_cron_t *old = chd.tasks;
    if (!m_realloc(chd.tasks, sizeof(scheduler_cron_t) * chd.size * 2)) {
      chd.tasks = old;
      return false;
    }
    chd.size *= 2;
  }
  scheduler_cron_t *p = &chd.tasks[chd.num];
  chd.num += 1;
  p->task = func;
  p->enable = enable;
  p->mode = mode;
  p->args = args;
  p->name = name;
  memset(&p->hd, 0, sizeof(_cron_handle_t));
  return true;
}

bool Sch_DeleteCoron(const char *name) {
  if (chd.num == 0) return false;
  for (uint16_t i = 0; i < chd.num; i++) {
    if (fast_str_check(chd.tasks[i].name, name)) {
      if (i < chd.num - 1) {  // 不是最后一个, 需要移动
        memmove(&chd.tasks[i], &chd.tasks[i + 1],
                sizeof(scheduler_cron_t) * (chd.num - i - 1));
      }
      chd.num -= 1;
      if (chd.num == 0) {  // 任务已清空
        m_free(chd.tasks);
        chd.tasks = NULL;
        chd.size = 0;
      } else if (chd.num < chd.size / 2) {  // 缩容
        scheduler_cron_t *old = chd.tasks;
        if (!m_realloc(chd.tasks, sizeof(scheduler_cron_t) * chd.size / 2)) {
          chd.tasks = old;
        } else {
          chd.size /= 2;
        }
      }
      return true;
    }
  }
  return false;
}

bool Sch_SetCoronState(const char *name, uint8_t enable, uint8_t clearState) {
  for (uint16_t i = 0; i < chd.num; i++) {
    if (fast_str_check(chd.tasks[i].name, name)) {
      if (enable == TOGGLE) {
        chd.tasks[i].enable = !chd.tasks[i].enable;
      } else {
        chd.tasks[i].enable = enable;
      }
      if (clearState) {
        memset(&chd.tasks[i].hd, 0, sizeof(_cron_handle_t));
      }
      return true;
    }
  }
  return false;
}

uint16_t Sch_GetCoronNum(void) { return chd.num; }

bool Sch_IsCoronExist(const char *name) {
  for (uint16_t i = 0; i < chd.num; i++) {
    if (fast_str_check(chd.tasks[i].name, name)) {
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
  void *next;
} scheduler_call_later_t;
#pragma pack()
scheduler_call_later_t *schCallLaterEntry = NULL;

_STATIC_INLINE void CallLater_Runner(void) {
  if (schCallLaterEntry != NULL) {
    for (scheduler_call_later_t *callLater_p = schCallLaterEntry;
         callLater_p != NULL; callLater_p = callLater_p->next) {
      if (m_time_us() >= callLater_p->runTimeUs) {
        callLater_p->task(callLater_p->args);
        Sch_CancelCallLater(callLater_p->task);
        return;
      }
    }
  }
}

bool Sch_CallLater(sch_func_t func, m_time_t delayUs, void *args) {
  scheduler_call_later_t *p = NULL;
  if (schCallLaterEntry == NULL) {
    m_alloc(schCallLaterEntry, sizeof(scheduler_call_later_t));
    p = schCallLaterEntry;
  } else {
    scheduler_call_later_t *q = schCallLaterEntry;
    while (q->next != NULL) {
      q = q->next;
    }
    m_alloc(q->next, sizeof(scheduler_call_later_t));
    p = q->next;
  }
  if (p == NULL) return false;
  p->task = func;
  p->runTimeUs = delayUs + m_time_us();
  p->args = args;
  p->next = NULL;
  return true;
}

void Sch_CancelCallLater(sch_func_t func) {
  scheduler_call_later_t *p = schCallLaterEntry;
  scheduler_call_later_t *q = NULL;
  void **temp = NULL;
  while (p != NULL) {
    if (p->task == func) {
      if (q == NULL) {
        temp = (void **)&(p->next);
        m_free(schCallLaterEntry);
        if (*temp != NULL) m_replace(*temp, (schCallLaterEntry));
        p = schCallLaterEntry;
      } else {
        temp = (void **)&(p->next);
        m_free(q->next);
        if (*temp != NULL) m_replace(*temp, (q->next));
        p = q->next;
      }
      continue;
    }
    q = p;
    p = p->next;
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
    if (thd.num) {
      LOG_RAW(T_FMT(T_RESET, T_BOLD, T_BLUE));
      LOG_RAWLN("[ Task Report ]------------------------------------------");
      LOG_RAWLN(" No | Pri | Run | Tmax  | Usage | LTavg | LTmax | Name");
      for (uint16_t i = 0; i < thd.num; i++) {
        if (thd.tasks[i].enable) {
          usage = (double)thd.tasks[i].total_cost / period * 100;
          if ((thd.tasks[i].last_usage != 0 &&
               usage / thd.tasks[i].last_usage > 2) ||
              usage > 20) {  // 任务占用率大幅度增加或者超过20%
            LOG_RAW(T_FMT(T_RESET, T_BOLD, T_YELLOW));
            op = '!';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN(
              "%c%-4d %-5d %-5d %-7.2f %-7.3f %-7.2f %-7.2f %s ", op, i,
              thd.tasks[i].priority, thd.tasks[i].run_cnt,
              (double)thd.tasks[i].max_cost / temp, usage,
              (double)thd.tasks[i].total_lat / thd.tasks[i].run_cnt / temp,
              (double)thd.tasks[i].max_lat / temp, thd.tasks[i].name);
          thd.tasks[i].last_usage = usage;
          other -= thd.tasks[i].total_cost;
        } else {
          LOG_RAW(T_FMT(T_RESET, T_WHITE));  // 禁用
          op = 'x';
          LOG_RAWLN("%c%-4d %-4d %-5s %-7s %-7s %-7s %-7s %s ", op, i,
                    thd.tasks[i].priority, "-", "-", "-", "-", "-",
                    thd.tasks[i].name);
          thd.tasks[i].last_usage = 0;
        }
      }
    }
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
    if (ehd.num) {
      LOG_RAW(T_FMT(T_RESET, T_BOLD, T_BLUE));
      LOG_RAWLN("[ Event Report ]-----------------------------------------");
      LOG_RAWLN(" No | Tri | Run | Tmax  | Usage | LTavg | LTmax | Event");
      for (uint16_t i = 0; i < ehd.num; i++) {
        if (ehd.tasks[i].enable && ehd.tasks[i].run_cnt) {
          usage = (double)ehd.tasks[i].total_cost / period * 100;
          if ((ehd.tasks[i].last_usage != 0 &&
               usage / ehd.tasks[i].last_usage > 2) ||
              usage > 20) {  // 任务占用率大幅度增加或者超过20%
            LOG_RAW(T_FMT(T_RESET, T_BOLD, T_YELLOW));
            op = '!';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN(
              "%c%-4d %-5d %-5d %-7.2f %-7.3f %-7.2f %-7.2f %s ", op, i,
              ehd.tasks[i].trigger_cnt, ehd.tasks[i].run_cnt,
              (double)ehd.tasks[i].max_cost / temp, usage,
              (double)ehd.tasks[i].total_lat / ehd.tasks[i].run_cnt / temp,
              (double)ehd.tasks[i].max_lat / temp, ehd.tasks[i].name);
          ehd.tasks[i].last_usage = usage;
          other -= ehd.tasks[i].total_cost;
        } else {
          if (!ehd.tasks[i].enable) {
            LOG_RAW(T_FMT(T_RESET, T_WHITE));  // 禁用
            op = 'x';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN("%c%-4d %-5d %-5d -       -       -       -       %s ", op,
                    i, ehd.tasks[i].trigger_cnt, ehd.tasks[i].run_cnt,
                    ehd.tasks[i].name);
          ehd.tasks[i].last_usage = 0;
        }
      }
    }
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
    if (chd.num) {
      LOG_RAW(T_FMT(T_RESET, T_BOLD, T_BLUE));
      LOG_RAWLN("[ Coroutine Report ]-------------------------------------");
      LOG_RAWLN(" No | Tmax  | Usage | Name");
      for (uint16_t i = 0; i < chd.num; i++) {
        if (chd.tasks[i].enable) {
          usage = (double)chd.tasks[i].total_cost / period * 100;
          if ((chd.tasks[i].last_usage != 0 &&
               usage / chd.tasks[i].last_usage > 2) ||
              usage > 20) {  // 任务占用率大幅度增加或者超过20%
            LOG_RAW(T_FMT(T_RESET, T_BOLD, T_YELLOW));
            op = '!';
          } else {
            LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
            op = ' ';
          }
          LOG_RAWLN("%c%-4d %-7.2f %-7.3f %s ", op, i,
                    (double)chd.tasks[i].max_cost / temp, usage,
                    chd.tasks[i].name);
          chd.tasks[i].last_usage = usage;
          other -= chd.tasks[i].total_cost;
        } else {
          LOG_RAW(T_FMT(T_RESET, T_WHITE));  // 禁用
          LOG_RAWLN("x%-4d -       -       %s ", i, chd.tasks[i].name);
          ehd.tasks[i].last_usage = 0;
        }
      }
    }
#endif  // _SCH_ENABLE_COROUTINE
    LOG_RAW(T_FMT(T_BOLD, T_BLUE));
    LOG_RAWLN("[ System Stats ]-----------------------------------------");
    LOG_RAW(T_FMT(T_RESET, T_GREEN));
    LOG_RAW(" Clock: %.0fMhz / Run: %.2fs / Idle: %.2f%%", temp,
            (double)m_time_ms() / 1000, (double)other / period * 100);
#ifdef m_usage
    LOG_RAW(" / Mem: %.3f%%", m_usage() * 100);
#endif
    LOG_RAWLN(T_FMT(T_BOLD, T_BLUE));
    LOG_RAWLN(
        "---------------------------------------------------------" T_RST);
  }
  offset = m_tick() - offset;
#if _SCH_ENABLE_TASK
  for (uint16_t i = 0; i < thd.num; i++) {
    thd.tasks[i].lastRun += offset;
    thd.tasks[i].max_cost = 0;
    thd.tasks[i].total_cost = 0;
    thd.tasks[i].run_cnt = 0;
    thd.tasks[i].max_lat = 0;
    thd.tasks[i].total_lat = 0;
  }
#endif  // _SCH_ENABLE_TASK
#if _SCH_ENABLE_EVENT
  for (uint16_t i = 0; i < ehd.num; i++) {
    ehd.tasks[i].trigger_time += offset;
    ehd.tasks[i].max_cost = 0;
    ehd.tasks[i].total_cost = 0;
    ehd.tasks[i].run_cnt = 0;
    ehd.tasks[i].trigger_cnt = 0;
    ehd.tasks[i].max_lat = 0;
    ehd.tasks[i].total_lat = 0;
    ehd.tasks[i].trigger = 0;
  }
#endif  // _SCH_ENABLE_EVENT
#if _SCH_ENABLE_COROUTINE
  for (uint16_t i = 0; i < chd.num; i++) {
    chd.tasks[i].max_cost = 0;
    chd.tasks[i].total_cost = 0;
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
    LOG_RAWLN(T_FMT(T_BOLD, T_BLUE) "Tasks list:" T_FMT(T_RESET, T_GREEN));
    for (uint16_t i = 0; i < thd.num; i++) {
      LOG_RAWLN("Task-%s: 0x%p pri:%d freq:%.1f en:%d", thd.tasks[i].name,
                thd.tasks[i].task, thd.tasks[i].priority,
                (double)m_tick_clk / thd.tasks[i].period, thd.tasks[i].enable);
    }
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Total %d tasks" T_RST, thd.num);
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
    LOG_RAWLN(T_FMT(T_BOLD, T_BLUE) "Events list:" T_FMT(T_RESET, T_GREEN));
    for (uint16_t i = 0; i < ehd.num; i++) {
      LOG_RAWLN("Event-%s: 0x%p en:%d", ehd.tasks[i].name, ehd.tasks[i].task,
                ehd.tasks[i].enable);
    }
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Total %d events" T_RST, ehd.num);
    return;
  }
  if (argc < 2) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Event name is required" T_RST);
    return;
  }
  const char *name = embeddedCliGetToken(args, 2);
  scheduler_event_t *p = NULL;
  for (uint16_t i = 0; i < ehd.num; i++) {
    if (fast_str_check(ehd.tasks[i].name, name)) {
      p = &ehd.tasks[i];
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
    LOG_RAWLN(T_FMT(T_BOLD, T_BLUE) "Coroutines list:" T_FMT(T_RESET, T_GREEN));
    for (uint16_t i = 0; i < chd.num; i++) {
      LOG_RAWLN("Coron-%s: 0x%p en:%d", chd.tasks[i].name, chd.tasks[i].task,
                chd.tasks[i].enable);
    }
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Total %d coroutines" T_RST, chd.num);
    return;
  }
  if (argc < 2) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Coroutine name is required" T_RST);
    return;
  }
  const char *name = embeddedCliGetToken(args, 2);
  scheduler_cron_t *p = NULL;
  for (uint16_t i = 0; i < chd.num; i++) {
    if (fast_str_check(chd.tasks[i].name, name)) {
      p = &chd.tasks[i];
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

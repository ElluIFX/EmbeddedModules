#include <scheduler.h>

#define _STATIC_INLINE static inline __attribute__((always_inline))

#if _SCH_ENABLE_COROUTINE
_STATIC_INLINE void Cron_Runner(void);
#endif
#if _SCH_ENABLE_CALLLATER
_STATIC_INLINE void CallLater_Runner(void);
#endif
#if _SCH_ENABLE_SOFTINT
_STATIC_INLINE void SoftInt_Runner(void);
#endif

#pragma pack(1)
typedef struct {       // 用户任务结构
  void (*task)(void);  // 任务函数指针
  m_time_t period;     // 任务调度周期(Tick)
  m_time_t lastRun;    // 上次执行时间(Tick)
  uint8_t enable;      // 是否使能
  uint8_t priority;    // 优先级
  uint16_t taskId;     // 任务ID
#if _SCH_DEBUG_MODE
  uint8_t flag;         // 任务标志(1:新增, 2:删除, 4:设置变更)
  m_time_t max_cost;    // 任务最大执行时间(Tick)
  m_time_t total_cost;  // 任务总执行时间(Tick)
  m_time_t max_lat;     // 任务调度延迟(Tick)
  m_time_t total_lat;   // 任务调度延迟总和(Tick)
  uint16_t run_cnt;     // 任务执行次数
  float last_usage;     // 任务上次执行占用率
  char name[13];        // 任务名
#endif
} scheduler_task_t;
#pragma pack()

static struct {
  scheduler_task_t *tasks;                          // 任务存储区
  uint16_t num;                                     // 存储区总任务数量
  uint16_t size;                                    // 存储区总大小
  uint16_t priOffset[_SCH_MAX_PRIORITY_LEVEL + 1];  // 各优先级偏移量
  uint16_t tempId;                                  // 最大编号
} thd = {0};

#if _SCH_DEBUG_MODE
#include "log.h"
#warning 调度器调试模式已开启, 预期性能下降, 且任务句柄占用内存增加
static void delete_task(uint16_t i, bool skip_mem_free);
static scheduler_task_t *get_task(uint16_t taskId);

static inline void Debug_PrintInfo(void) {
  static uint8_t first_print = 1;
  static m_time_t _sch_debug_last_print = 0;
  m_time_t now = m_tick();
  if (first_print) {  // 因为初始化耗时等原因，第一次的数据无参考价值，不打印
    first_print = 0;
  } else {
    if (now - _sch_debug_last_print <= _SCH_DEBUG_PERIOD * m_tick_clk) return;
    double temp = m_tick_per_us;
    m_time_t period = now - _sch_debug_last_print;
    m_time_t other = period;
    LOG_RAWLN(T_FMT(T_RESET, T_BOLD, T_BLUE));
    LOG_RAWLN("--------------------- Scheduler Report ----------------------");
    LOG_RAWLN("PID | Pri | Run | Tmax  | Usage | LTavg | LTmax | Function");
    float usage;
    char op;
    for (uint16_t i = 0; i < thd.num; i++) {
      if (thd.tasks[i].enable) {
        usage = (double)thd.tasks[i].total_cost / period * 100;
        if (thd.tasks[i].flag & 0x01) {  // 任务新增
          LOG_RAW(T_FMT(T_RESET, T_BOLD, T_CYAN));
          op = '+';
        } else if (thd.tasks[i].flag & 0x04) {  // 任务优先级变更
          LOG_RAW(T_FMT(T_RESET, T_BOLD, T_MAGENTA));
          op = '>';
        } else if ((thd.tasks[i].last_usage != 0 &&
                    usage / thd.tasks[i].last_usage > 2) ||
                   usage > 20) {  // 任务占用率大幅度增加或者超过20%
          LOG_RAW(T_FMT(T_RESET, T_BOLD, T_YELLOW));
          op = '!';
        } else {
          LOG_RAW(T_FMT(T_RESET, T_GREEN));  // 正常
          op = ' ';
        }
        LOG_RAWLN("%c%-5d %-4d %-5d %-7.2f %-7.3f %-7.2f %-7.2f %s ", op,
                  thd.tasks[i].taskId, thd.tasks[i].priority,
                  thd.tasks[i].run_cnt, (double)thd.tasks[i].max_cost / temp,
                  usage,
                  (double)thd.tasks[i].total_lat / thd.tasks[i].run_cnt / temp,
                  (double)thd.tasks[i].max_lat / temp, thd.tasks[i].name);
        thd.tasks[i].last_usage = usage;
      } else {
        if (thd.tasks[i].flag & 0x02) {  // 任务已被删除
          LOG_RAW(T_FMT(T_RESET, T_BOLD, T_RED));
          op = '-';
        } else if (thd.tasks[i].flag & 0x04) {  // 任务设置变更
          LOG_RAW(T_FMT(T_RESET, T_BOLD, T_MAGENTA));
          op = '>';
        } else {
          LOG_RAW(T_FMT(T_RESET, T_WHITE));  // 禁用
          op = ' ';
        }
        LOG_RAWLN("%c%-5d %-4d %-5s %-7s %-7s %-7s %-7s %s ", op,
                  thd.tasks[i].taskId, thd.tasks[i].priority, "-", "-", "-",
                  "-", "-", thd.tasks[i].name);
        thd.tasks[i].last_usage = 0;
      }
      other -= thd.tasks[i].total_cost;
    }
    LOG_RAW(T_FMT(T_RESET, T_CYAN));
    LOG_RAW("Core: %.0fMhz / RunTime: %ds / Idle: %.3f%%", temp, m_time_s(),
            (double)other / period * 100);
#if _MOD_USE_DALLOC
    LOG_RAW(" / Mem: %.3f%%", get_def_heap_usage() * 100);
#endif
    LOG_RAWLN(T_FMT(T_BOLD, T_BLUE));
    LOG_RAWLN("-------------------------------------------------------------");
    LOG_RAW(T_RST);
    for (uint16_t i = 0; i < thd.num; i++) {
      if (thd.tasks[i].flag & 0x02) {
        delete_task(i, false);  // 实际删除已标记删除的任务
        i = 0;
      } else {
        thd.tasks[i].flag = 0;
      }
    }
  }
  now = m_tick();
  for (uint16_t i = 0; i < thd.num; i++) {
    thd.tasks[i].lastRun = now;
    thd.tasks[i].max_cost = 0;
    thd.tasks[i].total_cost = 0;
    thd.tasks[i].run_cnt = 0;
    thd.tasks[i].max_lat = 0;
    thd.tasks[i].total_lat = 0;
  }
  _sch_debug_last_print = now;
}
static inline void Debug_RunTask(scheduler_task_t *task_p, m_time_t latency) {
  __IO uint16_t id = task_p->taskId;
  m_time_t _sch_debug_task_tick = m_tick();
  task_p->task();
  _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
  if (task_p->taskId != id) {  // 任务已被修改, 重新查找
    task_p = get_task(id);
    if (task_p == NULL) return;  // 任务已被删除
  }
  if (task_p->max_cost < _sch_debug_task_tick)
    task_p->max_cost = _sch_debug_task_tick;
  if (latency > task_p->max_lat) task_p->max_lat = latency;
  task_p->total_cost += _sch_debug_task_tick;
  task_p->total_lat += latency;
  task_p->run_cnt++;
}

static inline void Debug_SetName(scheduler_task_t *p, const char *name) {
  for (uint8_t i = 0; i < sizeof(p->name); i++) {
    if (name[i] == '\0') {
      p->name[i] = '\0';
      return;
    }
    p->name[i] = name[i];
    if (i >= sizeof(p->name) - 3) {
      p->name[sizeof(p->name) - 3] = '.';
      p->name[sizeof(p->name) - 2] = '.';
      p->name[sizeof(p->name) - 1] = '\0';
      return;
    }
  }
}
#endif  // _SCH_DEBUG_MODE

/**
 * @brief 时分调度器主函数
 * @param  block            是否阻塞
 **/
void __attribute__((always_inline)) Scheduler_Run(const uint8_t block) {
  do {
    m_time_t latency;
    m_time_t now = m_tick();

    if (thd.num) {
#if _SCH_DEBUG_MODE
      Debug_PrintInfo();
#endif
      for (uint16_t i = 0; i < thd.num; i++) {
        if (thd.tasks[i].enable &&
            (now >= thd.tasks[i].lastRun + thd.tasks[i].period)) {
          latency = now - (thd.tasks[i].lastRun + thd.tasks[i].period);
          if (latency <= _SCH_COMP_RANGE)
            thd.tasks[i].lastRun += thd.tasks[i].period;
          else
            thd.tasks[i].lastRun = now;
#if _SCH_DEBUG_MODE
          Debug_RunTask(&thd.tasks[i], latency);
#else
          thd.tasks[i].task();
#endif  // _SCH_DEBUG_MODE
          break;
        }
      }
    }
#if _SCH_ENABLE_SOFTINT
    SoftInt_Runner();
#endif
#if _SCH_ENABLE_COROUTINE
    Cron_Runner();
#endif
#if _SCH_ENABLE_CALLLATER
    CallLater_Runner();
#endif
  } while (block);
}

static scheduler_task_t *insert_task(uint8_t priority) {
  scheduler_task_t *p;
  if (priority > _SCH_MAX_PRIORITY_LEVEL) priority = _SCH_MAX_PRIORITY_LEVEL;
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
  if (priority == _SCH_MAX_PRIORITY_LEVEL) {  // 直接添加到末尾
    p = &thd.tasks[thd.num];
  } else {  // 插入priOffset[priority+1]前
    p = &thd.tasks[thd.priOffset[priority + 1]];
    if (thd.priOffset[priority + 1] < thd.num)
      memmove(
          p + 1, p,
          sizeof(scheduler_task_t) * (thd.num - thd.priOffset[priority + 1]));
    for (uint16_t i = priority + 1; i <= _SCH_MAX_PRIORITY_LEVEL; i++) {
      thd.priOffset[i]++;
    }
  }
  thd.num += 1;
  memset(p, 0, sizeof(scheduler_task_t));
  return p;
}

static scheduler_task_t *get_task(uint16_t taskId) {
  for (uint16_t i = 0; i < thd.num; i++) {
    if (thd.tasks[i].taskId == taskId) return &thd.tasks[i];
  }
  return NULL;
}

#if !_SCH_DEBUG_MODE
uint16_t Sch_CreateTask(void (*task)(void), float freqHz, uint8_t enable,
                        uint8_t priority) {
#else
#include <string.h>
uint16_t _Sch_CreateTask(const char *name, void (*task)(void), float freqHz,
                         uint8_t enable, uint8_t priority) {
#endif
  if (thd.num == 0xFFFE) return 0xffff;
  scheduler_task_t *p = insert_task(priority);
  if (p == NULL) return 0xffff;
#if _SCH_DEBUG_MODE
  Debug_SetName(p, name);
#endif
  p->task = task;
  p->enable = enable;
  p->priority = priority;
  p->period = (double)m_tick_clk / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = m_tick() - p->period;
  p->taskId = 0xFFFF;
  for (uint16_t i = 0; i < thd.num; i++) {
    if (thd.tasks[i].taskId == thd.tempId) {
      thd.tempId += 1;
      i = 0;
    }
  }
  p->taskId = thd.tempId;
  thd.tempId += 1;
#if _SCH_DEBUG_MODE
  p->flag |= 1;  // 标记为新增
#endif
  return p->taskId;
}

static void delete_task(uint16_t i, bool skip_mem_free) {
  if (thd.tempId == thd.tasks[i].taskId + 1) thd.tempId = thd.tasks[i].taskId;
  if (i < thd.num - 1) {  // 不是最后一个, 需要移动
    memmove(&thd.tasks[i], &thd.tasks[i + 1],
            sizeof(scheduler_task_t) * (thd.num - i - 1));
    memset(&thd.tasks[thd.num - 1], 0, sizeof(scheduler_task_t));
  }
  for (uint8_t j = 0; j <= _SCH_MAX_PRIORITY_LEVEL; j++) {
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

void Sch_DeleteTask(uint16_t taskId) {
  if (thd.num == 0) return;
  for (uint16_t i = 0; i < thd.num; i++) {
    if (thd.tasks[i].taskId == taskId) {
#if _SCH_DEBUG_MODE
      if (!(thd.tasks[i].flag & 0x02)) {
        thd.tasks[i].flag |= 2;  // 标记为删除
        thd.tasks[i].enable = 0;
        return;
      }
#endif
      delete_task(i, false);
      return;
    }
  }
}

bool Sch_IsTaskExist(uint16_t taskId) { return get_task(taskId) != NULL; }

void Sch_SetTaskPriority(uint16_t taskId, uint8_t priority) {
  if (priority > _SCH_MAX_PRIORITY_LEVEL) priority = _SCH_MAX_PRIORITY_LEVEL;
  scheduler_task_t *p = get_task(taskId);
  if (p == NULL) return;
  scheduler_task_t temp = {0};
  memcpy(&temp, p, sizeof(scheduler_task_t));
  delete_task(p - thd.tasks, true);
  p = insert_task(priority);
  memcpy(p, &temp, sizeof(scheduler_task_t));
  p->priority = priority;
#if _SCH_DEBUG_MODE
  p->flag |= 4;
#endif
}

void Sch_DelayTask(uint16_t taskId, m_time_t delayUs, uint8_t fromNow) {
  scheduler_task_t *p = get_task(taskId);
  if (p == NULL) return;
  if (fromNow)
    p->lastRun = delayUs * m_tick_per_us + m_tick();
  else
    p->lastRun += delayUs * m_tick_per_us;
}

uint16_t Sch_GetTaskNum(void) { return thd.num; }

void Sch_SetTaskState(uint16_t taskId, uint8_t enable) {
  scheduler_task_t *p = get_task(taskId);
  if (p == NULL) return;
  if (enable == TOGGLE)
    p->enable = !p->enable;
  else
    p->enable = enable;
  if (p->enable) p->lastRun = m_tick() - p->period;
}

void Sch_SetTaskFreq(uint16_t taskId, float freqHz) {
  scheduler_task_t *p = get_task(taskId);
  if (p == NULL) return;
  p->period = (double)m_tick_clk / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = m_tick() - p->period;
#if _SCH_DEBUG_MODE
  p->flag |= 4;
#endif
}

uint16_t Sch_GetTaskId(void (*task)(void)) {
  for (uint16_t i = 0; i < thd.num; i++) {
    if (thd.tasks[i].task == task) return thd.tasks[i].taskId;
  }
  return 0xffff;
}

#if _SCH_ENABLE_COROUTINE
#pragma pack(1)
typedef struct {        // 协程任务结构
  void (*task)(void);   // 任务函数指针
  uint8_t enable;       // 是否使能
  uint8_t mode;         // 模式
  uint16_t taskId;      // 任务ID
  m_time_t yieldUntil;  // 等待态结束时间(us)
  long ptr;             // 协程跳入地址
} scheduler_cron_t;
#pragma pack()

struct {
  scheduler_cron_t *tasks;  // 任务存储区
  uint16_t num;             // 存储区总任务数量
  uint16_t size;            // 存储区总大小
  uint16_t tempId;          // 最大编号
} chd = {0};

_cron_handle_t _cron_hp = {0};

_STATIC_INLINE void Cron_Runner(void) {
  for (uint16_t i = 0; i < chd.num; i++) {
    if (chd.tasks[i].enable && m_time_us() >= chd.tasks[i].yieldUntil) {
      _cron_hp.ptr = chd.tasks[i].ptr;
      _cron_hp.delay = 0;
      chd.tasks[i].task();  // 执行
      chd.tasks[i].ptr = _cron_hp.ptr;
      chd.tasks[i].yieldUntil = _cron_hp.delay;
      if (!chd.tasks[i].ptr) {
        if (chd.tasks[i].mode == CR_MODE_AUTODEL) {
          Sch_DeleteCoron(chd.tasks[i].taskId);
          return;  // 指针已被释放
        } else if (chd.tasks[i].mode == CR_MODE_ONESHOT) {
          chd.tasks[i].enable = 0;
        }
      }
    }
  }
}

uint16_t Sch_CreateCoron(void (*task)(void), uint8_t enable,
                         enum CR_MODES mode) {
  if (chd.num == 0xFFFE) return 0xffff;
  if (chd.tasks == NULL) {
    m_alloc(chd.tasks, sizeof(scheduler_cron_t) * 2);
    if (chd.tasks == NULL) return 0xffff;
    chd.size = 2;
  }
  if (chd.num + 1 > chd.size) {
    scheduler_cron_t *old = chd.tasks;
    if (!m_realloc(chd.tasks, sizeof(scheduler_cron_t) * chd.size * 2)) {
      chd.tasks = old;
      return 0xffff;
    }
    chd.size *= 2;
  }
  scheduler_cron_t *p = &chd.tasks[chd.num];
  chd.num += 1;
  p->task = task;
  p->enable = enable;
  p->mode = mode;
  p->yieldUntil = 0;
  p->ptr = NULL;
  p->taskId = 0xFFFF;
RETRY_ID_CHECK:
  for (uint16_t i = 0; i < chd.num; i++) {
    if (chd.tasks[i].taskId == chd.tempId) {
      chd.tempId += 1;
      goto RETRY_ID_CHECK;
    }
  }
  p->taskId = chd.tempId;
  chd.tempId += 1;
  return p->taskId;
}

void Sch_DeleteCoron(uint16_t taskId) {
  if (chd.num == 0) return;
  for (uint16_t i = 0; i < chd.num; i++) {
    if (chd.tasks[i].taskId == taskId) {
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
      return;
    }
  }
}

void Sch_SetCoronState(uint16_t taskId, uint8_t enable, uint8_t clearState) {
  for (uint16_t i = 0; i < chd.num; i++) {
    if (chd.tasks[i].taskId == taskId) {
      if (enable == TOGGLE) {
        chd.tasks[i].enable = !chd.tasks[i].enable;
      } else {
        chd.tasks[i].enable = enable;
      }
      if (clearState) {
        chd.tasks[i].ptr = 0;
        chd.tasks[i].yieldUntil = 0;
      }
      break;
    }
  }
}

uint16_t Sch_GetCoronNum(void) { return chd.num; }

uint16_t Sch_GetCoronId(void (*task)(void)) {
  for (uint16_t i = 0; i < chd.num; i++) {
    if (chd.tasks[i].task == task) return chd.tasks[i].taskId;
  }
  return 0xffff;
}

bool Sch_IsCoronExist(uint16_t taskId) {
  for (uint16_t i = 0; i < chd.num; i++) {
    if (chd.tasks[i].taskId == taskId) return true;
  }
  return false;
}

#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_CALLLATER
#pragma pack(1)
typedef struct {       // 延时调用任务结构
  void (*task)(void);  // 任务函数指针
  m_time_t runTimeUs;  // 执行时间(us)
  void *next;
} scheduler_call_later_t;
#pragma pack()
scheduler_call_later_t *schCallLaterEntry = NULL;
scheduler_call_later_t *callLater_p = NULL;

_STATIC_INLINE void CallLater_Runner(void) {
  if (schCallLaterEntry != NULL) {
    if (callLater_p == NULL) callLater_p = schCallLaterEntry;
    if (m_time_us() >= callLater_p->runTimeUs) {
      callLater_p->task();
      Sch_CancelCallLater(callLater_p->task);
      return;
    }
    callLater_p = callLater_p->next;
  }
}

bool Sch_CallLater(void (*task)(void), m_time_t delayUs) {
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
  p->task = task;
  p->runTimeUs = delayUs + m_time_us();
  p->next = NULL;
  return true;
}

void Sch_CancelCallLater(void (*task)(void)) {
  scheduler_call_later_t *p = schCallLaterEntry;
  scheduler_call_later_t *q = NULL;
  void **temp = NULL;
  while (p != NULL) {
    if (p->task == task) {
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

static uint8_t _imm = 0;
static uint8_t _ism[8] = {0};

void Sch_TriggerSoftInt(uint8_t mainChannel, uint8_t subChannel) {
  if (mainChannel > 7 || subChannel > 7) return;
  _imm |= 1 << mainChannel;
  _ism[mainChannel] |= 1 << subChannel;
}

__weak void Scheduler_SoftIntHandler_Ch0(uint8_t subMask) {}
__weak void Scheduler_SoftIntHandler_Ch1(uint8_t subMask) {}
__weak void Scheduler_SoftIntHandler_Ch2(uint8_t subMask) {}
__weak void Scheduler_SoftIntHandler_Ch3(uint8_t subMask) {}
__weak void Scheduler_SoftIntHandler_Ch4(uint8_t subMask) {}
__weak void Scheduler_SoftIntHandler_Ch5(uint8_t subMask) {}
__weak void Scheduler_SoftIntHandler_Ch6(uint8_t subMask) {}
__weak void Scheduler_SoftIntHandler_Ch7(uint8_t subMask) {}

_STATIC_INLINE void SoftInt_Runner(void) {
  if (_imm) {
    __IO uint8_t imm = _imm;
    _imm = 0;
    if (imm & 0x01) Scheduler_SoftIntHandler_Ch0(_ism[0]), _ism[0] = 0;
    if (imm & 0x02) Scheduler_SoftIntHandler_Ch1(_ism[1]), _ism[1] = 0;
    if (imm & 0x04) Scheduler_SoftIntHandler_Ch2(_ism[2]), _ism[2] = 0;
    if (imm & 0x08) Scheduler_SoftIntHandler_Ch3(_ism[3]), _ism[3] = 0;
    if (imm & 0x10) Scheduler_SoftIntHandler_Ch4(_ism[4]), _ism[4] = 0;
    if (imm & 0x20) Scheduler_SoftIntHandler_Ch5(_ism[5]), _ism[5] = 0;
    if (imm & 0x40) Scheduler_SoftIntHandler_Ch6(_ism[6]), _ism[6] = 0;
    if (imm & 0x80) Scheduler_SoftIntHandler_Ch7(_ism[7]), _ism[7] = 0;
  }
}

#endif  // _SCH_ENABLE_SOFTINT

#if _SCH_ENABLE_TERMINAL
#include "log.h"
#include "nr_micro_shell.h"
#include "stdlib.h"
#include "string.h"
void sch_cmd(char argc, char *argv) {
  if (argc < 2) {
    LOG_RAWLN(T_FMT(
        T_BOLD,
        T_BLUE) "Usage: sch [list|enable|disable|delete|setfreq|setpri|excute] "
                "[taskid] "
                "[freq]" T_RST);
    return;
  }
  if (strcmp(argv + argv[1], "list") == 0) {
    LOG_RAWLN(T_FMT(T_BOLD, T_BLUE) "Tasks list:");
    for (uint16_t i = 0; i < thd.num; i++) {
      LOG_RAWLN("Task %d: 0x%p pri:%d freq:%.1f en:%d", thd.tasks[i].taskId,
                thd.tasks[i].task, thd.tasks[i].priority,
                (double)m_tick_clk / thd.tasks[i].period, thd.tasks[i].enable);
    }
    LOG_RAWLN("Total %d tasks" T_RST, thd.num);
    return;
  }
  if (argc < 3) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Task ID is required" T_RST);
    return;
  }
  uint16_t taskId = atoi(argv + argv[2]);
  scheduler_task_t *p = get_task(taskId);
  if (p == NULL) {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Task %d not found" T_RST, taskId);
    return;
  }
  if (strcmp(argv + argv[1], "enable") == 0) {
    Sch_SetTaskState(taskId, ENABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task %d enabled" T_RST, taskId);
  } else if (strcmp(argv + argv[1], "disable") == 0) {
    Sch_SetTaskState(taskId, DISABLE);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task %d disabled" T_RST, taskId);
  } else if (strcmp(argv + argv[1], "delete") == 0) {
    Sch_DeleteTask(taskId);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task %d deleted" T_RST, taskId);
  } else if (strcmp(argv + argv[1], "setfreq") == 0) {
    if (argc < 4) {
      LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Frequency is required" T_RST);
      return;
    }
    float freq = atof(argv + argv[3]);
    Sch_SetTaskFreq(taskId, freq);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task %d frequency set to %.2fHz" T_RST,
              taskId, freq);
  } else if (strcmp(argv + argv[1], "setpri") == 0) {
    if (argc < 4) {
      LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Priority is required" T_RST);
      return;
    }
    uint8_t pri = atoi(argv + argv[3]);
    Sch_SetTaskPriority(taskId, pri);
    LOG_RAWLN(T_FMT(T_BOLD, T_GREEN) "Task %d priority set to %d" T_RST, taskId,
              pri);
  } else if (strcmp(argv + argv[1], "excute") == 0) {
    LOG_RAWLN(T_FMT(T_BOLD, T_YELLOW) "Force excuting task %d" T_RST, taskId);
    p->task();
  } else {
    LOG_RAWLN(T_FMT(T_BOLD, T_RED) "Unknown command" T_RST);
  }
}
ADD_CMD(sch, sch_cmd);
#endif  // _SCH_ENABLE_TERMINAL

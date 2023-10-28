#include <scheduler.h>

#if _SCH_ENABLE_COROUTINE
static void Cron_Runner(void);
#endif
#if _SCH_ENABLE_CALLLATER
static void CallLater_Runner(void);
#endif

#pragma pack(1)
typedef struct {       // 用户任务结构
  void (*task)(void);  // 任务函数指针
  m_time_t period;     // 任务调度周期(Tick)
  m_time_t lastRun;    // 上次执行时间(Tick)
  uint8_t enable;      // 是否使能
  uint16_t taskId;     // 任务ID
#if _SCH_DEBUG_MODE
  m_time_t max_cost;    // 任务最大执行时间(Tick)
  m_time_t total_cost;  // 任务总执行时间(Tick)
  m_time_t max_lat;     // 任务调度延迟(Tick)
  m_time_t total_lat;   // 任务调度延迟总和(Tick)
  uint16_t run_cnt;     // 任务执行次数
  char name[13];        // 任务名
#endif
} scheduler_task_t;
#pragma pack()

static struct {
  scheduler_task_t *tasks;                  // 任务存储区
  uint16_t num;                             // 存储区总任务数量
  uint16_t size;                            // 存储区总大小
  uint16_t priOffset[_SCH_MAX_PRIORITY_LEVEL + 1];  // 各优先级偏移量
  uint16_t maxId;                           // 最大编号
} thd = {0};

#if _SCH_DEBUG_MODE
#warning 调度器调试模式已开启, 预期性能下降, 且任务句柄占用内存增加
#include <log.h>

static inline void Debug_PrintInfo(void) {
  static uint8_t first_print = 1;
  static m_time_t _sch_debug_last_print = 0;
  m_time_t now = m_tick();
  if (now - _sch_debug_last_print <= _SCH_DEBUG_PERIOD * m_tick_clk) return;
  if (first_print) {
    LOG_RAW("The first report is skipped due to inaccurate data.\r\n");
    first_print = 0;
  } else {
    double temp = m_tick_per_us;
    m_time_t period = now - _sch_debug_last_print;
    m_time_t other = period;
    LOG_RAW(
        "\r\n"
        "--------------------- Scheduler Report ----------------------\r\n");
    LOG_RAW(" ID | PRI | Run | Tmax  | Usage | Lavg  | Lmax  | Function\r\n");
    uint8_t pri = 0;
    for (uint16_t i = 0; i < thd.num; i++) {
      if (pri != _SCH_MAX_PRIORITY_LEVEL && i == thd.priOffset[pri + 1]) pri++;
      if (thd.tasks[i].enable)
        LOG_RAW(" #%-4d %d    %-5d %-7.2f %-7.3f %-7.2f %-7.2f %s \r\n",
                thd.tasks[i].taskId, pri, thd.tasks[i].run_cnt,
                (double)thd.tasks[i].max_cost / temp,
                (double)thd.tasks[i].total_cost / period * 100,
                (double)thd.tasks[i].total_lat / thd.tasks[i].run_cnt / temp,
                (double)thd.tasks[i].max_lat / temp, thd.tasks[i].name);
      else {
        LOG_RAW(" #%-4d %d    %-5s %-7s %-7s %-7s %-7s %s \r\n",
                thd.tasks[i].taskId, pri, "-", "-", "-", "-", "-",
                thd.tasks[i].name);
      }
      other -= thd.tasks[i].total_cost;
    }
#if _MOD_USE_DALLOC
    LOG_RAW("Core: %.0fMhz / RunTime: %ds / Idle: %.3f%% / Mem: %.3f%%\r\n",
            temp, m_time_s(), (double)other / period * 100,
            get_def_heap_usage() * 100);
#else
    LOG_RAW("Core: %.0fMhz / RunTime: %ds / Idle: %.3f%%\r\n", temp, m_time_s(),
            (double)other / period * 100);
#endif
    LOG_RAW(
        "-------------------------------------------------------------\r\n");
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
  m_time_t _sch_debug_task_tick = m_tick();
  task_p->task();
  _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
  if (task_p->max_cost < _sch_debug_task_tick)
    task_p->max_cost = _sch_debug_task_tick;
  if (latency > task_p->max_lat) task_p->max_lat = latency;
  task_p->total_cost += _sch_debug_task_tick;
  task_p->total_lat += latency;
  task_p->run_cnt++;
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
#if _SCH_DEBUG_MODE
          Debug_RunTask(&thd.tasks[i], latency);
#else
          thd.tasks[i].task();
#endif  // _SCH_DEBUG_MODE
          if (latency <= _SCH_COMP_RANGE)
            thd.tasks[i].lastRun += thd.tasks[i].period;
          else
            thd.tasks[i].lastRun = now;
          break;
        }
      }
    }

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
    if (!m_realloc(thd.tasks, sizeof(scheduler_task_t) * thd.size * 2)) {
      thd.tasks = old;
      return NULL;
    }
    thd.size *= 2;
  }
  if (priority == _SCH_MAX_PRIORITY_LEVEL) {  // 直接添加到末尾
    p = &thd.tasks[thd.num];
  } else {  // 插入priOffset[priority+1]前
    p = &thd.tasks[thd.priOffset[priority + 1]];
    memmove(p + 1, p,
            sizeof(scheduler_task_t) * (thd.num - thd.priOffset[priority + 1]));
    for (uint16_t i = priority + 1; i <= _SCH_MAX_PRIORITY_LEVEL; i++) {
      thd.priOffset[i]++;
    }
  }
  thd.num += 1;
  return p;
}

static scheduler_task_t *get_task(uint16_t taskId) {
  for (uint16_t i = 0; i < thd.num; i++) {
    if (thd.tasks[i].taskId == taskId) return &thd.tasks[i];
  }
  return NULL;
}

#if !_SCH_DEBUG_MODE
/**
 * @brief 创建一个调度任务
 * @param  task             任务函数指针
 * @param  freqHz           任务调度频率
 * @param  enable           初始化时是否使能
 * @param  priority         任务优先级
 * @retval uint16_t          任务ID (0xFFFF表示添加失败)
 */
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
  for (uint8_t i = 0; i < sizeof(p->name); i++) {
    if (name[i] == '\0') {
      p->name[i] = '\0';
      break;
    }
    p->name[i] = name[i];
    if (i >= sizeof(p->name) - 3) {
      p->name[sizeof(p->name) - 3] = '.';
      p->name[sizeof(p->name) - 2] = '.';
      p->name[sizeof(p->name) - 1] = '\0';
      break;
    }
  }
#endif
  p->task = task;
  p->enable = enable;
  p->period = (double)m_tick_clk / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = m_tick() - p->period;
  p->taskId = 0xFFFF;
RETRY_ID_CHECK:
  for (uint16_t i = 0; i < thd.num; i++) {
    if (thd.tasks[i].taskId == thd.maxId) {
      thd.maxId += 1;
      goto RETRY_ID_CHECK;
    }
  }
  p->taskId = thd.maxId;
  thd.maxId += 1;
  return p->taskId;
}

/**
 * @brief 删除一个调度任务
 * @param  taskId           目标任务ID
 */
void Sch_DeleteTask(uint16_t taskId) {
  if (thd.num == 0) return;
  for (uint16_t i = 0; i < thd.num; i++) {
    if (thd.tasks[i].taskId == taskId) {
      if (i < thd.num - 1) {  // 不是最后一个, 需要移动
        memmove(&thd.tasks[i], &thd.tasks[i + 1],
                sizeof(scheduler_task_t) * (thd.num - i - 1));
      }
      for (uint8_t j = 0; j <= _SCH_MAX_PRIORITY_LEVEL; j++) {
        if (thd.priOffset[j] > i) thd.priOffset[j]--;
      }
      thd.num -= 1;
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
      return;
    }
  }
}

/**
 * @brief 删除所有调度任务
 */
void Sch_DeleteAllTasks(void) {
  if (thd.num == 0) return;
  m_free(thd.tasks);
  thd.tasks = NULL;
  thd.num = 0;
  thd.size = 0;
  thd.maxId = 0;
}

/**
 * @brief 通过任务ID查询任务是否存在
 * @param  taskId           目标任务ID
 * @retval bool             任务是否存在
 */
bool Sch_IsTaskExist(uint16_t taskId) { return get_task(taskId) != NULL; }

/**
 * @brief 设置任务优先级
 * @param  taskId           目标任务ID
 * @param  priority         任务优先级
 */
void Sch_SetTaskPriority(uint16_t taskId, uint8_t priority) {
  scheduler_task_t *p = get_task(taskId);
  if (p == NULL) return;
  scheduler_task_t temp = {0};
  memcpy(&temp, p, sizeof(scheduler_task_t));
  Sch_DeleteTask(taskId);
  p = insert_task(priority);
  memcpy(p, &temp, sizeof(scheduler_task_t));
}

/**
 * @brief 获取调度器内任务数量
 */
uint16_t Sch_GetTaskNum(void) { return thd.num; }

/**
 * @brief 切换任务使能状态
 * @param  taskId           目标任务ID
 * @param  enable           使能状态(0xff:切换)
 */
void Sch_SetTaskState(uint16_t taskId, uint8_t enable) {
  scheduler_task_t *p = get_task(taskId);
  if (p == NULL) return;
  if (enable == TOGGLE)
    p->enable = !p->enable;
  else
    p->enable = enable;
}

/**
 * @brief 设置任务调度频率
 * @param  taskId           目标任务ID
 * @param  freq             调度频率
 */
void Sch_SetTaskFreq(uint16_t taskId, float freqHz) {
  scheduler_task_t *p = get_task(taskId);
  if (p == NULL) return;
  p->period = (double)m_tick_clk / (double)freqHz;
  if (!p->period) p->period = 1;
  p->lastRun = m_tick() - p->period;
}

/**
 * @brief 查询指定函数对应的任务ID
 * @param  task             目标任务函数指针
 * @retval uint16_t         任务ID (0xFFFF:未找到)
 */
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
  uint16_t maxId;           // 最大编号
} chd = {0};

_cron_handle_t _cron_hp = {0};

static __attribute__((always_inline)) void Cron_Runner(void) {
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

/**
 * @brief 创建一个协程
 * @param  task             任务函数指针
 * @param  enable           是否立即启动
 * @param  mode             模式(CR_MODE_xxx)
 * @retval uint16_t         任务ID (0xFFFF表示堆内存分配失败)
 */
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
    if (chd.tasks[i].taskId == chd.maxId) {
      chd.maxId += 1;
      goto RETRY_ID_CHECK;
    }
  }
  p->taskId = chd.maxId;
  chd.maxId += 1;
  return p->taskId;
}

/**
 * @brief 删除一个协程
 * @param  taskId           目标任务ID
 */
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

/**
 * @brief 设置协程使能状态
 * @param  taskId           目标任务ID
 * @param  enable           使能状态(0xff: 切换)
 * @param  clearState      是否清除协程状态(从头开始执行)
 */
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

/**
 * @brief 获取调度器内协程数量
 */
uint16_t Sch_GetCoronNum(void) { return chd.num; }

/**
 * @brief 查询指定函数对应的协程ID
 * @param  task             目标任务函数指针
 * @retval uint16_t         任务ID (0xFFFF:未找到)
 */
uint16_t Sch_GetCoronId(void (*task)(void)) {
  for (uint16_t i = 0; i < chd.num; i++) {
    if (chd.tasks[i].task == task) return chd.tasks[i].taskId;
  }
  return 0xffff;
}

/**
 * @brief 查询指定任务ID对应的协程是否存在
 * @param  taskId           目标任务ID
 * @retval bool             协程是否存在
 */
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

static __attribute__((always_inline)) void CallLater_Runner(void) {
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

/**
 * @brief 在指定时间后执行目标函数
 * @param  task             任务函数指针
 * @param  delayUs          延时启动时间(us)
 * @retval bool             是否成功
 */
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

/**
 * @brief 取消所有对应函数的延时调用任务
 * @param task              任务函数指针
 */
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

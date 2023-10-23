#include <scheduler.h>

#pragma pack(1)
typedef struct {       // 用户任务结构
  void (*task)(void);  // 任务函数指针
  m_time_t period;     // 任务调度周期(Tick)
  m_time_t lastRun;    // 上次执行时间(Tick)
  uint8_t enable;      // 是否使能
  uint8_t dTParam;     // 是否传递dT参数
  uint16_t taskId;     // 任务ID
#if _SCH_DEBUG_MODE
  m_time_t max_cost;    // 任务最大执行时间(Tick)
  m_time_t total_cost;  // 任务总执行时间(Tick)
  m_time_t max_lat;     // 任务调度延迟(Tick)
  m_time_t total_lat;   // 任务调度延迟总和(Tick)
  uint16_t run_cnt;     // 任务执行次数
#endif
  void *next;
} scheduler_task_t;
#pragma pack()

scheduler_task_t *schTaskEntry = NULL;
scheduler_task_t *task_p = NULL;
#if _SCH_ENABLE_HIGH_PRIORITY
scheduler_task_t *task_highPriority_p = NULL;
scheduler_task_t *task_hpBack_p = NULL;
static uint8_t schTask_hpJmp = 0;
#endif

#if _SCH_ENABLE_COROUTINE
#pragma pack(1)
typedef struct {       // 协程任务结构
  void (*task)(void);  // 任务函数指针
  uint8_t enable;      // 是否使能
  uint8_t mode;        // 协程模式
  uint16_t taskId;     // 任务ID
  m_time_t idleUntil;  // 无阻塞延时结束时间(us)
  long ptr;            // 协程跳入地址
  void *next;
} scheduler_cron_t;
#pragma pack()
scheduler_cron_t *schCronEntry = NULL;
scheduler_cron_t *cron_p = NULL;
_cron_handle_t _cron_hp = {0};
#endif

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
#endif

#if _SCH_DEBUG_MODE
#warning 调度器调试模式已开启, 预期性能下降
#include <log.h>

void Print_Debug_info(m_time_t period) {
  static uint8_t first_print = 1;
  double temp = m_tick_per_us;
  m_time_t other = period;
  scheduler_task_t *p = schTaskEntry;
  LOG_RAW("\r\n-------------- Task %ds stat ----------------\r\n",
          _SCH_DEBUG_PERIOD);
  LOG_RAW(" ID | Run | Tmax/us | Usage/%% | Late(max)/us\r\n");
  while (p != NULL) {
    if (p->enable) {
      LOG_RAW(
          " #%-3d %-5d %-9.3f %-9.3f %.3f(%.2f) \r\n", p->taskId, p->run_cnt,
          (double)p->max_cost / temp, (double)p->total_cost / period * 100,
          (double)p->total_lat / p->run_cnt / temp, (double)p->max_lat / temp);
    }
    other -= p->total_cost;
    p = p->next;
  }
  LOG_RAW("Other: %.3f %% (%.3fus)\r\n", (double)other / period * 100,
          (double)other / temp);
  LOG_RAW("Coreclock: %.3f Mhz\r\n", temp);
  if (first_print) {
    LOG_RAW("Note: the first report is inaccurate.\r\n");
    first_print = 0;
  }
  LOG_RAW("---------------------------------------------\r\n");
  p = schTaskEntry;
  other = m_tick();
  while (p != NULL) {
    p->lastRun = other;
    p->max_cost = 0;
    p->total_cost = 0;
    p->run_cnt = 0;
    p->max_lat = 0;
    p->total_lat = 0;
    p = p->next;
  }
}

#endif  // _SCH_DEBUG_MODE

/**
 * @brief 时分调度器主函数
 * @param  block            是否阻塞
 **/
void __attribute__((always_inline)) Scheduler_Run(const uint8_t block) {
  static m_time_t now = 0;
  static m_time_t latency = 0;
#if _SCH_DEBUG_MODE
  static m_time_t _sch_debug_task_tick = 0;
  static m_time_t _sch_debug_last_print = 0;
#endif  // _SCH_DEBUG_MODE

  do {
#if _SCH_DEBUG_MODE
    if (task_p == NULL &&
        m_tick() - _sch_debug_last_print > _SCH_DEBUG_PERIOD * m_tick_clk) {
      Print_Debug_info(m_tick() - _sch_debug_last_print);
      _sch_debug_last_print = m_tick();
    }
#endif  // _SCH_DEBUG_MODE

  RUN_SCH:
    now = m_tick();
    if (schTaskEntry != NULL) {
      if (task_p == NULL) task_p = schTaskEntry;
      if (task_p->enable && (now >= task_p->lastRun + task_p->period)) {
        latency = now - (task_p->lastRun + task_p->period);
#if _SCH_DEBUG_MODE
        _sch_debug_task_tick = m_tick();
        task_p->task();
        _sch_debug_task_tick = m_tick() - _sch_debug_task_tick;
        if (task_p->max_cost < _sch_debug_task_tick)
          task_p->max_cost = _sch_debug_task_tick;
        if (latency > task_p->max_lat) task_p->max_lat = latency;
        task_p->total_cost += _sch_debug_task_tick;
        task_p->total_lat += latency;
        task_p->run_cnt++;
#else
        if (task_p->dTParam)
          ((void (*)(m_time_t))task_p->task)(now - task_p->lastRun);
        else
          task_p->task();
#endif  // _SCH_DEBUG_MODE
        if (latency <= _SCH_COMP_RANGE)
          task_p->lastRun += task_p->period;
        else
          task_p->lastRun = now;
      }
      if (task_p != NULL) task_p = task_p->next;

#if _SCH_ENABLE_HIGH_PRIORITY
      if (task_highPriority_p != NULL && !schTask_hpJmp) {
        task_hpBack_p = task_p;
        task_p = task_highPriority_p;
        schTask_hpJmp = 1;
        goto RUN_SCH;
      } else if (schTask_hpJmp) {
        task_p = task_hpBack_p;
        schTask_hpJmp = 0;
      }
#endif  // _SCH_ENABLE_HIGH_PRIORITY
    }

#if _SCH_ENABLE_COROUTINE
    if (schCronEntry != NULL) {
      if (cron_p == NULL) cron_p = schCronEntry;
      if (cron_p->enable && m_time_us() >= cron_p->idleUntil) {
        _cron_hp.ptr = cron_p->ptr;
        _cron_hp.delay = 0;
        cron_p->task();  // 执行
        cron_p->ptr = _cron_hp.ptr;
        cron_p->idleUntil = _cron_hp.delay;
        if (!cron_p->ptr) {
          if (cron_p->mode == CORON_MODE_AUTODEL) {
            Sch_DelCoron(cron_p->taskId);
            continue;
          } else if (cron_p->mode == CORON_MODE_ONESHOT)
            cron_p->enable = 0;
        }
      }
      cron_p = cron_p->next;
    }
#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_CALLLATER
    if (schCallLaterEntry != NULL) {
      if (callLater_p == NULL) callLater_p = schCallLaterEntry;
      if (m_time_us() >= callLater_p->runTimeUs) {
        callLater_p->task();
        Sch_CancelCallLater(callLater_p->task);
        continue;
      }
      callLater_p = callLater_p->next;
    }
#endif  // _SCH_ENABLE_CALLLATER

  } while (block);
}

/**
 * @brief 添加一个任务到调度器
 * @param  task             任务函数指针
 * @param  freqHz           任务调度频率
 * @param  enable           初始化时是否使能
 * @retval uint16_t          任务ID (0xFFFF表示堆内存分配失败)
 */
uint16_t Sch_AddTask(void (*task)(void), float freqHz, uint8_t enable) {
  scheduler_task_t *p = NULL;
  if (schTaskEntry == NULL) {
    m_alloc(schTaskEntry, sizeof(scheduler_task_t));
    p = schTaskEntry;
    if (p == NULL) return 0xffff;
    p->taskId = 0;
  } else {
    scheduler_task_t *q = schTaskEntry;
    while (q->next != NULL) {
      q = q->next;
    }
    m_alloc(q->next, sizeof(scheduler_task_t));
    p = q->next;
    if (p == NULL) return 0xffff;
    p->taskId = q->taskId + 1;
  }
  p->task = task;
  p->period = (double)m_tick_clk / (double)freqHz;
  p->lastRun = m_tick() - p->period;
  p->enable = enable;
  p->dTParam = 0;
  p->next = NULL;
  return p->taskId;
}

/**
 * @brief 添加一个任务到调度器, 调度时传递上一次调度到本次调度的时间差
 * @param  task             任务函数指针
 * @param  freqHz           任务调度频率
 * @param  enable           初始化时是否使能
 * @retval uint16_t          任务ID (0xFFFF表示堆内存分配失败)
 */
uint16_t Sch_AddTask_dT(void (*task)(m_time_t dT), float freqHz,
                        uint8_t enable) {
  scheduler_task_t *p = NULL;
  if (schTaskEntry == NULL) {
    m_alloc(schTaskEntry, sizeof(scheduler_task_t));
    p = schTaskEntry;
    if (p == NULL) return 0xffff;
    p->taskId = 0;
  } else {
    scheduler_task_t *q = schTaskEntry;
    while (q->next != NULL) {
      q = q->next;
    }
    m_alloc(q->next, sizeof(scheduler_task_t));
    p = q->next;
    if (p == NULL) return 0xffff;
    p->taskId = q->taskId + 1;
  }
  p->task = (void *)task;
  p->period = (double)m_tick_clk / (double)freqHz;
  p->lastRun = m_tick() - p->period;
  p->enable = enable;
  p->dTParam = 1;
  p->next = NULL;
  return p->taskId;
}

/**
 * @brief 切换任务使能状态
 * @param  taskId           目标任务ID
 * @param  enable           使能状态(0xff:切换)
 */
void Sch_SetTaskState(uint16_t taskId, uint8_t enable) {
  scheduler_task_t *p = schTaskEntry;
  while (p != NULL) {
    if (p->taskId == taskId) {
      if (enable == TOGGLE)
        p->enable = !p->enable;
      else
        p->enable = enable;
      break;
    }
    p = p->next;
  }
}

/**
 * @brief 删除一个任务
 * @param  taskId           目标任务ID
 */
void Sch_DelTask(uint16_t taskId) {
  scheduler_task_t *p = schTaskEntry;
  scheduler_task_t *q = NULL;
  void **temp = NULL;
  while (p != NULL) {
    if (p->taskId == taskId) {
      if (q == NULL) {
        temp = (void **)&(p->next);
        m_free(schTaskEntry);
        if (*temp != NULL) m_replace(*temp, (schTaskEntry));
      } else {
        temp = (void **)&(p->next);
        m_free(q->next);
        if (*temp != NULL) m_replace(*temp, (q->next));
      }
      if (task_p == p) task_p = p->next;
#if _SCH_ENABLE_HIGH_PRIORITY
      if (task_highPriority_p == p) task_highPriority_p = NULL;
#endif
      break;
    }
    q = p;
    p = p->next;
  }
}

/**
 * @brief 设置任务调度频率
 * @param  taskId           目标任务ID
 * @param  freq             调度频率
 */
void Sch_SetTaskFreq(uint16_t taskId, float freqHz) {
  scheduler_task_t *p = schTaskEntry;
  while (p != NULL) {
    if (p->taskId == taskId) {
      p->period = (double)m_tick_clk / (double)freqHz;
      if (p->period == 0) {
        p->period = 1;
      }
      break;
    }
    p = p->next;
  }
}

/**
 * @brief 获取调度器内任务数量
 */
int Sch_GetTaskNum(void) {
  scheduler_task_t *p = schTaskEntry;
  int num = 0;
  while (p != NULL) {
    num++;
    p = p->next;
  }
  return num;
}

/**
 * @brief 查询指定函数对应的任务ID
 * @param  task             目标任务函数指针
 * @retval uint16_t         任务ID (0xFFFF:未找到)
 */
uint16_t Sch_GetTaskId(void (*task)(void)) {
  scheduler_task_t *p = schTaskEntry;
  while (p != NULL) {
    if (p->task == task) {
      return p->taskId;
    }
    p = p->next;
  }
  return 0xffff;
}

#if _SCH_ENABLE_HIGH_PRIORITY
/**
 * @brief 设置高优先级任务(仅支持一个)
 * @param  taskId           目标任务ID (0xFFFF:取消)
 */
void Sch_SetHighPriorityTask(uint16_t taskId) {
  scheduler_task_t *p = schTaskEntry;
  task_highPriority_p = NULL;
  if (taskId == 0xffff) return;
  while (p != NULL) {
    if (p->taskId == taskId) {
      task_highPriority_p = p;
      break;
    }
    p = p->next;
  }
}
#endif

#if _SCH_ENABLE_COROUTINE

/**
 * @brief 添加一个协程
 * @param  task             任务函数指针
 * @param  enable           是否立即启动
 * @param  mode             协程模式(CRON_MODE_xxx)
 * @param  delay            延时启动时间(us)
 * @retval uint16_t         任务ID (0xFFFF表示堆内存分配失败)
 */
uint16_t Sch_AddCoron(void (*task)(void), uint8_t enable, enum CORON_MODES mode,
                      m_time_t delay) {
  scheduler_cron_t *p = NULL;
  if (schCronEntry == NULL) {
    m_alloc(schCronEntry, sizeof(scheduler_cron_t));
    p = schCronEntry;
    if (p == NULL) return 0xffff;
    p->taskId = 0;
  } else {
    scheduler_cron_t *q = schCronEntry;
    while (q->next != NULL) {
      q = q->next;
    }
    m_alloc(q->next, sizeof(scheduler_cron_t));
    p = q->next;
    if (p == NULL) return 0xffff;
    p->taskId = q->taskId + 1;
  }
  p->task = task;
  p->enable = enable;
  p->mode = mode;
  p->idleUntil = delay + m_time_us();
  p->next = NULL;
  p->ptr = NULL;
  return p->taskId;
}

/**
 * @brief 设置协程使能状态
 * @param  taskId           目标任务ID
 * @param  enable           使能状态(0xff: 切换)
 * @param  clearState      是否清除协程状态(从头开始执行)
 */
void Sch_SetCoronState(uint16_t taskId, uint8_t enable, uint8_t clearState) {
  scheduler_cron_t *p = schCronEntry;
  while (p != NULL) {
    if (p->taskId == taskId) {
      if (enable == TOGGLE) {
        p->enable = !p->enable;
      } else {
        p->enable = enable;
      }
      if (clearState) {
        p->ptr = 0;
        p->idleUntil = 0;
      }
      break;
    }
    p = p->next;
  }
}

/**
 * @brief 删除一个协程
 * @param  taskId           目标任务ID
 */
void Sch_DelCoron(uint16_t taskId) {
  scheduler_cron_t *p = schCronEntry;
  scheduler_cron_t *q = NULL;
  void **temp = NULL;
  while (p != NULL) {
    if (p->taskId == taskId) {
      if (q == NULL) {
        temp = (void **)&(p->next);
        m_free(schCronEntry);
        if (*temp != NULL) m_replace(*temp, (schCronEntry));
      } else {
        temp = (void **)&(p->next);
        m_free(q->next);
        if (*temp != NULL) m_replace(*temp, (q->next));
      }
      break;
    }
    q = p;
    p = p->next;
  }
}

/**
 * @brief 获取调度器内协程数量
 */
int Sch_GetCoronNum(void) {
  scheduler_cron_t *p = schCronEntry;
  int num = 0;
  while (p != NULL) {
    num++;
    p = p->next;
  }
  return num;
}

/**
 * @brief 查询指定函数对应的协程ID
 * @param  task             目标任务函数指针
 * @retval uint16_t         任务ID (0xFFFF:未找到)
 */
uint16_t Sch_GetCoronId(void (*task)(void)) {
  scheduler_cron_t *p = schCronEntry;
  while (p != NULL) {
    if (p->task == task) {
      return p->taskId;
    }
    p = p->next;
  }
  return 0xffff;
}
#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_CALLLATER
/**
 * @brief 在指定时间后执行目标函数
 * @param  task             任务函数指针
 * @param  delay            延时启动时间(us)
 */
void Sch_CallLater(void (*task)(void), m_time_t delay) {
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
  if (p == NULL) return;
  p->task = task;
  p->runTimeUs = delay + m_time_us();
  p->next = NULL;
}

/**
 * @brief 删除一个已经添加的延时调用任务
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
      } else {
        temp = (void **)&(p->next);
        m_free(q->next);
        if (*temp != NULL) m_replace(*temp, (q->next));
      }
      break;
    }
    q = p;
    p = p->next;
  }
}
#endif  // _SCH_ENABLE_CALLLATER

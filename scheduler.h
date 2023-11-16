/**
 * @file scheduler.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2021-12-11
 *
 * THINK DIFFERENTLY
 */

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "modules.h"

extern void Scheduler_Run(const uint8_t block);

#if !_SCH_DEBUG_MODE
/**
 * @brief 创建一个调度任务
 * @param  task             任务函数指针
 * @param  freqHz           任务调度频率
 * @param  enable           初始化时是否使能
 * @param  priority         任务优先级
 * @retval uint16_t          任务ID (0xFFFF表示添加失败)
 */
extern uint16_t Sch_CreateTask(void (*task)(void), float freqHz, uint8_t enable,
                               uint8_t priority);
#else  // 调试模式开启时存储任务名
extern uint16_t _Sch_CreateTask(const char *name, void (*task)(void),
                                float freqHz, uint8_t enable, uint8_t priority);
#define Sch_CreateTask(task, freqHz, enable, priority) \
  _Sch_CreateTask(#task, task, freqHz, enable, priority)
#endif  // !_SCH_DEBUG_MODE

/**
 * @brief 切换任务使能状态
 * @param  taskId           目标任务ID
 * @param  enable           使能状态(0xff:切换)
 */
extern void Sch_SetTaskState(uint16_t taskId, uint8_t enable);

/**
 * @brief 删除一个调度任务
 * @param  taskId           目标任务ID
 */
extern void Sch_DeleteTask(uint16_t taskId);

/**
 * @brief 设置任务调度频率
 * @param  taskId           目标任务ID
 * @param  freq             调度频率
 */
extern void Sch_SetTaskFreq(uint16_t taskId, float freqHz);

/**
 * @brief 设置任务优先级
 * @param  taskId           目标任务ID
 * @param  priority         任务优先级
 */
extern void Sch_SetTaskPriority(uint16_t taskId, uint8_t priority);

/**
 * @brief 通过任务ID查询任务是否存在
 * @param  taskId           目标任务ID
 * @retval bool             任务是否存在
 */
extern bool Sch_IsTaskExist(uint16_t taskId);

/**
 * @brief 延迟(推后)指定任务下一次调度的时间
 * @param  taskId          目标任务ID
 * @param  delayUs         延迟时间(us)
 * @param  fromNow         是否从当前时间开始计算延迟
 */
extern void Sch_DelayTask(uint16_t taskId, m_time_t delayUs, uint8_t fromNow);

/**
 * @brief 获取调度器内任务数量
 */
extern uint16_t Sch_GetTaskNum(void);

/**
 * @brief 查询指定函数对应的任务ID
 * @param  task             目标任务函数指针
 * @retval uint16_t         任务ID (0xFFFF:未找到)
 */
extern uint16_t Sch_GetTaskId(void (*task)(void));

#if _SCH_ENABLE_COROUTINE
enum CR_MODES {         // 协程模式
  CR_MODE_ONESHOT = 0,  // 单次执行
  CR_MODE_AUTODEL,      // 执行完毕后自动删除
  CR_MODE_LOOP          // 循环执行
};

#pragma pack(1)
typedef struct {   // 协程任务结构
  m_time_t delay;  // 设定延时(us)
  long ptr;        // 协程跳入地址
} _cron_handle_t;
#pragma pack()
extern _cron_handle_t _cron_hp;

/**
 * @brief 初始化协程, 在协程函数开头调用
 */
#define CR_BEGIN()      \
  crap:;                \
  void *pcrap = &&crap; \
  if ((_cron_hp.ptr) != 0) goto *(void *)(_cron_hp.ptr)

/**
 * @brief 释放CPU, 让出时间片, 下次调度时从此处继续执行
 */
#define CR_YIELD()              \
  do {                          \
    __label__ l;                \
    (_cron_hp.ptr) = (long)&&l; \
    return;                     \
  l:;                           \
    (_cron_hp.ptr) = 0;         \
  } while (0)

/**
 * @brief 无阻塞等待直到条件满足
 */
#define CR_YIELD_UNTIL(cond)    \
  do {                          \
    __label__ l;                \
    (_cron_hp.ptr) = (long)&&l; \
  l:;                           \
    if (!(cond)) return;        \
    (_cron_hp.ptr) = 0;         \
  } while (0)

/**
 * @brief 无阻塞延时, 单位us
 */
#define CR_DELAY_US(us)                    \
  do {                                     \
    _cron_hp.delay = get_system_us() + us; \
    CR_YIELD();                            \
  } while (0)

/**
 * @brief 无阻塞延时, 单位ms
 */
#define CR_DELAY(ms) CR_DELAY_US(ms * 1000)

/**
 * @brief 创建一个协程
 * @param  task             任务函数指针
 * @param  enable           是否立即启动
 * @param  mode             模式(CR_MODE_xxx)
 * @retval uint16_t         任务ID (0xFFFF表示堆内存分配失败)
 */
extern uint16_t Sch_CreateCoron(void (*task)(void), uint8_t enable,
                                enum CR_MODES mode);

/**
 * @brief 设置协程使能状态
 * @param  taskId           目标任务ID
 * @param  enable           使能状态(0xff: 切换)
 * @param  clearState      是否清除协程状态(从头开始执行)
 */
extern void Sch_SetCoronState(uint16_t taskId, uint8_t enable,
                              uint8_t clearState);

/**
 * @brief 删除一个协程
 * @param  taskId           目标任务ID
 */
extern void Sch_DeleteCoron(uint16_t taskId);

/**
 * @brief 获取调度器内协程数量
 */
extern uint16_t Sch_GetCoronNum(void);

/**
 * @brief 查询指定函数对应的协程ID
 * @param  task             目标任务函数指针
 * @retval uint16_t         任务ID (0xFFFF:未找到)
 */
extern uint16_t Sch_GetCoronId(void (*task)(void));

/**
 * @brief 查询指定任务ID对应的协程是否存在
 * @param  taskId           目标任务ID
 * @retval bool             协程是否存在
 */
extern bool Sch_IsCoronExist(uint16_t taskId);

/**
 * @brief 运行一次协程, 执行完毕后自动删除
 */
#define Sch_RunCorontine(task) Sch_CreateCoron(task, 1, CR_MODE_AUTODEL)

#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_CALLLATER
/**
 * @brief 在指定时间后执行目标函数
 * @param  task             任务函数指针
 * @param  delayUs          延时启动时间(us)
 * @retval bool             是否成功
 */
extern bool Sch_CallLater(void (*task)(void), m_time_t delayUs);

/**
 * @brief 取消所有对应函数的延时调用任务
 * @param task              任务函数指针
 */
extern void Sch_CancelCallLater(void (*task)(void));
#endif  // _SCH_ENABLE_CALLLATER

#if _SCH_ENABLE_SOFTINT

/**
 * @brief 触发软中断
 * @param  mainChannel     主通道(0~7)
 * @param  subChannel     子通道(0~7)
 */
extern void Sch_TriggerSoftInt(uint8_t mainChannel, uint8_t subChannel);

/**
 * @brief 软中断处理函数
 * @param  subMask        子通道掩码(1 << subChannel)
 * @note  由调度器自动调用, 由用户实现
 */
extern void Scheduler_SoftIntHandler_Ch0(uint8_t subMask),
    Scheduler_SoftIntHandler_Ch1(uint8_t subMask),
    Scheduler_SoftIntHandler_Ch2(uint8_t subMask),
    Scheduler_SoftIntHandler_Ch3(uint8_t subMask),
    Scheduler_SoftIntHandler_Ch4(uint8_t subMask),
    Scheduler_SoftIntHandler_Ch5(uint8_t subMask),
    Scheduler_SoftIntHandler_Ch6(uint8_t subMask),
    Scheduler_SoftIntHandler_Ch7(uint8_t subMask);

#endif  // _SCH_ENABLE_SOFTINT

#endif  // _SCHEDULER_H_

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
#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

typedef void (*sch_func_t)(void *args);  // 任务函数指针类型

/**
 * @brief 时分调度器主函数
 * @param  block            是否阻塞
 * @param  sleep_us         block每轮休眠时间, 用于RTOS
 **/
extern void Scheduler_Run(const uint8_t block, const m_time_t sleep_us);

/**
 * @brief 创建一个调度任务
 * @param  name             任务名
 * @param  func             任务函数指针
 * @param  freqHz           任务调度频率
 * @param  enable           初始化时是否使能
 * @param  priority         任务优先级
 * @param  args             任务参数
 * @retval bool             是否成功
 */
extern bool Sch_CreateTask(const char *name, sch_func_t func, float freqHz,
                           uint8_t enable, uint8_t priority, void *args);

/**
 * @brief 切换任务使能状态
 * @param  name             任务名
 * @param  enable           使能状态(0xff:切换)
 * @retval bool             是否成功
 */
extern bool Sch_SetTaskState(const char *name, uint8_t enable);

/**
 * @brief 删除一个调度任务
 * @param  name             任务名
 * @retval bool             是否成功
 */
extern bool Sch_DeleteTask(const char *name);

/**
 * @brief 设置任务调度频率
 * @param  name             任务名
 * @param  freq             调度频率
 * @retval bool             是否成功
 */
extern bool Sch_SetTaskFreq(const char *name, float freqHz);

/**
 * @brief 设置任务优先级
 * @param  name             任务名
 * @param  priority         任务优先级
 * @retval bool             是否成功
 */
extern bool Sch_SetTaskPriority(const char *name, uint8_t priority);

/**
 * @brief 查询任务是否存在
 * @param  name             任务名
 * @retval bool             任务是否存在
 */
extern bool Sch_IsTaskExist(const char *name);

/**
 * @brief 延迟(推后)指定任务下一次调度的时间
 * @param  name             任务名
 * @param  delayUs          延迟时间(us)
 * @param  fromNow          从当前时间/上一次调度时间计算延迟
 * @retval bool             是否成功
 */
extern bool Sch_DelayTask(const char *name, m_time_t delayUs, uint8_t fromNow);

/**
 * @brief 获取调度器内任务数量
 */
extern uint16_t Sch_GetTaskNum(void);

#if _SCH_ENABLE_EVENT

/**
 * @brief 创建一个事件
 * @param  name             事件名
 * @param  callback         事件回调函数指针
 * @param  enable           初始化时是否使能
 * @retval bool             是否成功
 */
extern bool Sch_CreateEvent(const char *name, sch_func_t callback,
                            uint8_t enable);

/**
 * @brief 删除一个事件
 * @param  name             事件名
 * @retval bool             是否成功
 */
extern bool Sch_DeleteEvent(const char *name);

/**
 * @brief 设置事件使能状态
 * @param  name             事件名
 * @param  enable           使能状态(0xff: 切换)
 * @retval bool             是否成功
 */
extern bool Sch_SetEventState(const char *name, uint8_t enable);

/**
 * @brief 触发一个事件
 * @param  name             事件名
 * @param  args             传递参数
 * @retval bool             是否成功(事件不存在或禁用)
 */
extern bool Sch_TriggerEvent(const char *name, void *args);

/**
 * @brief 查询指定事件是否存在
 * @param  name             事件名
 * @retval bool             事件是否存在
 */
extern bool Sch_CheckEvent(const char *name);
#endif  // _SCH_ENABLE_EVENT

#if _SCH_ENABLE_COROUTINE
enum CR_MODES {         // 协程模式
  CR_MODE_ONESHOT = 0,  // 单次执行
  CR_MODE_LOOP,         // 循环执行
  CR_MODE_AUTODEL,      // 执行完毕后自动删除
};

#pragma pack(1)
typedef struct {                // 协程任务结构
  m_time_t yieldUntil;          // 等待态结束时间(us)
  long ptr[_SCH_CR_MAX_DEPTH];  // 协程跳入地址
  uint8_t depth;                // 协程当前深度
} _cron_handle_t;
#pragma pack()
extern _cron_handle_t *_cron_hp;

/**
 * @brief 初始化协程, 在协程函数开头调用
 */
#define CR_BEGIN()                                          \
  do {                                                      \
  crap:;                                                    \
    void *pcrap = &&crap;                                   \
    if (_cron_hp) {                                         \
      _cron_hp->depth++;                                    \
      if (_cron_hp->depth >= _SCH_CR_MAX_DEPTH) CR_END();   \
      if ((_cron_hp->ptr[_cron_hp->depth - 1]) != 0)        \
        goto *(void *)(_cron_hp->ptr[_cron_hp->depth - 1]); \
    }                                                       \
  } while (0)

/**
 * @brief 释放CPU, 让出时间片, 下次调度时从此处继续执行
 */
#define CR_YIELD()                                      \
  do {                                                  \
    __label__ l;                                        \
    if (_cron_hp) {                                     \
      (_cron_hp->ptr[_cron_hp->depth - 1]) = (long)&&l; \
      _cron_hp->depth--;                                \
      return;                                           \
    }                                                   \
  l:;                                                   \
    if (_cron_hp) {                                     \
      (_cron_hp->ptr[_cron_hp->depth - 1]) = 0;         \
    }                                                   \
  } while (0)

/**
 * @brief 执行其他协程函数，并处理跳转
 */
#define CR_CALL(func)                                   \
  do {                                                  \
    __label__ l;                                        \
    if (_cron_hp) {                                     \
      (_cron_hp->ptr[_cron_hp->depth - 1]) = (long)&&l; \
    }                                                   \
  l:;                                                   \
    func;                                               \
    if (_cron_hp) {                                     \
      if (_cron_hp->ptr[_cron_hp->depth]) {             \
        _cron_hp->depth--;                              \
        return;                                         \
      } else {                                          \
        (_cron_hp->ptr[_cron_hp->depth - 1]) = 0;       \
      }                                                 \
    }                                                   \
  } while (0)

/**
 * @brief 退出协程且不再重入, 在协程函数结尾调用
 */
#define CR_END()                              \
  do {                                        \
    if (_cron_hp) {                           \
      _cron_hp->ptr[_cron_hp->depth - 1] = 0; \
      _cron_hp->depth--;                      \
    }                                         \
    return;                                   \
  } while (0)

/**
 * @brief 无阻塞等待直到条件满足
 */
#define CR_YIELD_UNTIL(cond) \
  do {                       \
    CR_YIELD();              \
  } while (!(cond))

/**
 * @brief 无阻塞延时, 单位us
 */
#define CR_DELAY_US(us)                          \
  do {                                           \
    _cron_hp->yieldUntil = get_system_us() + us; \
    CR_YIELD();                                  \
  } while (0)

/**
 * @brief 无阻塞延时, 单位ms
 */
#define CR_DELAY(ms) CR_DELAY_US(ms * 1000)

/**
 * @brief 创建一个协程
 * @param  name             协程名
 * @param  func             任务函数指针
 * @param  enable           是否立即启动
 * @param  mode             模式(CR_MODE_xxx)
 * @param  args             任务参数
 * @retval bool             是否成功
 */
extern bool Sch_CreateCoron(const char *name, sch_func_t func,
                            uint8_t enable, enum CR_MODES mode, void *args);

/**
 * @brief 设置协程使能状态
 * @param  name            协程名
 * @param  enable          使能状态(0xff: 切换)
 * @param  clearState      是否清除协程状态(从头开始执行)
 * @retval bool            是否成功
 */
extern bool Sch_SetCoronState(const char *name, uint8_t enable,
                              uint8_t clearState);

/**
 * @brief 删除一个协程
 * @param  name            协程名
 * @retval bool            是否成功
 */
extern bool Sch_DeleteCoron(const char *name);

/**
 * @brief 获取调度器内协程数量
 */
extern uint16_t Sch_GetCoronNum(void);

/**
 * @brief 查询指定协程的协程是否存在
 * @param  name             协程名
 * @retval bool             协程是否存在
 */
extern bool Sch_IsCoronExist(const char *name);

/**
 * @brief 运行一次协程, 执行完毕后自动删除
 */
#define Sch_RunCorontine(func, args) \
  Sch_CreateCoron(#func, func, 1, CR_MODE_AUTODEL, args)

#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_CALLLATER
/**
 * @brief 在指定时间后执行目标函数
 * @param  func             任务函数指针
 * @param  delayUs          延时启动时间(us)
 * @param  args             任务参数
 * @retval bool             是否成功
 */
extern bool Sch_CallLater(sch_func_t func, m_time_t delayUs, void *args);

/**
 * @brief 取消所有对应函数的延时调用任务
 * @param func              任务函数指针
 */
extern void Sch_CancelCallLater(sch_func_t func);
#endif  // _SCH_ENABLE_CALLLATER

#if _SCH_ENABLE_SOFTINT

/**
 * @brief 触发软中断
 * @param  mainChannel     主通道(0-7)
 * @param  subChannel      子通道(0-7)
 */
extern void Sch_TriggerSoftInt(uint8_t mainChannel, uint8_t subChannel);

/**
 * @brief 软中断回调函数
 * @param  subMask        8个子通道掩码(1 << subChannel)
 * @note  调度器自动调用, 由用户实现
 */
extern void Scheduler_SoftInt_Handler_Ch0(uint8_t subMask),
    Scheduler_SoftInt_Handler_Ch1(uint8_t subMask),
    Scheduler_SoftInt_Handler_Ch2(uint8_t subMask),
    Scheduler_SoftInt_Handler_Ch3(uint8_t subMask),
    Scheduler_SoftInt_Handler_Ch4(uint8_t subMask),
    Scheduler_SoftInt_Handler_Ch5(uint8_t subMask),
    Scheduler_SoftInt_Handler_Ch6(uint8_t subMask),
    Scheduler_SoftInt_Handler_Ch7(uint8_t subMask);

#endif  // _SCH_ENABLE_SOFTINT

#if _SCH_ENABLE_TERMINAL
#include "embedded_cli.h"
/**
 * @brief 添加调度器相关的终端命令(task/event/coron/softint)
 */
extern void Sch_AddCmdToCli(EmbeddedCli *cli);
#endif  // _SCH_ENABLE_TERMINAL

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_H_

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
 * @brief 调度器主函数
 * @param  block            是否阻塞, 若不阻塞则应将此函数放在SuperLoop中
 * @retval m_time_t         返回时间: 距离下一次调度的时间(us)
 * @note block=0时. SuperLoop应保证在返回时间前交还CPU以最小化调度延迟
 * @note block=1时. 查看Scheduler_Idle_Callback函数说明
 **/
extern m_time_t Scheduler_Run(const bool block);

/**
 * @brief 调度器空闲回调函数, 由用户实现(block=1时调用)
 * @param  idleTimeUs      空闲时间(us)
 * @note  应保证在空闲时间前交还CPU以最小化调度延迟
 * @note  可在此函数内实现低功耗
 */
extern void Scheduler_Idle_Callback(m_time_t idleTimeUs);

#if _SCH_ENABLE_TASK

/**
 * @brief 创建一个调度任务
 * @param  name             任务名
 * @param  func             任务函数指针
 * @param  freqHz           任务调度频率
 * @param  enable           初始化时是否使能
 * @param  priority         任务优先级(越大优先级越高)
 * @param  args             任务参数
 * @retval bool             是否成功
 */
extern bool Sch_CreateTask(const char *name, sch_func_t func, float freqHz,
                           bool enable, uint8_t priority, void *args);

/**
 * @brief 切换任务使能状态
 * @param  name             任务名
 * @param  enable           使能状态(0xff:切换)
 * @retval bool             是否成功
 */
extern bool Sch_SetTaskState(const char *name, bool enable);

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
 * @brief 查询任务状态
 * @param  name             任务名
 * @retval bool             任务状态
 */
extern bool Sch_GetTaskState(const char *name);

/**
 * @brief 延迟(推后)指定任务下一次调度的时间
 * @param  name             任务名
 * @param  delayUs          延迟时间(us)
 * @param  fromNow          从当前时间/上一次调度时间计算延迟
 * @retval bool             是否成功
 */
extern bool Sch_DelayTask(const char *name, m_time_t delayUs, bool fromNow);

/**
 * @brief 获取调度器内任务数量
 */
extern uint16_t Sch_GetTaskNum(void);

#endif  // _SCH_ENABLE_TASK

#if _SCH_ENABLE_EVENT

/**
 * @brief 创建一个事件
 * @param  name             事件名
 * @param  callback         事件回调函数指针
 * @param  enable           初始化时是否使能
 * @retval bool             是否成功huidi
 * @warning 事件回调是异步执行的, 由调度器自动调用
 */
extern bool Sch_CreateEvent(const char *name, sch_func_t callback, bool enable);

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
extern bool Sch_SetEventState(const char *name, bool enable);

/**
 * @brief 查询指定事件使能状态
 * @param  name             事件名
 * @retval bool             事件使能状态
 */
extern bool Sch_GetEventState(const char *name);

/**
 * @brief 触发一个事件
 * @param  name             事件名
 * @param  args             传递参数
 * @retval bool             是否成功(事件不存在或禁用)
 * @warning 事件回调是异步执行的, 需注意回调参数的生命周期
 * @note 对于短生命周期的参数, 可以考虑使用Sch_TriggerEventEx
 */
extern bool Sch_TriggerEvent(const char *name, void *args);

/**
 * @brief 触发一个事件, 并为参数创建副本内存(回调执行后自动释放)
 * @param  name             事件名
 * @param  arg_ptr          参数指针
 * @param  arg_size         参数大小
 * @retval bool             是否成功(事件不存在或禁用, 或alloc失败)
 */
extern bool Sch_TriggerEventEx(const char *name, const void *arg_ptr,
                               uint16_t arg_size);

/**
 * @brief 获取调度器内事件数量
 */
extern uint16_t Sch_GetEventNum(void);

/**
 * @brief 查询指定事件是否存在
 * @param  name             事件名
 * @retval bool             事件是否存在
 */
extern bool Sch_IsEventExist(const char *name);
#endif  // _SCH_ENABLE_EVENT

#if _SCH_ENABLE_COROUTINE
#include "ulist.h"

typedef enum {          // 协程模式
  CR_MODE_ONESHOT = 0,  // 单次执行
  CR_MODE_LOOP,         // 循环执行
  CR_MODE_AUTODEL,      // 执行完毕后自动删除
} CR_MODE;

#pragma pack(1)
typedef struct {  // 协程句柄结构
  long ptr;       // 协程跳入地址
  void *local;    // 协程局部变量储存区
} __cortn_data_t;

typedef struct {         // 协程句柄结构
  ulist_t dataList;      // 协程数据列表
  __cortn_data_t *data;  // 协程数据指针
  uint8_t depth;         // 协程当前嵌套深度
  uint8_t maxDepth;      // 协程最大嵌套深度
  m_time_t yieldUntil;   // 等待态结束时间(us)
  void *msg;             // 协程消息指针
} __cortn_handle_t;
#pragma pack()
extern __cortn_handle_t *__chd;

// 内部函数
extern void *__Sch_CrInitLocal(uint16_t size);
extern void __Sch_CrFreeLocal(void);
extern bool __Sch_CrAwaitEnter(void);
extern bool __Sch_CrAwaitReturn(void);

/**
 * @brief 初始化协程局部变量, 在协程函数最开头调用(以CR_LOCAL_END结束)
 * @warning 不允许在初始化时赋初值, 全部填充0
 * @note  以l->xxx的形式访问局部变量
 */
#define CR_LOCAL_START struct _cr_local {
/**
 * @brief 结束协程局部变量初始化(CR_LOCAL后, CR_BEGIN前调用)
 */
#define CR_LOCAL_END                                \
  }                                                 \
  *l = __Sch_CrInitLocal(sizeof(struct _cr_local)); \
  if (l == NULL) return;

/**
 * @brief 初始化协程, 在协程函数开头调用(CR_LOCAL_END后调用)
 */
#define CR_BEGIN()                                   \
  do {                                               \
  crap:;                                             \
    void *pcrap = &&crap;                            \
    if ((__chd->data[__chd->depth].ptr) != 0)        \
      goto *(void *)(__chd->data[__chd->depth].ptr); \
  } while (0)

/**
 * @brief 释放CPU, 让出时间片, 下次调度时从此处继续执行
 */
#define CR_YIELD()                               \
  do {                                           \
    __label__ l;                                 \
    (__chd->data[__chd->depth].ptr) = (long)&&l; \
    return;                                      \
  l:;                                            \
    (__chd->data[__chd->depth].ptr) = 0;         \
  } while (0)

/**
 * @brief 执行其他协程函数，并处理跳转
 */
#define CR_AWAIT(func)                           \
  do {                                           \
    __label__ l;                                 \
    (__chd->data[__chd->depth].ptr) = (long)&&l; \
  l:;                                            \
    if (__Sch_CrAwaitEnter()) {                  \
      func;                                      \
      if (!__Sch_CrAwaitReturn()) return;        \
      (__chd->data[__chd->depth].ptr) = 0;       \
    }                                            \
  } while (0)

/**
 * @brief 退出协程, 在协程函数结尾调用
 */
#define CR_END()                       \
  do {                                 \
    __Sch_CrFreeLocal();               \
    __chd->data[__chd->depth].ptr = 0; \
    return;                            \
  } while (0)

/**
 * @brief 等待消息并唤醒
 */
#define CR_WAIT_FOR_MSG()  \
  do {                     \
    __chd->yieldUntil = 0; \
    CR_YIELD();            \
  } while (0)

/**
 * @brief 获取信箱指针并清空信箱
 */
static inline void *CR_GET_MSG(void) {
  void *msg = __chd->msg;
  __chd->msg = NULL;
  return msg;
}

/**
 * @brief 查询是否有消息
 */
#define CR_HAS_MSG() (__chd->msg != NULL)

/**
 * @brief 无阻塞延时, 单位us
 */
#define CR_DELAY_US(us)                       \
  do {                                        \
    __chd->yieldUntil = get_system_us() + us; \
    CR_YIELD();                               \
  } while (0)

/**
 * @brief 无阻塞延时, 单位ms
 */
#define CR_DELAY(ms) CR_DELAY_US(ms * 1000)

/**
 * @brief 无阻塞等待直到条件满足
 * @note  占用较大, 考虑使用CR_DELAY_UNTIL
 */
#define CR_YIELD_UNTIL(cond) \
  do {                       \
    CR_YIELD();              \
  } while (!(cond))

/**
 * @brief 无阻塞等待直到条件满足
 */
#define CR_DELAY_UNTIL(cond, delayUs) \
  do {                                \
    CR_DELAY_US(delayUs);             \
  } while (!(cond))

/**
 * @brief 运行一次协程, 执行完毕后自动删除
 */
#define CR_RUN(name, func, args) \
  Sch_CreateCortn(name, func, 1, CR_MODE_AUTODEL, args)

/**
 * @brief 等待直到指定协程完成
 */
#define CR_JOIN(name) CR_DELAY_UNTIL(!Sch_IsCortnExist(name), 1000)

/**
 * @brief 创建一个协程
 * @param  name             协程名
 * @param  func             任务函数指针
 * @param  enable           是否立即启动
 * @param  mode             模式(CR_MODE_xxx)
 * @param  args             任务参数
 * @retval bool             是否成功
 */
extern bool Sch_CreateCortn(const char *name, sch_func_t func, bool enable,
                            CR_MODE mode, void *args);

/**
 * @brief 设置协程使能状态
 * @param  name            协程名
 * @param  enable          使能状态(0xff: 切换)
 * @param  clearState      是否清除协程状态(从头开始执行)
 * @retval bool            是否成功
 */
extern bool Sch_SetCortnState(const char *name, bool enable, bool clearState);

/**
 * @brief 查询指定协程是否正在运行
 * @param  name             协程名
 * @retval bool             是否正在运行
 */
extern bool Sch_GetCortnState(const char *name);

/**
 * @brief 删除一个协程
 * @param  name            协程名
 * @retval bool            是否成功
 */
extern bool Sch_DeleteCortn(const char *name);

/**
 * @brief 获取调度器内协程数量
 */
extern uint16_t Sch_GetCortnNum(void);

/**
 * @brief 查询指定协程是否存在
 * @param  name             协程名
 * @retval bool             协程是否存在
 */
extern bool Sch_IsCortnExist(const char *name);

/**
 * @brief 查询指定协程是否处于等待消息状态
 * @param  name             协程名
 * @retval bool             协程是否处于等待消息状态
 */
extern bool Sch_IsCortnWaitForMsg(const char *name);

/**
 * @brief 发送消息给指定协程并唤醒
 * @param  name             协程名
 * @param  msg              消息指针
 * @retval bool             是否成功
 */
extern bool Sch_SendMsgToCortn(const char *name, void *msg);

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
 * @param  mainChannel    主通道(0-7)
 * @param  subMask        8个子通道掩码(1 << subChannel)
 * @note  调度器自动调用, 由用户实现
 */
extern void Scheduler_SoftInt_Handler(uint8_t mainChannel, uint8_t subMask);

#endif  // _SCH_ENABLE_SOFTINT

#if _SCH_ENABLE_TERMINAL
#include "embedded_cli.h"
/**
 * @brief 添加调度器相关的终端命令(task/event/cortn/softint)
 */
extern void Sch_AddCmdToCli(EmbeddedCli *cli);
#endif  // _SCH_ENABLE_TERMINAL

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_H_

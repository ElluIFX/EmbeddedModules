#ifndef _SCHEDULER_COROUTINE_H
#define _SCHEDULER_COROUTINE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"

#if _SCH_ENABLE_COROUTINE

#include "ulist.h"

#define CR_STATE_READY 0     // 就绪态
#define CR_STATE_RUNNING 1   // 运行态
#define CR_STATE_AWAITING 2  // 等待态
#define CR_STATE_SLEEPING 3  // 睡眠态
#define CR_STATE_STOPPED 4   // 停止态

#pragma pack(1)
typedef struct {  // 协程句柄结构
  long ptr;       // 协程跳入地址
  void *local;    // 协程局部变量储存区
} __cortn_data_t;

typedef struct {         // 协程句柄结构
  uint8_t state;         // 协程状态
  ulist_t dataList;      // 协程数据列表
  __cortn_data_t *data;  // 协程数据指针
  uint8_t depth;         // 协程当前深度
  uint8_t callDepth;     // 协程嵌套深度
  uint64_t sleepUntil;   // 等待态结束时间(us), 0表示暂停
  void *msg;             // 协程消息指针
  const char *name;      // 协程名
} __cortn_handle_t;
#pragma pack()

#define __chd__ You_forgot___async__
#define __async_check__ You_forgot_ASYNC_INIT

#define __async__ __cortn_handle_t *__chd__
typedef void (*cortn_func_t)(__async__, void *args);  // 协程函数指针类型

extern const char *__Internal_GetName(void);
extern void *__Internal_InitLocal(size_t size);
extern uint8_t __Internal_AwaitEnter(void);
extern uint8_t __Internal_AwaitReturn(void);
extern void __Internal_Delay(uint64_t delayUs);
extern uint8_t __Internal_AcquireMutex(const char *name);
extern void __Internal_ReleaseMutex(const char *name);
extern uint8_t __Internal_WaitBarrier(const char *name);
static void __Internal_AwaitMsg(__async__, void **msgPtr);

#define ASYNC_INIT                                       \
  __crap:;                                               \
  void *__async_check__ = &&__crap;                      \
  do {                                                   \
    if ((__chd__->data[__chd__->depth].ptr) != 0)        \
      goto *(void *)(__chd__->data[__chd__->depth].ptr); \
  } while (0);

/**
 * @brief 定义并初始化协程, 在协程函数开头调用
 * @note 如果需要使用局部变量, 使用ASYNC_LOCAL_START代替本宏
 */
#define ASYNC_NOLOCAL ASYNC_INIT

/**
 * @brief 初始化协程局部变量区, 在协程函数最开头调用(以ASYNC_LOCAL_END结束)
 * @warning 不允许在初始化时赋初值, 全部填充0
 * @note  以LOCAL(xxx)的形式访问局部变量
 */
#define ASYNC_LOCAL_START struct _cr_local {
/**
 * @brief 结束协程局部变量区并初始化协程
 */
#define ASYNC_LOCAL_END                                          \
  }                                                              \
  *_cr_local_p = __Internal_InitLocal(sizeof(struct _cr_local)); \
  if (_cr_local_p == NULL) return;                               \
  ASYNC_INIT

/**
 * @brief 局部变量
 */
#define LOCAL(var) (_cr_local_p->var)

/**
 * @brief 释放CPU, 下次调度时从此处继续执行
 */
#define YIELD()                                      \
  do {                                               \
    __label__ l;                                     \
    (__chd__->data[__chd__->depth].ptr) = (long)&&l; \
    return;                                          \
  l:;                                                \
    (__chd__->data[__chd__->depth].ptr) = 0;         \
    __async_check__ = __async_check__;               \
  } while (0)

/**
 * @brief 执行其他协程函数，并阻塞直至协程函数返回
 * @note 允许任意参数传递 如 AWAIT(Abc, 1, 2, 3);
 * @warning 所执行函数不应通过返回值返回变量, 应使用指针传递
 */
#define AWAIT(func_cmd, args...)                     \
  do {                                               \
    __label__ l;                                     \
    (__chd__->data[__chd__->depth].ptr) = (long)&&l; \
  l:;                                                \
    if (__Internal_AwaitEnter()) {                   \
      func_cmd(__chd__, ##args);                     \
      if (!__Internal_AwaitReturn()) return;         \
      (__chd__->data[__chd__->depth].ptr) = 0;       \
    }                                                \
    __async_check__ = __async_check__;               \
  } while (0)

/**
 * @brief 获取当前协程名
 */
#define ASYNC_SELF_NAME() __Internal_GetName()

/**
 * @brief 无阻塞延时, 单位us
 */
#define AWAIT_DELAY_US(us)              \
  do {                                  \
    __Internal_Delay(us);               \
    __chd__->state = CR_STATE_SLEEPING; \
    YIELD();                            \
  } while (0)

/**
 * @brief 无阻塞延时, 单位ms
 */
#define AWAIT_DELAY(ms) AWAIT_DELAY_US(ms * 1000)

/**
 * @brief 无阻塞等待直到条件表达式为真
 * @note  占用较大, 考虑使用AWAIT_DELAY_UNTIL
 */
#define AWAIT_YIELD_UNTIL(cond) \
  do {                          \
    YIELD();                    \
  } while (!(cond))

/**
 * @brief 无阻塞等待直到条件表达式为真，每隔delayMs检查一次
 */
#define AWAIT_DELAY_UNTIL(cond, delayMs) \
  do {                                   \
    AWAIT_DELAY(delayMs);                \
  } while (!(cond))

/**
 * @brief 异步执行其他协程
 */
#define ASYNC_RUN(name, func, args) Sch_RunCortn(name, func, (void *)args)

/**
 * @brief 等待直到指定协程完成
 */
#define AWAIT_JOIN(name) AWAIT_DELAY_UNTIL(!Sch_IsCortnRunning(name), 1)

/**
 * @brief 等待消息并将消息指针赋值给指定变量
 */
#define AWAIT_RECV_MSG(to_ptr) AWAIT(__Internal_AwaitMsg, (void **)&(to_ptr))

/**
 * @brief 发送消息给指定协程, 立即返回
 */
#define ASYNC_SEND_MSG(name, msg) Sch_SendMsgToCortn((name), (void *)(msg));

/**
 * @brief 获取互斥锁, 阻塞直至获取成功
 */
#define AWAIT_ACQUIRE_MUTEX(mutex_name)         \
  do {                                          \
    if (!__Internal_AcquireMutex(mutex_name)) { \
      __chd__->state = CR_STATE_AWAITING;       \
      YIELD();                                  \
    }                                           \
  } while (0)

/**
 * @brief 释放互斥锁, 立即返回
 */
#define ASYNC_RELEASE_MUTEX(mutex_name) __Internal_ReleaseMutex(mutex_name)

/**
 * @brief 等待屏障, 阻塞直至屏障解除
 */
#define AWAIT_BARRIER(barr_name)              \
  do {                                        \
    if (!__Internal_WaitBarrier(barr_name)) { \
      __chd__->state = CR_STATE_AWAITING;     \
      YIELD();                                \
    }                                         \
  } while (0)

/**
 * @brief 手动释放屏障, 立即返回
 */
#define ASYNC_RELEASE_BARRIER(barr_name) Sch_CortnBarrierRelease(barr_name)

/**
 * @brief 运行一个协程
 * @param  name             协程名
 * @param  func             任务函数指针
 * @param  args             任务参数
 * @retval uint8_t          是否成功
 */
extern uint8_t Sch_RunCortn(const char *name, cortn_func_t func, void *args);

/**
 * @brief 停止一个协程
 * @param  name            协程名
 * @retval uint8_t         是否成功
 */
extern uint8_t Sch_StopCortn(const char *name);

/**
 * @brief 获取调度器内协程数量
 */
extern uint16_t Sch_GetCortnNum(void);

/**
 * @brief 查询指定协程是否正在运行
 * @param  name             协程名
 * @retval uint8_t             协程是否正在运行
 */
extern uint8_t Sch_IsCortnRunning(const char *name);

/**
 * @brief 查询指定协程是否处于等待消息状态
 * @param  name             协程名
 * @retval uint8_t             协程是否处于等待消息状态
 */
extern uint8_t Sch_IsCortnWaitingMsg(const char *name);

/**
 * @brief 发送消息给指定协程并唤醒
 * @param  name             协程名
 * @param  msg              消息指针
 * @retval uint8_t          是否成功
 */
extern uint8_t Sch_SendMsgToCortn(const char *name, void *msg);

/**
 * @brief 手动释放一个屏障
 * @param  name            屏障名
 * @retval uint8_t         是否成功
 */
extern uint8_t Sch_CortnBarrierRelease(const char *name);

/**
 * @brief 获取指定屏障等待协程数量
 * @param  name            屏障名
 * @retval uint16_t        等待数量
 */
extern uint16_t Sch_GetCortnBarrierWaitingNum(const char *name);

/**
 * @brief 设置指定屏障目标协程数量
 * @param  name            屏障名
 * @param  target          目标数量
 */
extern uint8_t Sch_SetCortnBarrierTarget(const char *name, uint16_t target);

#endif  // _SCH_ENABLE_COROUTINE

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_COROUTINE_H

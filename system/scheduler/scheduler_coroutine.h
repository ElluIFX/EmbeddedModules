#ifndef _SCHEDULER_COROUTINE_H
#define _SCHEDULER_COROUTINE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"

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
  uint64_t yieldUntil;   // 等待态结束时间(us), 0表示暂停
  void *msg;             // 协程消息指针
} __cortn_handle_t;
#pragma pack()
extern __cortn_handle_t *__chd;
extern const char *__cortn_name;

extern void *__Sch_CrInitLocal(uint16_t size);
extern uint8_t __Sch_CrAwaitEnter(void);
extern uint8_t __Sch_CrAwaitReturn(void);
extern void __Sch_CrDelay(uint64_t delayUs);
extern uint8_t __Sch_CrAcquireMutex(const char *name);
extern void __Sch_CrReleaseMutex(const char *name);
extern uint8_t __Sch_CrWaitBarrier(const char *name);

/**
 * @brief 定义并初始化协程, 在协程函数开头调用(CR_LOCAL_END后调用)
 * @note 如果需要使用局部变量, 使用ASYNC_LOCAL_START代替本宏
 */
#define ASYNC_NOLOCAL                                \
  do {                                               \
  crap:;                                             \
    void *pcrap = &&crap;                            \
    if ((__chd->data[__chd->depth].ptr) != 0)        \
      goto *(void *)(__chd->data[__chd->depth].ptr); \
  } while (0);

/**
 * @brief 初始化协程局部变量区, 在协程函数最开头调用(以ASYNC_LOCAL_END结束)
 * @warning 不允许在初始化时赋初值, 全部填充0
 * @note  以LOCAL(xxx)的形式访问局部变量
 */
#define ASYNC_LOCAL_START struct _cr_local {
/**
 * @brief 结束协程局部变量区并初始化协程(ASYNC_LOCAL_START后调用)
 */
#define ASYNC_LOCAL_END                                       \
  }                                                           \
  *_cr_local_p = __Sch_CrInitLocal(sizeof(struct _cr_local)); \
  if (_cr_local_p == NULL) return;                            \
  ASYNC_NOLOCAL

/**
 * @brief 局部变量
 */
#define LOCAL(var) (_cr_local_p->var)

/**
 * @brief 释放CPU, 让出时间片, 下次调度时从此处继续执行
 */
#define YIELD()                                  \
  do {                                           \
    __label__ l;                                 \
    (__chd->data[__chd->depth].ptr) = (long)&&l; \
    return;                                      \
  l:;                                            \
    (__chd->data[__chd->depth].ptr) = 0;         \
  } while (0)

/**
 * @brief 执行其他协程函数，并阻塞直至协程函数返回
 * @note 允许任意参数传递 如 AWAIT(Abc(1, 2, 3));
 * @warning 所执行函数不应通过返回值返回变量, 应使用指针传递
 */
#define AWAIT(func_cmd)                          \
  do {                                           \
    __label__ l;                                 \
    (__chd->data[__chd->depth].ptr) = (long)&&l; \
  l:;                                            \
    if (__Sch_CrAwaitEnter()) {                  \
      func_cmd;                                  \
      if (!__Sch_CrAwaitReturn()) return;        \
      (__chd->data[__chd->depth].ptr) = 0;       \
    }                                            \
  } while (0)

static void __GET_CR_MSG(void **msgPtr) {
  ASYNC_NOLOCAL
  if (__chd->msg == NULL) {
    __chd->yieldUntil = 0;
    YIELD();
  }
  if (msgPtr != NULL) *msgPtr = __chd->msg;
  __chd->msg = NULL;
}

/**
 * @brief 获取当前协程名
 */
#define ASYNC_SELF_NAME() (__cortn_name)

/**
 * @brief 等待消息并将消息指针赋值给指定变量
 */
#define AWAIT_RECV_MSG(to_ptr) AWAIT(__GET_CR_MSG((void **)&(to_ptr)))

/**
 * @brief 发送消息给指定协程, 立即返回
 */
#define ASYNC_SEND_MSG(name, msg) Sch_SendMsgToCortn((name), (void *)(msg));

/**
 * @brief 获取互斥锁, 阻塞直至获取成功
 */
#define AWAIT_ACQUIRE_MUTEX(mutex_name)      \
  do {                                       \
    if (!__Sch_CrAcquireMutex(mutex_name)) { \
      __chd->yieldUntil = 0;                 \
      YIELD();                               \
    }                                        \
  } while (0)

/**
 * @brief 释放互斥锁, 立即返回
 */
#define ASYNC_RELEASE_MUTEX(mutex_name) __Sch_CrReleaseMutex(mutex_name)

/**
 * @brief 等待屏障, 阻塞直至屏障解除
 */
#define AWAIT_BARRIER(barr_name)           \
  do {                                     \
    if (!__Sch_CrWaitBarrier(barr_name)) { \
      __chd->yieldUntil = 0;               \
      YIELD();                             \
    }                                      \
  } while (0)

/**
 * @brief 手动释放屏障, 立即返回
 */
#define ASYNC_RELEASE_BARRIER(barr_name) Sch_CortnBarrierRelease(barr_name)

/**
 * @brief 无阻塞延时, 单位us
 */
#define AWAIT_DELAY_US(us) \
  do {                     \
    __Sch_CrDelay(us);     \
    YIELD();               \
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
#define ASYNC_RUN(name, func, args) \
  Sch_CreateCortn(name, func, 1, CR_MODE_AUTODEL, (void *)args)

/**
 * @brief 等待直到指定协程完成
 */
#define AWAIT_JOIN(name) AWAIT_DELAY_UNTIL(!Sch_IsCortnExist(name), 100)

/**
 * @brief 创建一个协程
 * @param  name             协程名
 * @param  func             任务函数指针
 * @param  enable           是否立即启动
 * @param  mode             模式(CR_MODE_xxx)
 * @param  args             任务参数
 * @retval uint8_t          是否成功
 */
extern uint8_t Sch_CreateCortn(const char *name, sch_func_t func,
                               uint8_t enable, CR_MODE mode, void *args);

/**
 * @brief 设置协程使能状态
 * @param  name            协程名
 * @param  enable          使能状态(0xff: 切换)
 * @param  clearState      是否清除协程状态(从头开始执行)
 * @retval uint8_t         是否成功
 */
extern uint8_t Sch_SetCortnState(const char *name, uint8_t enable,
                                 uint8_t clearState);

/**
 * @brief 查询指定协程是否正在运行
 * @param  name             协程名
 * @retval uint8_t             是否正在运行
 */
extern uint8_t Sch_GetCortnState(const char *name);

/**
 * @brief 删除一个协程
 * @param  name            协程名
 * @retval uint8_t         是否成功
 */
extern uint8_t Sch_DeleteCortn(const char *name);

/**
 * @brief 获取调度器内协程数量
 */
extern uint16_t Sch_GetCortnNum(void);

/**
 * @brief 查询指定协程是否存在
 * @param  name             协程名
 * @retval uint8_t             协程是否存在
 */
extern uint8_t Sch_IsCortnExist(const char *name);

/**
 * @brief 查询指定协程是否处于等待消息状态
 * @param  name             协程名
 * @retval uint8_t             协程是否处于等待消息状态
 */
extern uint8_t Sch_IsCortnWaitForMsg(const char *name);

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

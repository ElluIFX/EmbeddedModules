#ifndef _SCHEDULER_COROUTINE_H
#define _SCHEDULER_COROUTINE_H
#ifdef __cplusplus
extern "C" {
#endif
#include "scheduler.h"
#if SCH_CFG_ENABLE_COROUTINE
#include "scheduler_coroutine_internal.h"

/**
 * @brief 初始化协程
 */
#define ASYNC_INIT __ASYNC_INIT

/**
 * @brief 定义并初始化协程, 在协程函数开头调用
 * @note 如果需要使用局部变量, 使用ASYNC_LOCAL_START代替本宏
 */
#define ASYNC_NOLOCAL __ASYNC_INIT

/**
 * @brief 初始化协程局部变量区, 在协程函数最开头调用(以ASYNC_LOCAL_END结束)
 * @warning 不允许在初始化时赋初值, 全部填充0
 * @note  以LOCAL(xxx)的形式访问局部变量
 */
#define ASYNC_LOCAL_START __ASYNC_LOCAL_START

/**
 * @brief 结束协程局部变量区并初始化协程
 */
#define ASYNC_LOCAL_END __ASYNC_LOCAL_END

/**
 * @brief 局部变量
 */
#define LOCAL(var) __LOCAL(var)

/**
 * @brief 释放CPU, 下次调度时从此处继续执行
 */
#define YIELD() __YIELD()

/**
 * @brief 执行其他协程函数，并阻塞直至协程函数返回
 * @note 允许任意参数传递 如 AWAIT(Abc, 1, 2, 3);
 * @warning 所执行函数不应通过返回值返回变量, 应使用指针传递
 */
#define AWAIT(func_cmd, args...) __AWAIT(func_cmd, ##args)

/**
 * @brief 获取当前协程名
 */
#define ASYNC_SELF_NAME() __cortn_internal_get_name()

/**
 * @brief 无阻塞延时, 单位us
 */
#define AWAIT_DELAY_US(us) __AWAIT_DELAY_US(us)

/**
 * @brief 无阻塞延时, 单位ms
 */
#define AWAIT_DELAY(ms) __AWAIT_DELAY_US(ms * 1000)

/**
 * @brief 无阻塞等待直到条件表达式为真
 * @note  占用较大, 考虑使用AWAIT_DELAY_UNTIL
 */
#define AWAIT_YIELD_UNTIL(cond) __AWAIT_YIELD_UNTIL(cond)

/**
 * @brief 无阻塞等待直到条件表达式为真，每隔delayMs检查一次
 */
#define AWAIT_DELAY_UNTIL(cond, delayMs) __AWAIT_DELAY_UNTIL(cond, delayMs)

/**
 * @brief 异步执行其他协程
 */
#define ASYNC_RUN(name, func, args) sch_cortn_run(name, func, (void *)args)

/**
 * @brief 等待直到指定协程完成
 */
#define AWAIT_JOIN(name) __AWAIT_DELAY_UNTIL(!sch_cortn_get_running(name), 1)

/**
 * @brief 等待消息并将消息指针赋值给指定变量
 */
#define AWAIT_RECV_MSG(to_ptr) __AWAIT(__Internal_AwaitMsg, (void **)&(to_ptr))

/**
 * @brief 发送消息给指定协程, 立即返回
 */
#define ASYNC_SEND_MSG(name, msg) sch_cortn_send_msg_to((name), (void *)(msg));

/**
 * @brief 获取互斥锁, 阻塞直至获取成功
 */
#define AWAIT_ACQUIRE_MUTEX(mutex_name) __AWAIT_ACQUIRE_MUTEX(mutex_name)

/**
 * @brief 释放互斥锁, 立即返回
 */
#define ASYNC_RELEASE_MUTEX(mutex_name) __cortn_internal_rel_mutex(mutex_name)

/**
 * @brief 等待屏障, 阻塞直至屏障解除
 */
#define AWAIT_BARRIER(barr_name) __AWAIT_BARRIER(barr_name)

/**
 * @brief 手动释放屏障, 立即返回
 */
#define ASYNC_RELEASE_BARRIER(barr_name) sch_cortn_release_barrier(barr_name)

/**
 * @brief 设置屏障目标协程数量
 */
#define ASYNC_SET_BARRIER_TARGET(barr_name, target) \
  sch_cortn_set_barrier_target(barr_name, target)

/**
 * @brief 运行一个协程
 * @param  name             协程名
 * @param  func             任务函数指针
 * @param  args             任务参数
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_cortn_run(const char *name, cortn_func_t func, void *args);

/**
 * @brief 停止一个协程
 * @param  name            协程名
 * @retval uint8_t         是否成功
 */
extern uint8_t sch_cortn_stop(const char *name);

/**
 * @brief 获取调度器内协程数量
 */
extern uint16_t sch_cortn_get_num(void);

/**
 * @brief 查询指定协程是否正在运行
 * @param  name             协程名
 * @retval uint8_t             协程是否正在运行
 */
extern uint8_t sch_cortn_get_running(const char *name);

/**
 * @brief 查询指定协程是否处于等待消息状态
 * @param  name             协程名
 * @retval uint8_t             协程是否处于等待消息状态
 */
extern uint8_t sch_cortn_get_waiting_msg(const char *name);

/**
 * @brief 发送消息给指定协程并唤醒
 * @param  name             协程名
 * @param  msg              消息指针
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_cortn_send_msg_to(const char *name, void *msg);

/**
 * @brief 手动释放一个屏障
 * @param  name            屏障名
 * @retval uint8_t         是否成功
 */
extern uint8_t sch_cortn_release_barrier(const char *name);

/**
 * @brief 获取指定屏障等待协程数量
 * @param  name            屏障名
 * @retval uint16_t        等待数量
 */
extern uint16_t sch_cortn_get_barrier_num(const char *name);

/**
 * @brief 设置指定屏障目标协程数量
 * @param  name            屏障名
 * @param  target          目标数量
 */
extern uint8_t sch_cortn_set_barrier_target(const char *name, uint16_t target);

#endif  // SCH_CFG_ENABLE_COROUTINE
#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_COROUTINE_H

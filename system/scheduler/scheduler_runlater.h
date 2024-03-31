#ifndef _SCHEDULER_CALLLATER_H
#define _SCHEDULER_CALLLATER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"

#if SCH_CFG_ENABLE_CALLLATER
#include "macro.h"

/**
 * @brief 在指定时间后执行目标函数
 * @param  func             任务函数指针
 * @param  delay_us          延时启动时间(us)
 * @param  ...              可变任务参数(必须与目标函数参数类型**对应一致**)
 * @retval uint8_t          是否成功(参数过多/参数过大)
 * @warning 所有参数必须都是变量, 常量参数无法取址和判断类型.
 * @warning 暂不支持浮点数, 但可以通过指针传递(注意生命周期)
 */
#define sch_runlater(func, delay_us, ...) \
  EVAL(__sch_runlater, __VA_ARGS__)(func, delay_us, ##__VA_ARGS__)

/**
 * @brief 取消所有对应函数的延时调用任务
 * @param func              任务函数指针
 */
#define sch_runlater_cancel(func) __sch_runlater_cancel((void *)func)

/*********************内部宏定义**********************/

extern uint8_t __sch_runlater(void *func_addr, uint64_t delay_us, uint8_t argc,
                              void *arg_addr[], uint8_t arg_size[]);
extern void __sch_runlater_cancel(void *func_addr);
#define __sch_runlater0(func, delay_us) \
  __sch_runlater((void *)func, delay_us, 0, NULL, NULL)
#define __sch_runlater1(func, delay_us, arg1)                            \
  __sch_runlater((void *)func, delay_us, 1, (void *[]){(void *)&(arg1)}, \
                 (uint8_t[]){sizeof(arg1)})
#define __sch_runlater2(func, delay_us, arg1, arg2)            \
  __sch_runlater((void *)func, delay_us, 2,                    \
                 (void *[]){(void *)&(arg1), (void *)&(arg2)}, \
                 (uint8_t[]){sizeof(arg1), sizeof(arg2)})
#define __sch_runlater3(func, delay_us, arg1, arg2, arg3)            \
  __sch_runlater(                                                    \
      (void *)func, delay_us, 3,                                     \
      (void *[]){(void *)&(arg1), (void *)&(arg2), (void *)&(arg3)}, \
      (uint8_t[]){sizeof(arg1), sizeof(arg2), sizeof(arg3)})
#define __sch_runlater4(func, delay_us, arg1, arg2, arg3, arg4)     \
  __sch_runlater(                                                   \
      (void *)func, delay_us, 4,                                    \
      (void *[]){(void *)&(arg1), (void *)&(arg2), (void *)&(arg3), \
                 (void *)&(arg4)},                                  \
      (uint8_t[]){sizeof(arg1), sizeof(arg2), sizeof(arg3), sizeof(arg4)})
#define __sch_runlater5(func, delay_us, arg1, arg2, arg3, arg4, arg5)          \
  __sch_runlater((void *)func, delay_us, 5,                                    \
                 (void *[]){(void *)&(arg1), (void *)&(arg2), (void *)&(arg3), \
                            (void *)&(arg4), (void *)&(arg5)},                 \
                 (uint8_t[]){sizeof(arg1), sizeof(arg2), sizeof(arg3),         \
                             sizeof(arg4), sizeof(arg5)})
#define __sch_runlater6(func, delay_us, arg1, arg2, arg3, arg4, arg5, arg6) \
  __sch_runlater(                                                           \
      (void *)func, delay_us, 6,                                            \
      (void *[]){(void *)&(arg1), (void *)&(arg2), (void *)&(arg3),         \
                 (void *)&(arg4), (void *)&(arg5), (void *)&(arg6)},        \
      (uint8_t[]){sizeof(arg1), sizeof(arg2), sizeof(arg3), sizeof(arg4),   \
                  sizeof(arg5), sizeof(arg6)})
#define __sch_runlater7(func, delay_us, arg1, arg2, arg3, arg4, arg5, arg6, \
                        arg7)                                               \
  __sch_runlater(                                                           \
      (void *)func, delay_us, 7,                                            \
      (void *[]){(void *)&(arg1), (void *)&(arg2), (void *)&(arg3),         \
                 (void *)&(arg4), (void *)&(arg5), (void *)&(arg6),         \
                 (void *)&(arg7)},                                          \
      (uint8_t[]){sizeof(arg1), sizeof(arg2), sizeof(arg3), sizeof(arg4),   \
                  sizeof(arg5), sizeof(arg6), sizeof(arg7)})
#define __sch_runlater8(func, delay_us, arg1, arg2, arg3, arg4, arg5, arg6, \
                        arg7, arg8)                                         \
  __sch_runlater(                                                           \
      (void *)func, delay_us, 8,                                            \
      (void *[]){(void *)&(arg1), (void *)&(arg2), (void *)&(arg3),         \
                 (void *)&(arg4), (void *)&(arg5), (void *)&(arg6),         \
                 (void *)&(arg7), (void *)&(arg8)},                         \
      (uint8_t[]){sizeof(arg1), sizeof(arg2), sizeof(arg3), sizeof(arg4),   \
                  sizeof(arg5), sizeof(arg6), sizeof(arg7), sizeof(arg8)})
#define __sch_runlater9(func, delay_us, arg1, arg2, arg3, arg4, arg5, arg6, \
                        arg7, arg8, arg9)                                   \
  __sch_runlater(                                                           \
      (void *)func, delay_us, 9,                                            \
      (void *[]){(void *)&(arg1), (void *)&(arg2), (void *)&(arg3),         \
                 (void *)&(arg4), (void *)&(arg5), (void *)&(arg6),         \
                 (void *)&(arg7), (void *)&(arg8), (void *)&(arg9)},        \
      (uint8_t[]){sizeof(arg1), sizeof(arg2), sizeof(arg3), sizeof(arg4),   \
                  sizeof(arg5), sizeof(arg6), sizeof(arg7), sizeof(arg8),   \
                  sizeof(arg9)})
#define __sch_runlater10(func, delay_us, arg1, arg2, arg3, arg4, arg5, arg6, \
                         arg7, arg8, arg9, arg10)                            \
  __sch_runlater(                                                            \
      (void *)func, delay_us, 10,                                            \
      (void *[]){(void *)&(arg1), (void *)&(arg2), (void *)&(arg3),          \
                 (void *)&(arg4), (void *)&(arg5), (void *)&(arg6),          \
                 (void *)&(arg7), (void *)&(arg8), (void *)&(arg9),          \
                 (void *)&(arg10)},                                          \
      (uint8_t[]){sizeof(arg1), sizeof(arg2), sizeof(arg3), sizeof(arg4),    \
                  sizeof(arg5), sizeof(arg6), sizeof(arg7), sizeof(arg8),    \
                  sizeof(arg9), sizeof(arg10)})
#define __sch_runlater11 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater12 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater13 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater14 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater15 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater16 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater17 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater18 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater19 ERR_TOO_MANY_PARAMETERS
#define __sch_runlater20 ERR_TOO_MANY_PARAMETERS
// 够了吧(

#endif  // SCH_CFG_ENABLE_CALLLATER
#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_CALLLATER_H

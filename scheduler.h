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

extern uint16_t Sch_AddTask(void (*task)(void), float freqHz, uint8_t enable);
extern void Scheduler_Run(const uint8_t block);

extern void Sch_SetTaskState(uint16_t taskId, uint8_t enable);
extern void Sch_DelTask(uint16_t taskId);
extern void Sch_SetTaskFreq(uint16_t taskId, float freqHz);
extern int Sch_GetTaskNum(void);
extern uint16_t Sch_GetTaskId(void (*task)(void));

#if _SCH_ENABLE_HIGH_PRIORITY
extern void Sch_SetHighPriorityTask(uint16_t taskId);
#endif

#if _SCH_ENABLE_COROUTINE
enum CR_MODES {
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

extern uint16_t Sch_AddCoron(void (*task)(void), uint8_t enable,
                             enum CR_MODES mode);
extern void Sch_SetCoronState(uint16_t taskId, uint8_t enable,
                              uint8_t clearState);
extern void Sch_DelCoron(uint16_t taskId);
extern int Sch_GetCoronNum(void);
extern uint16_t Sch_GetCoronId(void (*task)(void));

/**
 * @brief 运行一次协程, 执行完毕后自动删除
 */
#define Sch_RunCorontine(task) Sch_AddCoron(task, 1, CR_MODE_AUTODEL)

#endif  // _SCH_ENABLE_COROUTINE

#if _SCH_ENABLE_CALLLATER
extern void Sch_CallLater(void (*task)(void), m_time_t delayUs);
extern void Sch_CancelCallLater(void (*task)(void));
#endif  // _SCH_ENABLE_CALLLATER

#endif  // _SCHEDULER_H_

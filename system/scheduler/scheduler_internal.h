#ifndef _SCHEDULER_INTERNAL_H
#define _SCHEDULER_INTERNAL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>

#include "log.h"
#include "scheduler.h"
#include "ulist.h"

#define _INLINE __attribute__((always_inline)) inline
#define _STATIC_INLINE static _INLINE

/**
 * @brief 获取系统时钟, 无单位
 * @retval uint64_t          系统时钟
 */
_STATIC_INLINE uint64_t get_sys_tick(void) { return m_tick(); }

/**
 * @brief 获取系统时钟频率, 单位为Hz
 * @retval uint64_t          系统时钟频率
 */
_STATIC_INLINE uint64_t get_sys_freq(void) { return m_tick_clk; }

/**
 * @brief 获取系统时钟, 单位为us
 * @retval uint64_t          系统时钟
 */
_STATIC_INLINE uint64_t get_sys_us(void) {
  static uint64_t div = 0;
  if (!div) div = get_sys_freq() / 1000000;
  return get_sys_tick() / div;
}

/**
 * @brief 转换时钟为us
 * @param  tick            时钟
 * @retval float           us
 */
_STATIC_INLINE float tick_to_us(uint64_t tick) {
  static float tick2us = 0;
  if (!tick2us) tick2us = (float)1000000 / (float)get_sys_freq();
  return tick2us * (float)tick;
}

/**
 * @brief 转换us为时钟
 * @param  us              us
 * @retval uint64_t        时钟
 */
_STATIC_INLINE uint64_t us_to_tick(float us) {
  static uint64_t us2tick = 0;
  if (!us2tick) us2tick = get_sys_freq() / 1000000;
  return us * us2tick;
}

/**
 * @brief 快速字符串比较
 * @param  str1             字符串1
 * @param  str2             字符串2
 * @retval uint8_t          是否相等(1/0)
 */
_STATIC_INLINE uint8_t fast_strcmp(const char *str1, const char *str2) {
  if (str1 == str2) return 1;
  while ((*str1) && (*str2)) {
    if ((*str1++) != (*str2++)) return 0;
  }
  return (!*str1) && (!*str2);
}

#if SCH_CFG_STATIC_NAME
#define ID_NAME_VAR(name) char name[SCH_CFG_STATIC_NAME_LEN]
#define ID_NAME_SET(name, str) strncpy(name, str, SCH_CFG_STATIC_NAME_LEN)
#else
#define ID_NAME_VAR(name) const char *name
#define ID_NAME_SET(name, str) name = str
#endif

//////// 子模块的运行函数 ////////
extern void Event_Runner(void);
extern void SoftInt_Runner(void);
extern uint64_t Task_Runner(void);
extern uint64_t Cortn_Runner(void);
extern uint64_t CallLater_Runner(void);
extern uint8_t DebugInfo_Runner(uint64_t sleep_us);

#if SCH_CFG_DEBUG_REPORT
#include "term_table.h"
//////// 子模块向调试输出表添加数据的函数 ////////
extern void sch_task_add_debug(TT tt, uint64_t period, uint64_t *other);
extern void sch_event_add_debug(TT tt, uint64_t period, uint64_t *other);
extern void sch_cortn_add_debug(TT tt, uint64_t period, uint64_t *other);
extern void sch_task_finish_debug(uint8_t first_print, uint64_t offset);
extern void sch_event_finish_debug(uint8_t first_print, uint64_t offset);
extern void sch_cortn_finish_debug(uint8_t first_print, uint64_t offset);
#endif

#if SCH_CFG_ENABLE_TERMINAL
//////// 子模块的命令行回调函数 ////////
extern void task_cmd_func(EmbeddedCli *cli, char *args, void *context);
extern void event_cmd_func(EmbeddedCli *cli, char *args, void *context);
extern void cortn_cmd_func(EmbeddedCli *cli, char *args, void *context);
extern void softint_cmd_func(EmbeddedCli *cli, char *args, void *context);
#endif

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_INTERNAL_H

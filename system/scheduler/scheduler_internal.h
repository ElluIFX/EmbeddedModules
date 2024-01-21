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

_STATIC_INLINE uint64_t get_sys_tick(void) {
  return (uint64_t)get_system_ticks();
}
_STATIC_INLINE uint64_t get_sys_freq(void) { return SystemCoreClock; }

_STATIC_INLINE uint64_t get_sys_us(void) {
  static uint64_t div = 0;
  if (!div) div = get_sys_freq() / 1000000;
  return get_sys_tick() / div;
}

_STATIC_INLINE float tick_to_us(uint64_t tick) {
  static float tick2us = 0;
  if (!tick2us) tick2us = (float)1000000 / (float)get_sys_freq();
  return tick2us * (float)tick;
}
_STATIC_INLINE uint64_t us_to_tick(float us) {
  static uint64_t us2tick = 0;
  if (!us2tick) us2tick = get_sys_freq() / 1000000;
  return us * us2tick;
}

_STATIC_INLINE uint8_t fast_strcmp(const char *str1, const char *str2) {
  if (str1 == str2) return 1;
  while ((*str1) && (*str2)) {
    if ((*str1++) != (*str2++)) return 0;
  }
  return (!*str1) && (!*str2);
}

extern void Event_Runner(void);
extern void SoftInt_Runner(void);
extern uint64_t Task_Runner(void);
extern uint64_t Cortn_Runner(void);
extern uint64_t CallLater_Runner(void);

#if _SCH_DEBUG_REPORT
#include "term_table.h"
extern void sch_task_add_debug(TT tt, uint64_t period, uint64_t *other);
extern void sch_event_add_debug(TT tt, uint64_t period, uint64_t *other);
extern void sch_cortn_add_debug(TT tt, uint64_t period, uint64_t *other);
extern void sch_task_finish_debug(uint8_t first_print, uint64_t offset);
extern void sch_event_finish_debug(uint8_t first_print, uint64_t offset);
extern void sch_cortn_finish_debug(uint8_t first_print, uint64_t offset);
#endif

#if _SCH_ENABLE_TERMINAL
extern void task_cmd_func(EmbeddedCli *cli, char *args, void *context);
extern void event_cmd_func(EmbeddedCli *cli, char *args, void *context);
extern void cortn_cmd_func(EmbeddedCli *cli, char *args, void *context);
extern void softint_cmd_func(EmbeddedCli *cli, char *args, void *context);
#endif

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_INTERNAL_H

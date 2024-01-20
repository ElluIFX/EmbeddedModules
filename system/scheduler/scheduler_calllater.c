#include "scheduler_calllater.h"

#include "scheduler_internal.h"

#if _SCH_ENABLE_CALLLATER
#pragma pack(1)
typedef struct {       // 延时调用任务结构
  sch_func_t task;     // 任务函数指针
  uint64_t runTimeUs;  // 执行时间(us)
  void *args;          // 任务参数
} scheduler_call_later_t;
#pragma pack()

static ulist_t clist = {.data = NULL,
                        .cap = 0,
                        .num = 0,
                        .elfree = NULL,
                        .isize = sizeof(scheduler_call_later_t),
                        .cfg = ULIST_CFG_CLEAR_DIRTY_REGION};

_INLINE uint64_t CallLater_Runner(void) {
  if (!clist.num) return UINT64_MAX;
  uint64_t sleep_us = UINT64_MAX;
  uint64_t now = get_sys_us();
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (now >= callLater_p->runTimeUs) {
      callLater_p->task(callLater_p->args);
      ulist_delete(&clist, callLater_p - (scheduler_call_later_t *)clist.data);
      return 0;  // 有任务被执行，不确定
    }
    if (callLater_p->runTimeUs - now < sleep_us) {
      sleep_us = callLater_p->runTimeUs - now;
    }
  }
  return sleep_us;
}

uint8_t Sch_CallLater(sch_func_t func, uint64_t delayUs, void *args) {
  scheduler_call_later_t *p = (scheduler_call_later_t *)ulist_append(&clist);
  if (p == NULL) return 0;
  p->task = func;
  p->runTimeUs = delayUs + get_sys_us();
  p->args = args;
  return 1;
}

void Sch_CancelCallLater(sch_func_t func) {
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (callLater_p->task == func) {
      ulist_delete(&clist, callLater_p - (scheduler_call_later_t *)clist.data);
      return;
    }
  }
}
#endif  // _SCH_ENABLE_CALLLATER

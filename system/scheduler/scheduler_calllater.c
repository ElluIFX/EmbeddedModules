#include "scheduler_calllater.h"

#include "scheduler_internal.h"

#if _SCH_ENABLE_CALLLATER
#pragma pack(1)
typedef struct {       // 延时调用任务结构
  cl_func_t task;      // 任务函数指针
  uint64_t runTimeUs;  // 执行时间(us)
  void *args;          // 任务参数
} scheduler_call_later_t;
#pragma pack()

static ulist_t clist = {.data = NULL,
                        .cap = 0,
                        .num = 0,
                        .elfree = NULL,
                        .isize = sizeof(scheduler_call_later_t),
                        .cfg = ULIST_CFG_NO_SHRINK | ULIST_CFG_NO_AUTO_FREE};

_INLINE uint64_t CallLater_Runner(void) {
  static uint64_t last_active_us = 0;
  if (!clist.num) {
    if (clist.cap &&
        get_sys_us() - last_active_us > 10000000) {  // 10s无触发，释放内存
      ulist_mem_shrink(&clist);
    }
    return UINT64_MAX;
  }
  uint64_t sleep_us = UINT64_MAX;
  uint64_t now = get_sys_us();
  last_active_us = now;
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (now >= callLater_p->runTimeUs) {
      callLater_p->task(callLater_p->args);
      ulist_remove(&clist, callLater_p);
      return 0;  // 有任务被执行，不确定
    }
    if (callLater_p->runTimeUs - now < sleep_us) {
      sleep_us = callLater_p->runTimeUs - now;
    }
  }
  return sleep_us;
}

uint8_t Sch_CallLater(cl_func_t func, uint64_t delayUs, void *args) {
  scheduler_call_later_t task = {
      .task = func, .runTimeUs = get_sys_us() + delayUs, .args = args};
  return ulist_append_copy(&clist, &task);
}

void Sch_CancelCallLater(cl_func_t func) {
  ulist_foreach(&clist, scheduler_call_later_t, callLater_p) {
    if (callLater_p->task == func) {
      ulist_remove(&clist, callLater_p);
      callLater_p--;
      callLater_p_end--;
    }
  }
}
#endif  // _SCH_ENABLE_CALLLATER

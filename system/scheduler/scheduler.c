#include "scheduler_internal.h"

/**
 * @brief 利用Systick中断配合WFI实现低功耗延时
 * @param  us 延时时间(us)
 */
static void SysTick_Sleep(uint32_t us) {
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  uint32_t load = SysTick->LOAD;   // 保存原本的重装载值
  SysTick->LOAD = us_to_tick(us);  // 在指定时间后中断
  uint32_t val = SysTick->VAL;
  SysTick->VAL = 0;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
  __wfi();  // 关闭CPU等待中断
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  SysTick->LOAD = load;  // 恢复重装载值
  SysTick->VAL = val;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

__weak void Scheduler_Idle_Callback(uint64_t idleTimeUs) {
  if (idleTimeUs > 1000) idleTimeUs = 1000;  // 最多休眠1ms以保证事件的及时响应
#if !MOD_CFG_USE_OS_NONE
  m_delay_us(idleTimeUs);
#else  // 关闭CPU
#if MOD_CFG_WFI_WHEN_SYSTEM_IDLE
  SysTick_Sleep(idleTimeUs);
#endif
#endif
}

uint64_t _INLINE Scheduler_Run(const uint8_t block) {
// #define CHECK(rslp, name) LOG_DEBUG_LIMIT(1000, #name " rslp=%d", rslp)
#define CHECK(rslp, name) ((void)0)
  uint64_t mslp, rslp;
  do {
    mslp = UINT64_MAX;
#if SCH_CFG_ENABLE_SOFTINT
    SoftInt_Runner();
#endif
#if SCH_CFG_ENABLE_TASK
    rslp = Task_Runner();
    CHECK(rslp, Task);
    if (rslp < mslp) mslp = rslp;
#endif
#if SCH_CFG_ENABLE_COROUTINE
    rslp = Cortn_Runner();
    CHECK(rslp, Cortn);
    if (rslp < mslp) mslp = rslp;
#endif
#if SCH_CFG_ENABLE_CALLLATER
    rslp = CallLater_Runner();
    CHECK(rslp, CallLater);
    if (rslp < mslp) mslp = rslp;
#endif
#if SCH_CFG_ENABLE_EVENT
    Event_Runner();
#endif
    if (mslp == UINT64_MAX) mslp = 1000;  // 没有任何任务
#if SCH_CFG_DEBUG_REPORT
    if (DebugInfo_Runner(mslp)) continue;
#endif
    if (block && mslp) {
      Scheduler_Idle_Callback(mslp);
    }
  } while (block);
  return mslp;
}

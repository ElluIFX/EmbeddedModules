#include "scheduler_internal.h"

#if MOD_CFG_WFI_WHEN_SYSTEM_IDLE && !MOD_CFG_OS_AVAILABLE
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
#endif

__weak void scheduler_idle_handler(uint64_t idleTimeUs) {
    if (idleTimeUs > 1000)
        idleTimeUs = 1000;  // 最多休眠1ms以保证事件的及时响应
#if MOD_CFG_OS_AVAILABLE
    m_delay_us(idleTimeUs);
#else  // 关闭CPU
#if MOD_CFG_WFI_WHEN_SYSTEM_IDLE
    SysTick_Sleep(idleTimeUs);
#endif
#endif
}

uint64_t _INLINE scheduler_run(const uint8_t block) {
// #define CHECK(rslp, name) LOG_DEBUG_LIMIT(1000, #name " rslp=%d", rslp)
#define CHECK(rslp, name) ((void)0)
    uint64_t mslp, rslp;
    do {
        mslp = UINT64_MAX;
#if SCH_CFG_ENABLE_SOFTINT
        soft_int_runner();
#endif
#if SCH_CFG_ENABLE_TASK
        rslp = task_runner();
        CHECK(rslp, task);
        if (rslp < mslp)
            mslp = rslp;
#endif
#if SCH_CFG_ENABLE_COROUTINE
        rslp = cortn_runner();
        CHECK(rslp, cortn);
        if (rslp < mslp)
            mslp = rslp;
#endif
#if SCH_CFG_ENABLE_CALLLATER
        rslp = runlater_runner();
        CHECK(rslp, runlater);
        if (rslp < mslp)
            mslp = rslp;
#endif
#if SCH_CFG_ENABLE_EVENT
        event_runner();
#endif
        if (mslp == UINT64_MAX)
            mslp = 1000;  // 没有任何任务
#if SCH_CFG_DEBUG_REPORT
        if (debug_info_runner(mslp))
            continue;
#endif
        if (block && mslp) {
            scheduler_idle_handler(mslp);
        }
    } while (block);
    return mslp;
}

#if SCH_CFG_DEBUG_REPORT
_INLINE uint8_t debug_info_runner(uint64_t sleep_us) {
    static uint8_t first_print = 1;
    static uint64_t last_print = 0;
    static uint64_t sleep_sum = 0;
    static uint16_t sleep_cnt = 0;
    uint64_t now = get_sys_tick();
    if (!first_print) {  // 因为初始化耗时等原因，第一次的数据无参考价值，不打印
        if (now - last_print <= us_to_tick(SCH_CFG_DEBUG_PERIOD * 1000000)) {
            sleep_sum += sleep_us;
            sleep_cnt++;
            return 0;
        }
        uint64_t period = now - last_print;
        uint64_t other = period;
        TT tt = TT_NewTable(-1);
        TT_AddTitle(tt,
                    TT_Str(TT_ALIGN_CENTER, TT_FMT1_YELLOW, TT_FMT2_BOLD,
                           "[ Scheduler Debug Report ]"),
                    '-');
#if SCH_CFG_ENABLE_TASK
        sch_task_add_debug(tt, period, &other);
#endif  // SCH_CFG_ENABLE_TASK
#if SCH_CFG_ENABLE_EVENT
        sch_event_add_debug(tt, period, &other);
#endif  // SCH_CFG_ENABLE_EVENT
#if SCH_CFG_ENABLE_COROUTINE
        sch_cortn_add_debug(tt, period, &other);
#endif  // SCH_CFG_ENABLE_COROUTINE
        TT_AddTitle(tt,
                    TT_Str(TT_ALIGN_LEFT, TT_FMT1_BLUE, TT_FMT2_BOLD,
                           "[ Scheduler Info ]"),
                    '-');
        TT_AddString(tt,
                     TT_FmtStr(TT_ALIGN_CENTER, TT_FMT1_GREEN, TT_FMT2_NONE,
                               "Dur: %.3fs / Idle: %.2f%% / AvgSleep: %.3fus",
                               tick_to_us(now - last_print) / 1000000.0,
                               (float)other / period * 100,
                               (float)sleep_sum / sleep_cnt),
                     -1);
        sleep_sum = 0;
        sleep_cnt = 0;
        TT_AddSeparator(tt, TT_FMT1_YELLOW, TT_FMT2_BOLD, '-');
        TT_LineBreak(tt, 1);
        TT_Print(tt);
        TT_FreeTable(tt);
    }
    now = get_sys_tick() - now;  // 补偿打印LOG的时间
#if SCH_CFG_ENABLE_TASK
    sch_task_finish_debug(first_print, now);
#endif  // SCH_CFG_ENABLE_TASK
#if SCH_CFG_ENABLE_EVENT
    sch_event_finish_debug(first_print, now);
#endif  // SCH_CFG_ENABLE_EVENT
#if SCH_CFG_ENABLE_COROUTINE
    sch_cortn_finish_debug(first_print, now);
#endif
    last_print = get_sys_tick();
    first_print = 0;
    return 1;
}
#endif  // SCH_CFG_DEBUG_REPORT

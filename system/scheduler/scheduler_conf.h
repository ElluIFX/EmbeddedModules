#ifndef _SCHEDULER_CONF_H_
#define _SCHEDULER_CONF_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SCH_ENABLE_TASK
#define _SCH_ENABLE_TASK 1  // 支持任务
#endif

#ifndef _SCH_ENABLE_EVENT
#define _SCH_ENABLE_EVENT 1  // 支持事件
#endif

#ifndef _SCH_ENABLE_COROUTINE
#define _SCH_ENABLE_COROUTINE 1  // 支持宏协程
#endif

#ifndef _SCH_ENABLE_CALLLATER
#define _SCH_ENABLE_CALLLATER 1  // 支持延时调用
#endif

#ifndef _SCH_ENABLE_SOFTINT
#define _SCH_ENABLE_SOFTINT 1  // 支持软中断
#endif

#ifndef _SCH_COMP_RANGE_US
#define _SCH_COMP_RANGE_US 1000  // 任务调度自动补偿范围(us)
#endif

#ifndef _SCH_EVENT_ALLOW_DUPLICATE
#define _SCH_EVENT_ALLOW_DUPLICATE 0  // 允许事件重复注册
#endif

#ifndef _SCH_DEBUG_REPORT
#define _SCH_DEBUG_REPORT 0  // 输出调度器统计信息(调试模式/低性能)
#endif

#ifndef _SCH_DEBUG_PERIOD
#define _SCH_DEBUG_PERIOD 5  // 调试报告打印周期(s)(超过10s的值可能导致溢出)
#endif

#ifndef _SCH_ENABLE_TERMINAL
#define _SCH_ENABLE_TERMINAL 0  // 是否启用终端命令集(依赖embedded-cli)
#endif

#ifdef __cplusplus
}
#endif

#endif  // _SCHEDULER_CONF_H_

#ifndef KLITE_CFG_H
#define KLITE_CFG_H

#include "modules.h"

#if !KCONFIG_AVAILABLE

#define KLITE_CFG_FREQ 1000            // 内核时基频率(赫兹)
#define KLITE_CFG_MAX_PRIO 7           // 最大优先级
#define KLITE_CFG_HEAP_USE_BARE 0      // 使用裸机基础内存管理器
#define KLITE_CFG_HEAP_USE_LWMEM 0     // 使用lwmem内存管理器
#define KLITE_CFG_HEAP_USE_HEAP4 1     // 使用heap4内存管理器
#define KLITE_CFG_IDLE_THREAD_STACK_SIZE 256  // 空闲线程栈大小
#define KLITE_CFG_DEFAULT_STACK_SIZE 1024     // 默认线程栈大小
#define KLITE_CFG_WAIT_LIST_ORDER_BY_PRIOR 1  // 等待队列按优先级排序
#define KLITE_CFG_INTERFACE_ENABLE 0          // 统一接口使能

#define KLITE_CFG_STACK_OVERFLOW_DETECT 1      // 栈溢出保护
#define KLITE_CFG_STACKOF_BEHAVIOR_SYSRESET 1  // 栈溢出时系统复位
#define KLITE_CFG_STACKOF_BEHAVIOR_SUSPEND 0   // 栈溢出时挂起线程
#define KLITE_CFG_STACKOF_BEHAVIOR_HARDFLT 0  // 栈溢出时访问0x10触发异常
#define KLITE_CFG_STACKOF_BEHAVIOR_CALLBACK 0  // 栈溢出时调用回调函数

#define KLITE_CFG_OPT_MUTEX 1        // 编译互斥锁功能
#define KLITE_CFG_OPT_SEM 1          // 编译信号量功能
#define KLITE_CFG_OPT_COND 0         // 编译条件变量功能
#define KLITE_CFG_OPT_BARRIER 0      // 编译屏障功能
#define KLITE_CFG_OPT_RWLOCK 0       // 编译读写锁功能
#define KLITE_CFG_OPT_EVENT 0        // 编译事件功能
#define KLITE_CFG_OPT_EVENT_FLAGS 0  // 编译事件标志功能
#define KLITE_CFG_OPT_MAILBOX 0      // 编译邮箱功能
#define KLITE_CFG_OPT_MPOOL 0        // 编译内存池功能
#define KLITE_CFG_OPT_MSG_QUEUE 0    // 编译消息队列功能
#define KLITE_CFG_OPT_SOFT_TIMER 0   // 编译软定时器功能
#define KLITE_CFG_OPT_THREAD_POOL 0  // 编译线程池功能

#endif /* !KCONFIG_AVAILABLE */

#endif /* KLITE_CFG_H */

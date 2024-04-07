#ifndef __KLITE_CFG_H
#define __KLITE_CFG_H

#include "modules.h"

#if !KCONFIG_AVAILABLE

#define KLITE_CFG_FREQ 1000                   // 内核时基频率(赫兹)
#define KLITE_CFG_MAX_PRIO 7                  // 最大优先级
#define KLITE_CFG_IDLE_THREAD_STACK_SIZE 256  // 空闲线程栈大小
#define KLITE_CFG_DEFAULT_STACK_SIZE 1024     // 默认线程栈大小
#define KLITE_CFG_WAIT_LIST_ORDER_BY_PRIOR 1  // 等待队列按优先级排序

#define KLITE_CFG_HEAP_USE_BUILTIN 1  // 使用内置内存管理器
#define KLITE_CFG_HEAP_USE_LWMEM 0    // 第三方管理器:lwmem
#define KLITE_CFG_HEAP_USE_HEAP4 0    // 第三方管理器:heap4

#if KLITE_CFG_HEAP_USE_BUILTIN  // 内置内存管理器配置

#define KLITE_CFG_HEAP_CLEAR_MEMORY_ON_FREE (0)  // 释放内存时清零
#define KLITE_CFG_HEAP_STORAGE_PREV_NODE (1)     // 存储前一个节点指针
#define KLITE_CFG_HEAP_TRACE_OWNER (1)           // 跟踪内存拥有者
#define KLITE_CFG_HEAP_ALIGN_BYTE (4)            // 内存对齐字节

#endif

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
#define KLITE_CFG_OPT_MQUEUE 0       // 编译消息队列功能
#define KLITE_CFG_OPT_TIMER 0        // 编译软定时器功能
#define KLITE_CFG_OPT_THREAD_POOL 0  // 编译线程池功能

#endif /* !KCONFIG_AVAILABLE */

#endif /* KLITE_CFG_H */

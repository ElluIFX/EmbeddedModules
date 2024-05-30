#ifndef __KL_PRIV_H__
#define __KL_PRIV_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "kl_cfg.h"
#include "kl_def.h"
#include "klite.h"

/* Compatible with armcc5 armcc6 gcc icc */
#if defined(__GNUC__) || (__ARMCC_VERSION >= 6100100)
#define __weak __attribute__((weak))
#endif

#define KL_STACK_MAGIC_VALUE 0xDEADBEEFU
#define KL_THREAD_MAGIC_VALUE 0xFEEDU

#define KL_SET_FLAG(flags, mask) ((flags) |= (mask))
#define KL_GET_FLAG(flags, mask) ((flags) & (mask))
#define KL_CLR_FLAG(flags, mask) ((flags) &= ~(mask))

// 当前线程控制块
extern kl_thread_t kl_sched_tcb_now;

// 切换目标控制块
extern kl_thread_t kl_sched_tcb_next;

// 平台实现: 进入临界区, 并执行需要在内核初始化前执行的操作
void kl_port_sys_init(void);

// 平台实现: 执行内核初始化后的操作, 退出临界区, 退出后系统应直接进入调度
void kl_port_sys_start(void);

// 平台实现: 系统空闲回调
// @param time: 系统空闲时间, 单位tick
void kl_port_sys_idle(kl_tick_t time);

// 平台实现: 触发PendSV, 进行上下文切换
// @note: kl_sched_tcb_now -> kl_sched_tcb_next
void kl_port_context_switch(void);

// 平台实现: 初始化线程栈
// @param stack_base: 栈基地址
// @param stack_top: 栈顶地址
// @param entry: 线程入口地址
// @param arg: 线程参数
// @param exit: 线程结束回调地址
// @return: 线程栈指针
void* kl_port_stack_init(void* stack_base, void* stack_top, void* entry,
                         void* arg, void* exit);

// 平台实现: 进入临界区
void kl_port_enter_critical(void);

// 平台实现: 退出临界区
void kl_port_leave_critical(void);

// 初始化内核堆
void kl_heap_init(void* addr, kl_size_t size);

#if KLITE_CFG_HEAP_AUTO_FREE
// 内核堆自动释放
void kl_heap_auto_free(kl_thread_t owner);
#endif

// 内核时钟递增
void kl_kernel_tick_source(kl_tick_t time);

// 内核空闲线程
void kl_kernel_idle_entry(void* args);

// 处理线程空闲任务
void kl_thread_idle_task(void);

// 初始化线程调度器
void kl_sched_init(void);

// 线程调度器空闲处理
// 如果有线程就绪, 则调度线程
// 如果没有线程就绪, 则进入系统空闲
void kl_sched_idle(void);

// 线程调度器时钟处理
// 如果有线程超时, 则唤醒线程
// @param time: 时钟增量
void kl_sched_timing(kl_tick_t time);

// 线程调度器挂起
void kl_sched_suspend(void);

// 线程调度器恢复
void kl_sched_resume(void);

// 执行线程切换
void kl_sched_switch(void);

// 将当前线程加入就绪队列并切换
// @param round_robin: 允许同优先级时间片轮转
void kl_sched_preempt(const bool round_robin);

// 重置线程优先级
// @param tcb: 线程控制块
// @param prio: 优先级
void kl_sched_tcb_reset_prio(kl_thread_t tcb, uint32_t prio);

// 从调度器移除线程, 并标记为EXIT状态
// @param tcb: 线程控制块
void kl_sched_tcb_remove(kl_thread_t tcb);

// 将线程加入就绪队列
// @param tcb: 线程控制块
// @param head: 是否插入队列头部
void kl_sched_tcb_ready(kl_thread_t tcb, const bool head);

// 将线程挂起
// @param tcb: 线程控制块
void kl_sched_tcb_suspend(kl_thread_t tcb);

// 将线程恢复
// @param tcb: 线程控制块
void kl_sched_tcb_resume(kl_thread_t tcb);

// 将线程加入睡眠队列
// @param tcb: 线程控制块
// @param timeout: 睡眠时间
void kl_sched_tcb_sleep(kl_thread_t tcb, kl_tick_t timeout);

// 将线程加入等待队列
// @param tcb: 线程控制块
// @param list: 等待队列
void kl_sched_tcb_wait(kl_thread_t tcb, struct kl_thread_list* list);

// 将线程同时加入等待队列和睡眠队列
// @param tcb: 线程控制块
// @param list: 等待队列
// @param timeout: 睡眠时间(等待超时时间)
void kl_sched_tcb_timed_wait(kl_thread_t tcb, struct kl_thread_list* list,
                             kl_tick_t timeout);

// 尝试唤醒等待队列中的线程
// @param list: 等待队列
// @return: 被唤醒的线程控制块, NULL表示无等待线程
kl_thread_t kl_sched_tcb_wake_from(struct kl_thread_list* list);

// Heap使用的独立互斥锁实现 (不支持递归!)
#define __KL_HEAP_MUTEX_IMPL__                                   \
    volatile static uint8_t heap_lock = 0;                       \
    static struct kl_thread_list heap_waitlist;                  \
    static void heap_mutex_lock(void) {                          \
        kl_port_enter_critical();                                \
        if (!heap_lock) {                                        \
            heap_lock = 1;                                       \
        } else {                                                 \
            kl_sched_tcb_wait(kl_sched_tcb_now, &heap_waitlist); \
            kl_sched_switch();                                   \
        }                                                        \
        kl_port_leave_critical();                                \
    }                                                            \
    static void heap_mutex_unlock(void) {                        \
        kl_port_enter_critical();                                \
        if (kl_sched_tcb_wake_from(&heap_waitlist)) {            \
            kl_sched_preempt(false);                             \
        } else {                                                 \
            heap_lock = 0;                                       \
        }                                                        \
        kl_port_leave_critical();                                \
    }

// 设置当前线程的errno
#define KL_SET_ERRNO(errno)                           \
    do {                                              \
        if (kl_sched_tcb_now) {                       \
            kl_sched_tcb_now->err = (uint8_t)(errno); \
        }                                             \
    } while (0)

// 检查是否超时并返回true/false
#define KL_RET_CHECK_TIMEOUT()               \
    do {                                     \
        if (kl_sched_tcb_now->timeout > 0) { \
            KL_SET_ERRNO(KL_ETIMEOUT);       \
            return false;                    \
        }                                    \
        return true;                         \
    } while (0)

#endif /* __KL_PRIV_H__ */

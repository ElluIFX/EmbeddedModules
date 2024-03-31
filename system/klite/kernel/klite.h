/******************************************************************************
 * Copyright (c) 2015-2023 jiangxiaogang<kerndev@foxmail.com>
 *
 * This file is part of KLite distribution.
 *
 * KLite is free software, you can redistribute it and/or modify it under
 * the MIT Licence.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#ifndef __KLITE_H
#define __KLITE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "klite_cfg.h"

typedef struct tcb *thread_t;
typedef struct sem *sem_t;
typedef struct event *event_t;
typedef struct mutex *mutex_t;
typedef struct cond *cond_t;

/******************************************************************************
 * kernel
 ******************************************************************************/

/**
 * @param heap_addr 内核堆起始地址
 * @param heap_size 内核堆大小
 * @brief 用于内核初始化在调用内核初始化时需保证中断处于关闭状态,
 * @warning 此函数只能执行一次, 在初始化内核之前不可调用内核其它函数。
 */
void kernel_init(void *heap_addr, uint32_t heap_size);

/**
 * @brief 用于启动内核, 此函正常情况下不会返回, 在调用之前至少要创建一个线程
 */
void kernel_start(void);

/**
 * @brief 进入临界区, 禁止中断, 允许嵌套
 */
void kernel_enter_critical(void);

/**
 * @brief 退出临界区, 允许中断, 允许嵌套
 */
void kernel_exit_critical(void);

/**
 * @retval KLite版本号, BIT[31:24]主版本号, BIT[23:16]次版本号, BIT[15:0]修订号
 */
uint32_t kernel_version(void);

/**
 * @brief 获取系统从启动到现在空闲线程占用CPU的总时间
 * @note 可使用此函数和kernel_tick_count()一起计算CPU占用率
 * @retval 系统空闲时间(Tick)
 */
uint32_t kernel_idle_time(void);

/**
 * @brief 此函数不是用户API, 而是由CPU的滴答时钟中断程序调用, 为系统提供时钟源。
 * @note 滴答定时器的周期决定了系统计时功能的细粒度 (1/KLITE_CFG_FREQ)
 * @param time 递增Tick数
 */
void kernel_tick(uint32_t time);

/**
 * @brief 此函数可以获取内核从启动到现在所运行的总时间
 * @retval 系统运行时间(Tick)
 */
uint32_t kernel_tick_count(void);

/**
 * @brief 此函数可以获取内核从启动到现在所运行的总时间
 * @retval 系统运行时间(Tick)
 */
uint64_t kernel_tick_count64(void);

/**
 * @brief 计算ms对应的Tick数
 * @param ms 毫秒
 * @retval 返回ms对应的Tick数
 */
static inline uint32_t kernel_ms_to_ticks(uint32_t ms) {
  return ((uint64_t)ms * KLITE_CFG_FREQ) / 1000;
}

/**
 * @brief 计算Tick对应的毫秒数
 * @param tick Tick数
 * @retval 返回Tick对应的毫秒数
 */
static inline uint32_t kernel_ticks_to_ms(uint32_t tick) {
  return ((uint64_t)tick * 1000) / KLITE_CFG_FREQ;
}

/**
 * @brief 计算us对应的Tick数
 * @param us 微秒
 * @retval 返回us对应的Tick数
 */
static inline uint32_t kernel_us_to_ticks(uint32_t us) {
  return ((uint64_t)us * KLITE_CFG_FREQ) / 1000000;
}

/**
 * @brief 计算Tick对应的微秒数
 * @param tick Tick数
 * @retval 返回Tick对应的微秒数
 */
static inline uint32_t kernel_ticks_to_us(uint32_t tick) {
  return ((uint64_t)tick * 1000000) / KLITE_CFG_FREQ;
}

/******************************************************************************
 * heap
 ******************************************************************************/

/**
 * @brief 用户在指定内存创建一个用于动态管理的堆内存
 * @param addr 动态分配起始地址
 * @param size 动态分配内存大小
 */
void heap_create(void *addr, uint32_t size);

/**
 * @brief 从堆中申请一段连续的内存, 功能和标准库的malloc()一样
 * @param size 申请内存大小
 * @retval 申请成功返回内存指针, 申请失败返回NULL
 */
void *heap_alloc(uint32_t size);

/**
 * @brief 释放内存, 功能和标准库的free()一样
 * @param mem 内存指针
 */
void heap_free(void *mem);

/**
 * @brief 重新分配内存大小, 功能和标准库的realloc()一样
 * @param mem 内存指针
 * @param size 申请内存大小
 * @retval 申请成功返回内存指针, 申请失败返回NULL
 */
void *heap_realloc(void *mem, uint32_t size);

/**
 * @brief 获取堆内存使用情况
 * @param used 已使用内存大小
 * @param free 空闲内存大小
 */
void heap_usage(uint32_t *used, uint32_t *free);

/**
 * @brief 获取堆内存使用率(0.0~1.0)
 * @retval 堆内存使用率
 */
float heap_usage_percent(void);

/******************************************************************************
 * thread
 ******************************************************************************/

/**
 * @brief 创建新线程, 并加入就绪队列
 * @param entry 线程入口函数
 * @param arg 线程入口函数的参数
 * @param stack_size 线程的栈大小（字节）, 0:使用默认值
 * @param prio 线程优先级 (>0) , 0:使用默认值
 * @retval 成功返回线程句柄, 失败返回NULL
 */
thread_t thread_create(void (*entry)(void *), void *arg, uint32_t stack_size,
                       uint32_t prio);

/**
 * @brief 删除线程, 并释放内存
 * @param thread 被删除的线程标识符
 * @warning 该函数不能用来结束当前线程, 如果想要结束当前线程,
 * 请使用thread_exit()或直接使用return退出主循环
 */
void thread_delete(thread_t thread);

/**
 * @brief 挂起线程, 使线程进入休眠状态, 并释放CPU控制权
 * @param thread 线程标识符
 */
void thread_suspend(thread_t thread);

/**
 * @brief 恢复线程, 使线程从休眠状态中恢复
 * @param thread 线程标识符
 */
void thread_resume(thread_t thread);

/**
 * @brief 挂起所有线程, 禁止线程调度, 允许嵌套
 * @warning 非中断安全, 挂起期间禁止调用内核API
 */
void thread_suspend_all(void);

/**
 * @brief 恢复所有线程, 允许线程调度, 允许嵌套
 * @warning 非中断安全, 挂起期间禁止调用内核API
 */
void thread_resume_all(void);

/**
 * @brief 使当前线程立即释放CPU控制权, 并进入就绪队列
 */
void thread_yield(void);

/**
 * @brief 将当前线程休眠一段时间, 释放CPU控制权
 * @param time 睡眠时间(取决于时钟源粒度)
 */
void thread_sleep(uint32_t time);

/**
 * @brief 退出当前线程, 此函数不会立即释放线程占用的内存, 需等待系统空闲时释放
 */
void thread_exit(void);

/**
 * @brief 用于获取当前线程标识符
 * @retval 调用线程的标识符
 */
thread_t thread_self(void);

/**
 * @brief 获取线程自创建以来所占用CPU的时间
 * @param thread 线程标识符
 * @retval 指定线程运行时间（Tick）
 * @note 在休眠期间的时间不计算在内, 可以使用此函数来监控某个线程的CPU占用率
 */
uint32_t thread_time(thread_t thread);

/**
 * @brief 获取线程栈信息
 * @param thread 线程标识符
 * @param stack_free 剩余栈空间
 * @param stack_size 栈总大小
 */
void thread_stack_info(thread_t thread, size_t *stack_free, size_t *stack_size);

/**
 * @brief 重新设置线程优先级, 立即生效
 * @param thread 线程标识符
 * @param prio 新的优先级 (>0) , 0:使用默认值
 * @warning 不允许在内核启动前修改线程优先级
 */
void thread_set_priority(thread_t thread, uint32_t prio);

/**
 * @brief 获取线程优先级
 * @param thread 线程标识符
 * @retval 线程优先级
 */
uint32_t thread_get_priority(thread_t thread);

/**
 * @brief 迭代获取所有线程
 * @param thread NULL:获取第1个线程, 其它:获取下一个线程
 * @retval 返回线程标识符, 如果返回NULL则说明没有更多线程
 */
thread_t thread_iter(thread_t thread);

/******************************************************************************
 * semaphore
 ******************************************************************************/

/**
 * @brief 创建信号量对象
 * @param value 信号量初始值
 * @retval 成功返回信号量标识符, 失败返回NULL
 */
sem_t sem_create(uint32_t value);

/**
 * @brief 删除对象, 并释放内存
 * @warning 在没有线程使用它时才能删除, 否则将导致未定义行为
 * @param sem 信号量标识符
 */
void sem_delete(sem_t sem);

/**
 * @brief 信号量计数值加1, 如果有线程在等待信号量, 此函数会唤醒优先级最高的线程
 * @param sem 信号量标识符
 */
void sem_post(sem_t sem);

/**
 * @brief 等待信号量, 信号量计数值减1, 如果当前信号量计数值为0, 则线程阻塞,
 * 直到计数值大于0
 * @param sem 信号量标识符
 */
void sem_wait(sem_t sem);

/**
 * @brief 定时等待信号量, 并将信号量计数值减1, 如果当前信号量计数值为0,
 * 则线程阻塞, 直到计数值大于0, 或者阻塞时间超过timeout指定的时间
 * @param sem 信号量标识符
 * @param timeout 超时时间（Tick）
 * @retval 剩余等待时间, 如果返回0则说明等待超时
 */
uint32_t sem_timed_wait(sem_t sem, uint32_t timeout);

/**
 * @brief 获取信号量计数值
 * @param sem 信号量标识符
 * @retval 信号量计数值
 */
uint32_t sem_value(sem_t sem);

/**
 * @brief 重置信号量计数值
 * @param sem 信号量标识符
 * @param value 信号量计数值
 */
void sem_reset(sem_t sem, uint32_t value);

/******************************************************************************
 * event
 ******************************************************************************/

/**
 * @brief 创建一个事件对象, 当auto_reset为true时事件会在传递成功后自动复位
 * @param auto_reset 是否自动复位事件
 * @retval 创建成功返回事件标识符, 失败返回NULL
 */
event_t event_create(bool auto_reset);

/**
 * @brief 删除事件对象, 并释放内存, 在没有线程使用它时才能删除,
 * 否则将导致未定义行为
 * @param event 事件标识符
 */
void event_delete(event_t event);

/**
 * @brief 标记事件为置位状态, 并唤醒等待队列中的线程, 如果auto_reset为true,
 * 那么只唤醒第1个线程, 并且将事件复位, 如果auto_reset为false,
 * 那么会唤醒所有线程, 事件保持置位状态
 * @param event 事件标识符
 */
void event_set(event_t event);

/**
 * @brief 标记事件为复位状态, 此函数不会唤醒任何线程
 * @param event 事件标识符
 */
void event_reset(event_t event);

/**
 * @brief 等待事件被置位
 * @param event 事件标识符
 */
void event_wait(event_t event);

/**
 * @brief 判断事件是否被置位
 * @param event 事件标识符
 * @retval 事件置位返回true, 否则返回false
 */
bool event_is_set(event_t event);

/**
 * @brief 定时等待事件置位, 如果等待时间超过timeout设定的时间则退出等待
 * @param event 事件标识符
 * @param timeout 等待时间（Tick）
 * @retval 剩余等待时间, 如果返回0则说明等待超时
 */
uint32_t event_timed_wait(event_t event, uint32_t timeout);

// 见event_reset
#define event_clear(event) event_reset(event)

/******************************************************************************
 * mutex
 ******************************************************************************/

/**
 * @brief 创建一个互斥锁对象, 支持递归锁
 * @retval 成功返回互斥锁标识符, 失败返回NULL
 */
mutex_t mutex_create(void);

/**
 * @brief 删除互斥锁对象, 并释放内存, 在没有线程使用它时才能删除, 否则线程将死锁
 * @param mutex 互斥锁标识符
 */
void mutex_delete(mutex_t mutex);

/**
 * @brief 将mutex指定的互斥锁标记为锁定状态, 如果mutex已被其它线程锁定,
 * 则调用线程将会被阻塞, 直到另一个线程释放这个互斥锁
 * @param mutex 互斥锁标识符
 */
void mutex_lock(mutex_t mutex);

/**
 * @brief 释放mutex标识的互斥锁, 如果有其它线程正在等待这个锁,
 * 则会唤醒优先级最高的那个线程
 * @param mutex 互斥锁标识符
 */
void mutex_unlock(mutex_t mutex);

/**
 * @brief 此函数是mutex_lock的非阻塞版本
 * @param mutex 互斥锁标识符
 * @retval 如果锁定成功则返回true, 失败则返回false
 */
bool mutex_try_lock(mutex_t mutex);

/**
 * @brief 此函数是mutex_lock的定时版本
 * @param mutex 互斥锁标识符
 * @param timeout 超时时间（Tick）
 * @retval 如果锁定成功则返回true, 失败则返回false
 */
bool mutex_timed_lock(mutex_t mutex, uint32_t timeout);

/******************************************************************************
 * condition variable
 ******************************************************************************/

/**
 * @brief 创建条件变量对象
 * @retval 成功返回条件变量标识符, 失败返回NULL
 */
cond_t cond_create(void);

/**
 * @brief 删除条件变量
 * @param cond 条件变量标识符
 */
void cond_delete(cond_t cond);

/**
 * @brief 唤醒一个被条件变量阻塞的线程, 如果没有线程被阻塞则此函数什么也不做
 * @param cond 条件变量标识符
 */
void cond_signal(cond_t cond);

/**
 * @brief 唤醒所有被条件变量阻塞的线程, 如果没有线程被阻塞则此函数什么也不做
 * @param cond 条件变量标识符
 */
void cond_broadcast(cond_t cond);

/**
 * @brief 阻塞线程, 并等待被条件变量唤醒
 * @param cond 条件变量标识符
 * @param mutex 互斥锁标识符
 */
void cond_wait(cond_t cond, mutex_t mutex);

/**
 * @brief 定时阻塞线程, 并等待被条件变量唤醒
 * @param cond 条件变量标识符
 * @param mutex 互斥锁标识符
 * @param timeout 超时时间（Tick）
 * @retval 剩余等待时间, 如果返回0则说明等待超时
 */
uint32_t cond_timed_wait(cond_t cond, mutex_t mutex, uint32_t timeout);

/******************************************************************************
 * hook
 ******************************************************************************/

#if KLITE_CFG_HOOK_ENABLE

/**
 * @brief 内核空闲钩子函数
 */
void kernel_hook_idle(void);

/**
 * @param time 当前滴答加数
 * @brief 内核滴答钩子函数
 */
void kernel_hook_tick(uint32_t time);

/**
 * @brief 内核堆内存分配错误钩子函数
 * @param size 内存大小
 */
void heap_hook_fault(uint32_t size);

#define HEAP_HOOK_OP_ALLOC 0
#define HEAP_HOOK_OP_FREE 1
#define HEAP_HOOK_OP_REALLOC 2

/**
 * @brief 内核堆内存操作钩子函数
 * @param addr1 内存地址1
 * @param addr2 内存地址2
 * @param size 内存大小
 * @param op 操作类型 0:分配 1:释放 2:重新分配
 */
void heap_hook_operation(void *addr1, void *addr2, uint32_t size, uint8_t op);

/**
 * @brief 内核线程创建钩子函数
 * @param thread 线程标识符
 */
void thread_hook_create(thread_t thread);

/**
 * @brief 内核线程删除/退出钩子函数
 * @param thread 线程标识符
 */
void thread_hook_delete(thread_t thread);

/**
 * @brief 内核线程优先级改变钩子函数
 * @param thread 线程标识符
 * @param prio 新的优先级
 */
void thread_hook_prio_change(thread_t thread, uint32_t prio);

/**
 * @brief 内核线程挂起钩子函数
 * @param thread 线程标识符
 */
void thread_hook_suspend(thread_t thread);

/**
 * @brief 内核线程恢复钩子函数
 * @param thread 线程标识符
 */
void thread_hook_resume(thread_t thread);

/**
 * @brief 内核线程切换钩子函数
 * @param from 当前线程标识符
 * @param to 目标线程标识符
 */
void thread_hook_switch(thread_t from, thread_t to);

/**
 * @brief 内核线程休眠钩子函数
 * @param thread 线程标识符
 * @param time 休眠时间
 */
void thread_hook_sleep(thread_t thread, uint32_t time);

#endif  // KLITE_CFG_HOOK_ENABLE

/******************************************************************************
 * event flags
 ******************************************************************************/

#if KLITE_CFG_OPT_EVENT_FLAGS

#define EVENT_FLAGS_WAIT_ANY 0x00
#define EVENT_FLAGS_WAIT_ALL 0x01
#define EVENT_FLAGS_AUTO_RESET 0x02

typedef struct event_flags *event_flags_t;

/**
 * @brief 创建一个事件组对象
 * @param 无
 * @retval 创建成功返回事件标识符，失败返回NULL
 */
event_flags_t event_flags_create(void);

/**
 * @brief 删除事件组对象，并释放内存
 * @param event 事件组标识符
 * @retval 无
 * @warning 在没有线程使用它时才能删除，否则会导致未定义行为
 */
void event_flags_delete(event_flags_t flags);

/**
 * @brief 置位bits指定的事件标志位，并唤醒等待队列中想要获取bits的线程
 * @param event 事件组标识符
 * @retval 无
 */
void event_flags_set(event_flags_t flags, uint32_t bits);

/**
 * @brief 清除bits指定的事件标志位，此函数不会唤醒任何线程
 * @param event 事件组标识符
 * @retval 无
 */
void event_flags_reset(event_flags_t flags, uint32_t bits);

/**
 * @brief 等待1个或多个事件标志位
 * @param event 事件组标识符
 * @param bits 想要等待的标志位
 * @param ops 等待标志位的行为
 *      EVENT_FLAGS_WAIT_ANY: 只要bits中的任意一位有效，函数立即返回；
 *      EVENT_FLAGS_WAIT_ALL: 只有bits中的所有位都有效，函数才能返回；
 *      EVENT_FLAGS_AUTO_RESET: 函数返回时自动清零获取到的标志位；
 * @retval 实际获取到的标志位状态
 */
uint32_t event_flags_wait(event_flags_t flags, uint32_t bits, uint32_t ops);

/**
 * @brief 等待1个或多个事件标志位，超时返回
 * @param event 事件组标识符
 * @param bits 想要等待的标志位
 * @param ops 等待标志位的行为
 *      EVENT_FLAGS_WAIT_ANY: 只要bits中的任意一位有效，函数立即返回；
 *      EVENT_FLAGS_WAIT_ALL: 只有bits中的所有位都有效，函数才能返回；
 *      EVENT_FLAGS_AUTO_RESET: 函数返回时自动清零获取到的标志位；
 * @param timeout 等待超时时间(毫秒)
 * @retval 实际获取到的标志位状态
 */
uint32_t event_flags_timed_wait(event_flags_t flags, uint32_t bits,
                                uint32_t ops, uint32_t timeout);

#endif  // KLITE_CFG_OPT_EVENT_FLAGS

/******************************************************************************
 * mailbox
 ******************************************************************************/

#if KLITE_CFG_OPT_MAILBOX

typedef struct mailbox *mailbox_t;

/**
 * @brief 创建消息邮箱
 * @param size 缓冲区长度
 * @retval 创建成功返回标识符，失败返回NULL
 * @note 消息邮箱按照FIFO机制取出消息。取出的消息长度和发送的消息长度一致
 * @note 如果输入的buf长度小于消息长度，则丢弃超出buf长度的部分内容
 */
mailbox_t mailbox_create(uint32_t size);

/**
 * @brief 删除消息邮箱，并释放内存.
 * @param mailbox 消息邮箱标识符
 * @retval 无
 */
void mailbox_delete(mailbox_t mailbox);

/**
 * @param mailbox 消息邮箱标识符
 * @retval 无
 * @brief 清空消息邮箱
 */
void mailbox_clear(mailbox_t mailbox);

/**
 * @brief 向消息邮箱发送消息
 * @param mailbox 消息邮箱标识符
 * @param buf 消息缓冲区
 * @param len 消息长度
 * @param timeout 超时时间
 * @retval 实际写入数据长度
 */
uint32_t mailbox_post(mailbox_t mailbox, void *buf, uint32_t len,
                      uint32_t timeout);

/**
 * @brief 从消息邮箱读取消息
 * @param mailbox 消息邮箱标识符
 * @param buf 读取缓冲区
 * @param len 读取缓冲区长度
 * @param timeout 超时时间
 * @retval 实际读取数据长度
 */
uint32_t mailbox_wait(mailbox_t mailbox, void *buf, uint32_t len,
                      uint32_t timeout);

#endif  // KLITE_CFG_OPT_MAILBOX

/******************************************************************************
 * mpool
 ******************************************************************************/

#if KLITE_CFG_OPT_MPOOL

typedef struct mpool *mpool_t;

/**
 * @brief 创建内存池
 * @param  block_size   内存块大小
 * @param  block_count  内存块数量
 * @retval 创建成功返回内存池标识符，失败返回NULL
 */
mpool_t mpool_create(uint32_t block_size, uint32_t block_count);

/**
 * @brief 删除内存池，并释放内存
 * @param mpool 内存池标识符
 * @retval 无
 */
void mpool_delete(mpool_t mpool);

/**
 * @brief 从内存池中分配内存块
 * @param mpool 内存池标识符
 * @retval 成功返回内存块指针，失败返回NULL
 */
void *mpool_alloc(mpool_t mpool);

/**
 * @brief 释放内存块
 * @param mpool 内存池标识符
 * @param block 内存块指针
 * @retval 无
 */
void mpool_free(mpool_t mpool, void *block);

#endif  // KLITE_CFG_OPT_MPOOL

/******************************************************************************
 * mqueue
 ******************************************************************************/

#if KLITE_CFG_OPT_MSG_QUEUE

typedef struct msg_queue *msg_queue_t;

/**
 * @brief 创建消息队列
 * @param item_size 消息大小
 * @param queue_depth 队列长度
 * @retval 创建成功返回消息队列标识符，失败返回NULL
 */
msg_queue_t msg_queue_create(uint32_t item_size, uint32_t queue_depth);

/**
 * @brief 删除消息队列，并释放内存
 * @param queue 消息队列标识符
 * @retval 无
 */
void msg_queue_delete(msg_queue_t queue);

/**
 * @brief 清空消息队列
 * @param queue 消息队列标识符
 * @retval 无
 */
void msg_queue_clear(msg_queue_t queue);

/**
 * @brief 发送消息到消息队列
 * @param queue 消息队列标识符
 * @param item 消息缓冲区
 * @param timeout 超时时间
 * @retval 发送成功返回true，失败返回false
 */
bool msg_queue_send(msg_queue_t queue, void *item, uint32_t timeout);

/**
 * @brief 从消息队列接收消息
 * @param queue 消息队列标识符
 * @param item 消息缓冲区
 * @param timeout 超时时间
 * @retval 接收成功返回true，失败返回false
 */
bool msg_queue_recv(msg_queue_t queue, void *item, uint32_t timeout);

#endif  // KLITE_CFG_OPT_MSG_QUEUE

/******************************************************************************
 * soft timer
 ******************************************************************************/

#if KLITE_CFG_OPT_SOFT_TIMER

typedef struct soft_timer *soft_timer_t;

/**
 * @brief 初始化软定时器
 * @param priority 定时器线程优先级
 * @retval 初始化成功返回true，失败返回false
 */
bool soft_timer_init(uint32_t priority);

/**
 * @brief 反初始化软定时器, 并释放内存
 * @retval 无
 */
void soft_timer_deinit(void);

/**
 * @brief 创建软定时器
 * @param handler 定时器回调函数
 * @param arg 回调函数参数
 * @retval 创建成功返回定时器标识符，失败返回NULL
 * @warning 应首先调用soft_timer_init()初始化软定时器
 */
soft_timer_t soft_timer_create(void (*handler)(void *), void *arg);

/**
 * @brief 删除软定时器
 * @param timer 定时器标识符
 * @retval 无
 */
void soft_timer_delete(soft_timer_t timer);

/**
 * @brief 启动软定时器
 * @param timer 定时器标识符
 * @param timeout 定时时间（毫秒）
 * @retval 无
 */
void soft_timer_start(soft_timer_t timer, uint32_t timeout);

/**
 * @brief 停止软定时器
 * @param timer 定时器标识符
 * @retval 无
 */
void soft_timer_stop(soft_timer_t timer);

#endif  // KLITE_CFG_OPT_SOFT_TIMER

/******************************************************************************
 * thread pool
 ******************************************************************************/

#if KLITE_CFG_OPT_THREAD_POOL

typedef struct thread_pool *thread_pool_t;

/**
 * @brief 创建线程池
 * @param worker_num 工作线程数量
 * @param worker_stack_size 工作线程栈大小
 * @param worker_priority 工作线程优先级
 * @retval 创建成功返回线程池标识符，失败返回NULL
 */
thread_pool_t thread_pool_create(uint8_t worker_num, uint32_t worker_stack_size,
                                 uint32_t worker_priority);

/**
 * @brief 删除线程池，并释放内存
 * @param pool 线程池标识符
 * @retval 无
 */
void thread_pool_delete(thread_pool_t pool);

/**
 * @brief 提交任务到线程池
 * @param pool 线程池标识符
 * @param process 任务处理函数
 * @param arg 任务参数
 * @retval 无
 */
void thread_pool_submit(thread_pool_t pool, void (*process)(void *arg),
                        void *arg);

/**
 * @brief 关闭线程池
 * @param  pool 线程池标识符
 */
void thread_pool_shutdown(thread_pool_t pool);

/**
 * @brief 等待线程池中的任务执行完成
 * @param pool 线程池标识符
 * @retval 无
 */
void thread_pool_join(thread_pool_t pool);

/**
 * @brief 获取线程池中的未完成任务数量
 * @param pool 线程池标识符
 * @retval 未完成任务数量
 */
uint16_t thread_pool_pending_task(thread_pool_t pool);

#endif  // KLITE_CFG_OPT_THREAD_POOL

/******************************************************************************
 * interface
 ******************************************************************************/

#if KLITE_CFG_INTERFACE_ENABLE

#include "klite_api.h"

#endif  // KLITE_CFG_OPT_INTERFACE

#endif  // __KLITE_H

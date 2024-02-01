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
#ifndef __KERNEL_H
#define __KERNEL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct tcb *thread_t;
typedef struct sem *sem_t;
typedef struct event *event_t;
typedef struct mutex *mutex_t;
typedef struct cond *cond_t;

/******************************************************************************
 * kernel
 ******************************************************************************/

#define KERNEL_FREQ 1000000  // 内核时基频率(赫兹)

/**
 * 参数：heap_addr 动态分配起始地址
 * 参数：heap_size 动态分配内存大小
 * 返回：无
 * 描述：用于内核初始化在调用内核初始化时需保证中断处于关闭状态,
 *      此函数只能执行一次, 在初始化内核之前不可调用内核其它函数。
 */
void kernel_init(void *heap_addr, uint32_t heap_size);

/**
 * 参数：无
 * 返回：无
 * 描述：用于启动内核, 此函正常情况下不会返回, 在调用之前至少要创建一个线程
 */
void kernel_start(void);

/**
 * 参数：无
 * 返回：KLite版本号, BIT[31:24]主版本号, BIT[23:16]次版本号, BIT[15:0]修订号
 */
uint32_t kernel_version(void);

/**
 * 参数：无
 * 返回：无
 * 描述：处理内核空闲事务, 回收线程资源
 *      此函数不会返回。必须单独创建一个线程来调用。
 */
void kernel_idle(void);

/**
 * 参数：无
 * 返回：系统空闲时间(毫秒)
 * 描述：获取系统从启动到现在空闲线程占用CPU的总时间
 *      可使用此函数和kernel_tick_count()一起计算CPU占用率
 */
uint32_t kernel_idle_time(void);

/**
 * 返回：无
 * 描述：此函数不是用户API, 而是由CPU的滴答时钟中断程序调用, 为系统提供时钟源。
 * 滴答定时器的周期决定了系统计时功能的细粒度, 主频较低的处理器推荐使用10ms周期,
 * 主频较高则使用1ms周期。
 */
void kernel_tick(uint32_t time);

/**
 * 参数：无
 * 返回：系统运行时间(毫秒)
 * 描述：此函数可以获取内核从启动到现在所运行的总时间
 */
uint32_t kernel_tick_count(void);

/**
 * 参数：无
 * 返回：系统运行时间(毫秒)
 * 描述：此函数可以获取内核从启动到现在所运行的总时间
 */
uint64_t kernel_tick_count64(void);

extern void *kernel_heap_addr;

/******************************************************************************
 * heap
 ******************************************************************************/
#define KERNEL_HEAP_MATHOD 3  // 1:bare 2:lwmem 3:heap4 4:heap5

/**
 * 参数：addr 动态分配起始地址
 * 参数：size 动态分配内存大小
 * 返回：无
 * 描述：用户在指定内存创建一个用于动态管理的堆内存
 */
void heap_create(void *addr, uint32_t size);

/**
 * 参数：size 申请内存大小
 * 返回：申请成功返回内存指针, 申请失败返回NULL
 * 描述：从堆中申请一段连续的内存, 功能和标准库的malloc()一样
 */
void *heap_alloc(uint32_t size);

/**
 * 参数：mem 内存指针
 * 返回：无
 * 描述：释放内存, 功能和标准库的free()一样
 */
void heap_free(void *mem);

/**
 * 参数：mem 内存指针
 * 参数：size 申请内存大小
 * 返回：申请成功返回内存指针, 申请失败返回NULL
 * 描述：重新分配内存大小, 功能和标准库的realloc()一样
 */
void *heap_realloc(void *mem, uint32_t size);

/**
 * 参数：used 已使用内存大小
 * 参数：free 空闲内存大小
 * 返回：无
 * 描述：获取堆内存使用情况
 */
void heap_usage(uint32_t *used, uint32_t *free);

/**
 * 参数：无
 * 返回：堆内存使用率
 * 描述：获取堆内存使用率(0.0~1.0)
 */
float heap_usage_percent(void);

/**
 * 参数：无
 * 返回：无
 * 描述：任意堆内存操作失败时会调用此函数, 用户可以在此函数中添加断点或者日志
 */
void heap_fault_handler(void);

/******************************************************************************
 * thread
 ******************************************************************************/
#define THREAD_PRIORITY_HIGHEST 7
#define THREAD_PRIORITY_HIGHER 6
#define THREAD_PRIORITY_HIGH 5
#define THREAD_PRIORITY_NORMAL 4
#define THREAD_PRIORITY_LOW 3
#define THREAD_PRIORITY_LOWER 2
#define THREAD_PRIORITY_LOWEST 1
#define THREAD_PRIORITY_IDLE 0

/**
 * 参数：entry 线程入口函数
 * 参数：arg 线程入口函数的参数
 * 参数：stack_size 线程的栈大小（字节）, 为0则使用系统默认值(1024字节)
 * 返回：成功返回线程句柄, 失败返回NULL
 * 描述：创建新线程, 并加入就绪队列, 默认优先级为THREAD_PRIORITY_NORMAL
 */
thread_t thread_create(void (*entry)(void *), void *arg, uint32_t stack_size);

/**
 * 参数：thread 被删除的线程标识符
 * 返回：无
 * 描述：删除线程, 并释放内存
 * 该函数不能用来结束当前线程, 如果想要结束当前线程
 * 请使用thread_exit()或直接使用return退出主循环
 */
void thread_delete(thread_t thread);

/**
 * 参数：无
 * 返回：无
 * 描述：使当前线程立即释放CPU控制权, 并进入就绪队列
 */
void thread_yield(void);

/**
 * 参数：time 睡眠时间(取决于时钟源粒度)
 * 返回：无
 * 描述：将当前线程休眠一段时间, 释放CPU控制权
 */
void thread_sleep(uint32_t time);

/**
 * 参数：无
 * 返回：无
 * 描述：退出当前线程, 此函数不会立即释放线程占用的内存, 需等待系统空闲时释放
 */
void thread_exit(void);

/**
 * 参数：无
 * 返回：调用线程的标识符
 * 描述：用于获取当前线程标识符
 */
thread_t thread_self(void);

/**
 * 参数：thread 线程标识符
 * 返回：指定线程运行时间（毫秒）
 * 描述：获取线程自创建以来所占用CPU的时间
 * 在休眠期间的时间不计算在内, 可以使用此函数来监控某个线程的CPU占用率
 */
uint32_t thread_time(thread_t thread);

/**
 * 参数：thread 线程标识符
 * 参数：prio 新的优先级
 *      THREAD_PRIORITY_HIGHEST 最高优先级
 *      THREAD_PRIORITY_HIGHER 更高优先级
 *      THREAD_PRIORITY_HIGH 高优先级
 *      THREAD_PRIORITY_NORMAL 默认优先级
 *      THREAD_PRIORITY_LOW 低优先级
 *      THREAD_PRIORITY_LOWER 更低优先级
 *      THREAD_PRIORITY_LOWEST 最低优先级
 *      THREAD_PRIORITY_IDLE 空闲优先级
 * 返回：无
 * 描述：重新设置线程优先级, 立即生效。
 * 不允许在内核启动前修改线程优先级。
 */
void thread_set_priority(thread_t thread, uint32_t prio);

/**
 * 参数：thread 线程标识符
 * 返回：线程优先级
 * 描述：获取线程优先级
 */
uint32_t thread_get_priority(thread_t thread);
// #define sleep(x) thread_sleep(x)

/******************************************************************************
 * semaphore
 ******************************************************************************/

/**
 * 参数：value 信号量初始值
 * 返回：成功返回信号量标识符, 失败返回NULL
 * 描述：创建信号量对象
 */
sem_t sem_create(uint32_t value);

/**
 * 参数：sem 信号量标识符
 * 返回：无
 * 描述：删除对象, 并释放内存在没有线程使用它时才能删除, 否则将导致未定义行为
 */
void sem_delete(sem_t sem);

/**
 * 参数：sem 信号量标识符
 * 返回：无
 * 描述：信号量计数值加1, 如果有线程在等待信号量, 此函数会唤醒优先级最高的线程
 */
void sem_post(sem_t sem);

/**
 * 参数：sem 信号量标识符
 * 返回：无
 * 描述：等待信号量, 信号量计数值减1, 如果当前信号量计数值为0, 则线程阻塞,
 * 直到计数值大于0
 */
void sem_wait(sem_t sem);

/**
 * 参数：sem 信号量标识符
 * 参数：timeout 超时时间（毫秒）
 * 返回：剩余等待时间, 如果返回0则说明等待超时
 * 描述：定时等待信号量, 并将信号量计数值减1, 如果当前信号量计数值为0,
 * 则线程阻塞, 直到计数值大于0, 或者阻塞时间超过timeout指定的时间
 */
uint32_t sem_timed_wait(sem_t sem, uint32_t timeout);

/**
 * 参数：sem 信号量标识符
 * 返回：信号量计数值
 * 描述：获取信号量计数值
 */
uint32_t sem_value(sem_t sem);

/******************************************************************************
 * event
 ******************************************************************************/

/**
 * 参数：auto_reset 是否自动复位事件
 * 返回：创建成功返回事件标识符, 失败返回NULL
 * 描述：创建一个事件对象, 当auto_reset为true时事件会在传递成功后自动复位
 */
event_t event_create(bool auto_reset);

/**
 * 参数：event 事件标识符
 * 返回：无
 * 描述：删除事件对象, 并释放内存, 在没有线程使用它时才能删除,
 * 否则将导致未定义行为
 */
void event_delete(event_t event);

/**
 * 参数：event 事件标识符
 * 返回：无
 * 描述：标记事件为置位状态, 并唤醒等待队列中的线程, 如果auto_reset为true,
 * 那么只唤醒第1个线程, 并且将事件复位, 如果auto_reset为false,
 * 那么会唤醒所有线程, 事件保持置位状态
 */
void event_set(event_t event);

/**
 * 参数：event 事件标识符
 * 返回：无
 * 描述：标记事件为复位状态, 此函数不会唤醒任何线程
 */
void event_reset(event_t event);

/**
 * 参数：event 事件标识符
 * 返回：无
 * 描述：等待事件被置位
 */
void event_wait(event_t event);

/**
 * 参数：event 事件标识符
 * 参数：timeout 等待时间（毫秒）
 * 返回：剩余等待时间, 如果返回0则说明等待超时
 * 描述：定时等待事件置位, 如果等待时间超过timeout设定的时间则退出等待
 */
uint32_t event_timed_wait(event_t event, uint32_t timeout);

// 见event_reset
#define event_clear(event) event_reset(event)

/******************************************************************************
 * mutex
 ******************************************************************************/

/**
 * 参数：无
 * 返回：成功返回互斥锁标识符, 失败返回NULL
 * 描述：创建一个互斥锁对象, 支持递归锁
 */
mutex_t mutex_create(void);

/**
 * 参数：mutex 互斥锁标识符
 * 返回：无
 * 描述：删除互斥锁对象, 并释放内存, 在没有线程使用它时才能删除, 否则线程将死锁
 */
void mutex_delete(mutex_t mutex);

/**
 * 参数：mutex 互斥锁标识符
 * 返回：无
 * 描述：将mutex指定的互斥锁标记为锁定状态, 如果mutex已被其它线程锁定,
 * 则调用线程将会被阻塞, 直到另一个线程释放这个互斥锁
 */
void mutex_lock(mutex_t mutex);

/**
 * 参数：mutex 互斥锁标识符
 * 返回：无
 * 描述：释放mutex标识的互斥锁, 如果有其它线程正在等待这个锁,
 * 则会唤醒优先级最高的那个线程
 */
void mutex_unlock(mutex_t mutex);

/**
 * 参数：mutex 互斥锁标识符
 * 返回：如果锁定成功则返回true, 失败则返回false
 * 描述：此函数是mutex_lock的非阻塞版本
 */
bool mutex_try_lock(mutex_t mutex);

/******************************************************************************
 * condition variable
 ******************************************************************************/

/**
 * 参数：无
 * 返回：成功返回条件变量标识符, 失败返回NULL
 * 描述：创建条件变量对象
 */
cond_t cond_create(void);

/**
 * 参数：cond 条件变量标识符
 * 返回：无
 * 描述：删除条件变量
 */
void cond_delete(cond_t cond);

/**
 * 参数：cond 条件变量标识符
 * 返回：无
 * 描述：唤醒一个被条件变量阻塞的线程, 如果没有线程被阻塞则此函数什么也不做
 */
void cond_signal(cond_t cond);

/**
 * 参数：cond 条件变量标识符
 * 返回：无
 * 描述：唤醒所有被条件变量阻塞的线程, 如果没有线程被阻塞则此函数什么也不做
 */
void cond_broadcast(cond_t cond);

/**
 * 参数：cond 条件变量标识符
 * 参数：mutex 互斥锁标识符
 * 返回：无
 * 描述：阻塞线程, 并等待被条件变量唤醒
 */
void cond_wait(cond_t cond, mutex_t mutex);

/**
 * 参数：cond 条件变量标识符
 * 参数：mutex 互斥锁标识符
 * 参数：timeout 超时时间（毫秒）
 * 返回：剩余等待时间, 如果返回0则说明等待超时
 * 功能：定时阻塞线程, 并等待被条件变量唤醒
 */
uint32_t cond_timed_wait(cond_t cond, mutex_t mutex, uint32_t timeout);

#endif

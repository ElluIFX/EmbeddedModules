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
#ifndef __KLITE_INTERNAL_H
#define __KLITE_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Compatible with armcc5 armcc6 gcc icc */
#if defined(__GNUC__) || (__ARMCC_VERSION >= 6100100)
#define __weak __attribute__((weak))
#endif

#define STACK_MAGIC_VALUE 0xCC

struct tcb_list {
  struct tcb_node *head;
  struct tcb_node *tail;
};

struct tcb_node {
  struct tcb_node *prev;
  struct tcb_node *next;
  struct tcb *tcb;
};

struct tcb {
  void *stack;                  // 栈基地址
  uint32_t stack_size;          // 栈大小
  void (*entry)(void *);        // 线程入口
  uint32_t prio;                // 线程优先级
  uint32_t time;                // 线程运行时间
  uint32_t timeout;             // 睡眠超时时间
  struct tcb_list *list_sched;  // 当前所处调度队列
  struct tcb_list *list_wait;   // 当前所处等待队列
  struct tcb_node node_sched;   // 调度队列节点
  struct tcb_node node_wait;    // 等待队列节点
  struct tcb_node node_manage;  // 管理节点
};

extern struct tcb *sched_tcb_now;
extern struct tcb *sched_tcb_next;

extern volatile uint32_t sched_susp_nesting;

// 平台实现: 进入临界区, 并执行需要在内核初始化前执行的操作
void cpu_sys_init(void);

// 平台实现: 执行内核初始化后的操作, 退出临界区, 退出后系统应直接进入调度
void cpu_sys_start(void);

// 平台实现: 系统空闲回调
// @param time: 系统空闲时间, 单位tick
void cpu_sys_sleep(uint32_t time);

// 平台实现: 触发PendSV, 进行上下文切换
void cpu_contex_switch(void);

// 平台实现: 初始化线程上下文
// @param stack_base: 栈基地址
// @param stack_top: 栈顶地址
// @param entry: 线程入口地址
// @param arg: 线程参数
// @param exit: 线程结束回调地址
void *cpu_contex_init(void *stack_base, void *stack_top, void *entry, void *arg,
                      void *exit);

// 平台实现: 进入临界区
void cpu_enter_critical(void);

// 平台实现: 退出临界区
void cpu_leave_critical(void);

// 内核空闲线程
void kernel_idle_thread(void *args);

// 清理待删除线程
void thread_clean_up(void);

// 初始化线程调度器
void sched_init(void);

// 线程调度器空闲处理
// 如果有线程就绪, 则调度线程
// 如果没有线程就绪, 则进入系统空闲
void sched_idle(void);

// 线程调度器时钟处理
// 如果有线程超时, 则唤醒线程
// @param time: 时钟增量
void sched_timing(uint32_t time);

// 执行线程切换
void sched_switch(void);

// 尝试切换线程, 并将当前线程加入就绪队列
// @param round_robin: 允许同优先级时间片轮转
void sched_preempt(bool round_robin);

// 重置线程优先级
// @param tcb: 线程控制块
// @param prio: 优先级
void sched_tcb_reset_prio(struct tcb *tcb, uint32_t prio);

// 从调度器移除线程
// @param tcb: 线程控制块
void sched_tcb_remove(struct tcb *tcb);

// 将线程加入就绪队列
// @param tcb: 线程控制块
void sched_tcb_ready(struct tcb *tcb);

// 将线程加入睡眠队列
// @param tcb: 线程控制块
// @param timeout: 睡眠时间
void sched_tcb_sleep(struct tcb *tcb, uint32_t timeout);

// 将线程加入等待队列
// @param tcb: 线程控制块
// @param list: 等待队列
void sched_tcb_wait(struct tcb *tcb, struct tcb_list *list);

// 将线程同时加入等待队列和睡眠队列
// @param tcb: 线程控制块
// @param list: 等待队列
// @param timeout: 睡眠时间(等待超时时间)
void sched_tcb_timed_wait(struct tcb *tcb, struct tcb_list *list,
                          uint32_t timeout);

// 尝试唤醒等待队列中的线程
// @param list: 等待队列
// @return: 被唤醒的线程控制块, NULL表示无等待线程
struct tcb *sched_tcb_wake_from(struct tcb_list *list);

#endif

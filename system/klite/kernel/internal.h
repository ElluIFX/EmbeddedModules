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
#ifndef __INTERNAL_H
#define __INTERNAL_H

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
  void *stack;
  uint32_t stack_size;
  void (*entry)(void *);
  uint32_t prio;
  uint32_t time;
  uint32_t timeout;
  struct tcb_list *list_sched;
  struct tcb_list *list_wait;
  struct tcb_node node_sched;
  struct tcb_node node_wait;
};

extern struct tcb *sched_tcb_now;
extern struct tcb *sched_tcb_next;
extern volatile uint32_t kernel_sys_nesting;

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

void sched_init(void);
void sched_idle(void);
void sched_timing(uint32_t time);
void sched_switch(void);
void sched_preempt(bool round_robin);
void sched_tcb_reset(struct tcb *tcb, uint32_t prio);
void sched_tcb_remove(struct tcb *tcb);
void sched_tcb_ready(struct tcb *tcb);
void sched_tcb_sleep(struct tcb *tcb, uint32_t timeout);
void sched_tcb_wait(struct tcb *tcb, struct tcb_list *list);
void sched_tcb_timed_wait(struct tcb *tcb, struct tcb_list *list,
                          uint32_t timeout);
struct tcb *sched_tcb_wake_from(struct tcb_list *list);

#endif

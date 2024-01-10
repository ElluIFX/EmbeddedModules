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
#ifndef __EVENT_FLAGS_H
#define __EVENT_FLAGS_H

#include "kernel.h"

#define EVENT_FLAGS_WAIT_ANY 0x00
#define EVENT_FLAGS_WAIT_ALL 0x01
#define EVENT_FLAGS_AUTO_RESET 0x02

typedef struct event_flags *event_flags_t;

/**
 * 参数：无
 * 返回：创建成功返回事件标识符，失败返回NULL
 * 描述：创建一个事件组对象
 */
event_flags_t event_flags_create(void);

/**
 * 参数：event 事件组标识符
 * 返回：无
 * 描述：删除事件组对象，并释放内存，在没有线程使用它时才能删除，否则会导致未定义行为
 */
void event_flags_delete(event_flags_t flags);

/**
 * 参数：event 事件组标识符
 * 返回：无
 * 描述：置位bits指定的事件标志位，并唤醒等待队列中想要获取bits的线程
 */
void event_flags_set(event_flags_t flags, uint32_t bits);

/**
 * 参数：event 事件组标识符
 * 返回：无
 * 描述：清除bits指定的事件标志位，此函数不会唤醒任何线程
 */
void event_flags_reset(event_flags_t flags, uint32_t bits);

/**
 * 参数：event 事件组标识符
 * 参数：bits 想要等待的标志位
 * 参数：ops 等待标志位的行为
 *      EVENT_FLAGS_WAIT_ANY: 只要bits中的任意一位有效，函数立即返回；
 *      EVENT_FLAGS_WAIT_ALL: 只有bits中的所有位都有效，函数才能返回；
 *      EVENT_FLAGS_AUTO_RESET: 函数返回时自动清零获取到的标志位；
 * 返回：实际获取到的标志位状态
 * 描述：等待1个或多个事件标志位
 */
uint32_t event_flags_wait(event_flags_t flags, uint32_t bits, uint32_t ops);

/**
 * 参数：event 事件组标识符
 * 参数：bits 想要等待的标志位
 * 参数：ops 等待标志位的行为
 *      EVENT_FLAGS_WAIT_ANY: 只要bits中的任意一位有效，函数立即返回；
 *      EVENT_FLAGS_WAIT_ALL: 只有bits中的所有位都有效，函数才能返回；
 *      EVENT_FLAGS_AUTO_RESET: 函数返回时自动清零获取到的标志位；
 * 参数：timeout 等待超时时间(毫秒)
 * 返回：实际获取到的标志位状态
 * 描述：等待1个或多个事件标志位，超时返回
 */
uint32_t event_flags_timed_wait(event_flags_t flags, uint32_t bits,
                                uint32_t ops, uint32_t timeout);

#endif

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
#ifndef __MAILBOX_H
#define __MAILBOX_H

#include "kernel.h"

typedef struct mailbox *mailbox_t;

/**
 * @param size 缓冲区长度
 * @retval 创建成功返回标识符，失败返回NULL
 * @brief 创建消息邮箱
 * @note 消息邮箱按照FIFO机制取出消息。取出的消息长度和发送的消息长度一致
 * @note 如果输入的buf长度小于消息长度，则丢弃超出buf长度的部分内容
 */
mailbox_t mailbox_create(uint32_t size);

/**
 * @param mailbox 消息邮箱标识符
 * @retval 无
 * @brief 删除消息邮箱，并释放内存.
 */
void mailbox_delete(mailbox_t mailbox);

/**
 * @param mailbox 消息邮箱标识符
 * @retval 无
 * @brief 清空消息邮箱
 */
void mailbox_clear(mailbox_t mailbox);

/**
 * @param mailbox 消息邮箱标识符
 * @param buf 消息缓冲区
 * @param len 消息长度
 * @param timeout 超时时间
 * @retval 实际写入数据长度
 * @brief 向消息邮箱发送消息
 */
uint32_t mailbox_post(mailbox_t mailbox, void *buf, uint32_t len,
                      uint32_t timeout);

/**
 * @param mailbox 消息邮箱标识符
 * @param buf 读取缓冲区
 * @param len 读取缓冲区长度
 * @param timeout 超时时间
 * @retval 实际读取数据长度
 * @brief 从消息邮箱读取消息
 */
uint32_t mailbox_wait(mailbox_t mailbox, void *buf, uint32_t len,
                      uint32_t timeout);

#endif

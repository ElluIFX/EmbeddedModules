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

event_flags_t event_flags_create(void);
void event_flags_delete(event_flags_t flags);
void event_flags_set(event_flags_t flags, uint32_t bits);
void event_flags_reset(event_flags_t flags, uint32_t bits);
uint32_t event_flags_wait(event_flags_t flags, uint32_t bits, uint32_t ops);
uint32_t event_flags_timed_wait(event_flags_t flags, uint32_t bits,
                                uint32_t ops, uint32_t timeout);

#endif

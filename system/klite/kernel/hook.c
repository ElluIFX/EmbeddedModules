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
#include "internal.h"
#include "kernel.h"
#if KERNEL_CFG_HOOK_ENABLE
#include "log.h"

__weak void kernel_hook_idle(void) {}

__weak void kernel_hook_tick(uint32_t time) { (void)time; }

__weak void kernel_hook_heap_fault(uint32_t size) {
  LOG_ERROR("heap alloc failed: size=%d", size);
}

__weak void kernel_hook_heap_operation(void *addr1, void *addr2, uint32_t size,
                                       uint8_t op) {
  (void)addr1;
  (void)addr2;
  (void)size;
  (void)op;
}

__weak void kernel_hook_thread_create(thread_t thread) { (void)thread; }

__weak void kernel_hook_thread_delete(thread_t thread) { (void)thread; }

__weak void kernel_hook_thread_suspend(thread_t thread) { (void)thread; }

__weak void kernel_hook_thread_resume(thread_t thread) { (void)thread; }

__weak void kernel_hook_thread_switch(thread_t from, thread_t to) {
  (void)from;
  (void)to;
}

__weak void kernel_hook_thread_sleep(thread_t thread, uint32_t time) {
  (void)thread;
  (void)time;
}

#endif  // KERNEL_CFG_HOOK_ENABLE

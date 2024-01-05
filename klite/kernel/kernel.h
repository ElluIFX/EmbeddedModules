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
void kernel_init(void *heap_addr, uint32_t heap_size);
void kernel_start(void);
uint32_t kernel_version(void);
void kernel_idle(void);
uint32_t kernel_idle_time(void);
void kernel_tick(uint32_t time);
uint32_t kernel_tick_count(void);
uint64_t kernel_tick_count64(void);

extern void *kernel_heap_addr;

/******************************************************************************
 * heap
 ******************************************************************************/
#define HEAP_USE_LWMEM 1

void heap_create(void *addr, uint32_t size);
void *heap_alloc(uint32_t size);
void *heap_realloc(void *mem, uint32_t size);
void heap_free(void *mem);
void heap_usage(uint32_t *used, uint32_t *free);
float heap_usage_percent(void);

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

thread_t thread_create(void (*entry)(void *), void *arg, uint32_t stack_size);
void thread_delete(thread_t thread);
void thread_yield(void);
void thread_sleep(uint32_t time);
void thread_exit(void);
thread_t thread_self(void);
uint32_t thread_time(thread_t thread);
void thread_set_priority(thread_t thread, uint32_t prio);
uint32_t thread_get_priority(thread_t thread);
#define sleep(x) thread_sleep(x)

/******************************************************************************
 * semaphore
 ******************************************************************************/
sem_t sem_create(uint32_t value);
void sem_delete(sem_t sem);
void sem_post(sem_t sem);
void sem_wait(sem_t sem);
uint32_t sem_timed_wait(sem_t sem, uint32_t timeout);
uint32_t sem_value(sem_t sem);

/******************************************************************************
 * event
 ******************************************************************************/
event_t event_create(bool auto_reset);
void event_delete(event_t event);
void event_set(event_t event);
void event_reset(event_t event);
void event_wait(event_t event);
uint32_t event_timed_wait(event_t event, uint32_t timeout);
#define event_clear(event) event_reset(event)

/******************************************************************************
 * mutex
 ******************************************************************************/
mutex_t mutex_create(void);
void mutex_delete(mutex_t mutex);
void mutex_lock(mutex_t mutex);
void mutex_unlock(mutex_t mutex);
bool mutex_try_lock(mutex_t mutex);

/******************************************************************************
 * condition variable
 ******************************************************************************/
cond_t cond_create(void);
void cond_delete(cond_t cond);
void cond_signal(cond_t cond);
void cond_broadcast(cond_t cond);
void cond_wait(cond_t cond, mutex_t mutex);
uint32_t cond_timed_wait(cond_t cond, mutex_t mutex, uint32_t timeout);

#endif

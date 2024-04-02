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
#include "klite.h"

#if KLITE_CFG_OPT_RWLOCK

#include <string.h>

struct rwlock {
  mutex_t mutex;
  cond_t read;
  cond_t write;
  uint32_t read_wait_count;   // 等待读锁数量
  uint32_t write_wait_count;  // 等待写锁数量
  int32_t rw_count;           // -1:写锁 0:无锁 >0:读锁数量
};

rwlock_t rwlock_create(void) {
  struct rwlock *rwlock;
  rwlock = heap_alloc(sizeof(struct rwlock));
  if (rwlock != NULL) {
    memset(rwlock, 0, sizeof(struct rwlock));
  } else {
    return NULL;
  }

  rwlock->mutex = mutex_create();
  if (rwlock->mutex == NULL) {
    heap_free(rwlock);
    return NULL;
  }

  rwlock->read = cond_create();
  if (rwlock->read == NULL) {
    mutex_delete(rwlock->mutex);
    heap_free(rwlock);
    return NULL;
  }

  rwlock->write = cond_create();
  if (rwlock->write == NULL) {
    cond_delete(rwlock->read);
    mutex_delete(rwlock->mutex);
    heap_free(rwlock);
    return NULL;
  }

  return (rwlock_t)rwlock;
}

void rwlock_delete(rwlock_t rwlock) { heap_free(rwlock); }

void rwlock_read_lock(rwlock_t rwlock) {
  mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count > 0 || rwlock->rw_count < 0) {
    rwlock->read_wait_count++;
    cond_wait(rwlock->read, rwlock->mutex);
    rwlock->read_wait_count--;
  }
  rwlock->rw_count++;
  mutex_unlock(rwlock->mutex);
}

void rwlock_read_unlock(rwlock_t rwlock) {
  mutex_lock(rwlock->mutex);
  rwlock->rw_count--;
  if (rwlock->rw_count == 0 && rwlock->write_wait_count > 0) {
    cond_signal(rwlock->write);
  }
  mutex_unlock(rwlock->mutex);
}

void rwlock_write_lock(rwlock_t rwlock) {
  mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count > 0 || rwlock->rw_count != 0) {
    rwlock->write_wait_count++;
    cond_wait(rwlock->write, rwlock->mutex);
    rwlock->write_wait_count--;
  }
  rwlock->rw_count = -1;
  mutex_unlock(rwlock->mutex);
}

void rwlock_write_unlock(rwlock_t rwlock) {
  mutex_lock(rwlock->mutex);
  rwlock->rw_count = 0;
  if (rwlock->read_wait_count > 0) {  // 优先唤醒读锁
    cond_broadcast(rwlock->read);
  } else if (rwlock->write_wait_count > 0) {  // 唤醒写锁
    cond_signal(rwlock->write);
  }
  mutex_unlock(rwlock->mutex);
}

bool rwlock_try_read_lock(rwlock_t rwlock) {
  bool ret = false;
  mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count == 0 && rwlock->rw_count >= 0) {
    rwlock->rw_count++;
    ret = true;
  }
  mutex_unlock(rwlock->mutex);
  return ret;
}

bool rwlock_try_write_lock(rwlock_t rwlock) {
  bool ret = false;
  mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count == 0 && rwlock->rw_count == 0) {
    rwlock->rw_count = -1;
    ret = true;
  }
  mutex_unlock(rwlock->mutex);
  return ret;
}

klite_tick_t rwlock_timed_read_lock(rwlock_t rwlock, klite_tick_t timeout) {
  klite_tick_t ret = 0;
  mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count > 0 || rwlock->rw_count < 0) {
    rwlock->read_wait_count++;
    ret = cond_timed_wait(rwlock->read, rwlock->mutex, timeout);
    rwlock->read_wait_count--;
  }
  if (ret != 0) {
    rwlock->rw_count++;
  }
  mutex_unlock(rwlock->mutex);
  return ret;
}

klite_tick_t rwlock_timed_write_lock(rwlock_t rwlock, klite_tick_t timeout) {
  klite_tick_t ret = 0;
  mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count > 0 || rwlock->rw_count != 0) {
    rwlock->write_wait_count++;
    ret = cond_timed_wait(rwlock->write, rwlock->mutex, timeout);
    rwlock->write_wait_count--;
  }
  if (ret != 0) {
    rwlock->rw_count = -1;
  }
  mutex_unlock(rwlock->mutex);
  return ret;
}

#endif

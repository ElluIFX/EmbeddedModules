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
#include "klite_internal.h"

#if KLITE_CFG_OPT_RWLOCK

#include <string.h>

kl_rwlock_t kl_rwlock_create(void) {
  struct kl_rwlock *rwlock;
  rwlock = kl_heap_alloc(sizeof(struct kl_rwlock));
  if (rwlock != NULL) {
    memset(rwlock, 0, sizeof(struct kl_rwlock));
  } else {
    return NULL;
  }

  rwlock->mutex = kl_mutex_create();
  if (rwlock->mutex == NULL) {
    kl_heap_free(rwlock);
    return NULL;
  }

  rwlock->read = kl_cond_create();
  if (rwlock->read == NULL) {
    kl_mutex_delete(rwlock->mutex);
    kl_heap_free(rwlock);
    return NULL;
  }

  rwlock->write = kl_cond_create();
  if (rwlock->write == NULL) {
    kl_cond_delete(rwlock->read);
    kl_mutex_delete(rwlock->mutex);
    kl_heap_free(rwlock);
    return NULL;
  }

  return (kl_rwlock_t)rwlock;
}

void kl_rwlock_delete(kl_rwlock_t rwlock) { kl_heap_free(rwlock); }

void kl_rwlock_read_lock(kl_rwlock_t rwlock) {
  kl_mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count > 0 || rwlock->rw_count < 0) {
    rwlock->read_wait_count++;
    kl_cond_wait(rwlock->read, rwlock->mutex);
    rwlock->read_wait_count--;
  }
  rwlock->rw_count++;
  kl_mutex_unlock(rwlock->mutex);
}

void kl_rwlock_read_unlock(kl_rwlock_t rwlock) {
  if (rwlock->rw_count <= 0) return;
  kl_mutex_lock(rwlock->mutex);
  rwlock->rw_count--;
  if (rwlock->rw_count == 0 && rwlock->write_wait_count > 0) {
    kl_cond_signal(rwlock->write);
  }
  kl_mutex_unlock(rwlock->mutex);
}

void kl_rwlock_write_lock(kl_rwlock_t rwlock) {
  kl_mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count > 0 || rwlock->rw_count != 0) {
    rwlock->write_wait_count++;
    kl_cond_wait(rwlock->write, rwlock->mutex);
    rwlock->write_wait_count--;
  }
  rwlock->rw_count = -1;
  kl_mutex_unlock(rwlock->mutex);
}

void kl_rwlock_write_unlock(kl_rwlock_t rwlock) {
  if (rwlock->rw_count >= 0) return;
  kl_mutex_lock(rwlock->mutex);
  rwlock->rw_count = 0;
  if (rwlock->read_wait_count > 0) {  // 优先唤醒读锁
    kl_cond_broadcast(rwlock->read);
  } else if (rwlock->write_wait_count > 0) {  // 唤醒写锁
    kl_cond_signal(rwlock->write);
  }
  kl_mutex_unlock(rwlock->mutex);
}

void kl_rwlock_unlock(kl_rwlock_t rwlock) {
  if (rwlock->rw_count < 0) {
    kl_rwlock_write_unlock(rwlock);
  } else if (rwlock->rw_count > 0) {
    kl_rwlock_read_unlock(rwlock);
  }
}

bool kl_rwlock_try_read_lock(kl_rwlock_t rwlock) {
  bool ret = false;
  kl_mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count == 0 && rwlock->rw_count >= 0) {
    rwlock->rw_count++;
    ret = true;
  }
  kl_mutex_unlock(rwlock->mutex);
  return ret;
}

bool kl_rwlock_try_write_lock(kl_rwlock_t rwlock) {
  bool ret = false;
  kl_mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count == 0 && rwlock->rw_count == 0) {
    rwlock->rw_count = -1;
    ret = true;
  }
  kl_mutex_unlock(rwlock->mutex);
  return ret;
}

kl_tick_t kl_rwlock_timed_read_lock(kl_rwlock_t rwlock, kl_tick_t timeout) {
  kl_tick_t ret = 0;
  kl_mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count > 0 || rwlock->rw_count < 0) {
    rwlock->read_wait_count++;
    ret = kl_cond_timed_wait(rwlock->read, rwlock->mutex, timeout);
    rwlock->read_wait_count--;
  }
  if (ret != 0) {
    rwlock->rw_count++;
  }
  kl_mutex_unlock(rwlock->mutex);
  return ret;
}

kl_tick_t kl_rwlock_timed_write_lock(kl_rwlock_t rwlock, kl_tick_t timeout) {
  kl_tick_t ret = 0;
  kl_mutex_lock(rwlock->mutex);
  if (rwlock->write_wait_count > 0 || rwlock->rw_count != 0) {
    rwlock->write_wait_count++;
    ret = kl_cond_timed_wait(rwlock->write, rwlock->mutex, timeout);
    rwlock->write_wait_count--;
  }
  if (ret != 0) {
    rwlock->rw_count = -1;
  }
  kl_mutex_unlock(rwlock->mutex);
  return ret;
}

#endif

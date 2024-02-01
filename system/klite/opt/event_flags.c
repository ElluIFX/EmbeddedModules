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
#include "event_flags.h"

#include <string.h>

#include "kernel.h"

struct event_flags {
  mutex_t mutex;
  cond_t cond;
  uint32_t bits;
};

event_flags_t event_flags_create(void) {
  struct event_flags *flags;
  flags = heap_alloc( sizeof(struct event_flags));
  if (flags != NULL) {
    memset(flags, 0, sizeof(struct event_flags));
    flags->mutex = mutex_create();
    if (flags->mutex == NULL) {
      heap_free( flags);
      return NULL;
    }
    flags->cond = cond_create();
    if (flags->cond == NULL) {
      mutex_delete(flags->mutex);
      heap_free( flags);
      return NULL;
    }
  }
  return flags;
}

void event_flags_delete(event_flags_t flags) {
  mutex_delete(flags->mutex);
  cond_delete(flags->cond);
  heap_free( flags);
}

void event_flags_set(event_flags_t flags, uint32_t bits) {
  mutex_lock(flags->mutex);
  flags->bits |= bits;
  mutex_unlock(flags->mutex);
  cond_broadcast(flags->cond);
}

void event_flags_reset(event_flags_t flags, uint32_t bits) {
  mutex_lock(flags->mutex);
  flags->bits &= ~bits;
  mutex_unlock(flags->mutex);
}

static uint32_t try_wait_bits(event_flags_t flags, uint32_t bits,
                              uint32_t ops) {
  uint32_t cmp;
  uint32_t wait_all;
  cmp = flags->bits & bits;
  wait_all = ops & EVENT_FLAGS_WAIT_ALL;
  if ((wait_all && (cmp == bits)) || ((!wait_all) && (cmp != 0))) {
    if (ops & EVENT_FLAGS_AUTO_RESET) {
      flags->bits &= ~bits;
    }
    return cmp;
  }
  return 0;
}

uint32_t event_flags_wait(event_flags_t flags, uint32_t bits, uint32_t ops) {
  uint32_t ret;
  mutex_lock(flags->mutex);
  while (1) {
    ret = try_wait_bits(flags, bits, ops);
    if (ret != 0) {
      break;
    }
    cond_wait(flags->cond, flags->mutex);
  }
  mutex_unlock(flags->mutex);
  return ret;
}

uint32_t event_flags_timed_wait(event_flags_t flags, uint32_t bits,
                                uint32_t ops, uint32_t timeout) {
  uint32_t ret;
  mutex_lock(flags->mutex);
  while (1) {
    ret = try_wait_bits(flags, bits, ops);
    if ((ret != 0) || (timeout == 0)) {
      break;
    }
    timeout = cond_timed_wait(flags->cond, flags->mutex, timeout);
  }
  mutex_unlock(flags->mutex);
  return ret;
}

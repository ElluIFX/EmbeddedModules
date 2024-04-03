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

#if KLITE_CFG_OPT_EVENT_FLAGS

#include <string.h>

struct kl_event_flags {
  kl_mutex_t mutex;
  kl_cond_t cond;
  uint32_t bits;
};

kl_event_flags_t kl_event_flags_create(void) {
  struct kl_event_flags *flags;
  flags = kl_heap_alloc(sizeof(struct kl_event_flags));
  if (flags != NULL) {
    memset(flags, 0, sizeof(struct kl_event_flags));
    flags->mutex = kl_mutex_create();
    if (flags->mutex == NULL) {
      kl_heap_free(flags);
      return NULL;
    }
    flags->cond = kl_cond_create();
    if (flags->cond == NULL) {
      kl_mutex_delete(flags->mutex);
      kl_heap_free(flags);
      return NULL;
    }
  }
  return flags;
}

void kl_event_flags_delete(kl_event_flags_t flags) {
  kl_mutex_delete(flags->mutex);
  kl_cond_delete(flags->cond);
  kl_heap_free(flags);
}

void kl_event_flags_set(kl_event_flags_t flags, uint32_t bits) {
  kl_mutex_lock(flags->mutex);
  flags->bits |= bits;
  kl_mutex_unlock(flags->mutex);
  kl_cond_broadcast(flags->cond);
}

void kl_event_flags_reset(kl_event_flags_t flags, uint32_t bits) {
  kl_mutex_lock(flags->mutex);
  flags->bits &= ~bits;
  kl_mutex_unlock(flags->mutex);
}

static uint32_t try_wait_bits(kl_event_flags_t flags, uint32_t bits,
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

uint32_t kl_event_flags_wait(kl_event_flags_t flags, uint32_t bits,
                             uint32_t ops) {
  uint32_t ret;
  kl_mutex_lock(flags->mutex);
  while (1) {
    ret = try_wait_bits(flags, bits, ops);
    if (ret != 0) {
      break;
    }
    kl_cond_wait(flags->cond, flags->mutex);
  }
  kl_mutex_unlock(flags->mutex);
  return ret;
}

uint32_t kl_event_flags_timed_wait(kl_event_flags_t flags, uint32_t bits,
                                   uint32_t ops, kl_tick_t timeout) {
  uint32_t ret;
  kl_mutex_lock(flags->mutex);
  while (1) {
    ret = try_wait_bits(flags, bits, ops);
    if ((ret != 0) || (timeout == 0)) {
      break;
    }
    timeout = kl_cond_timed_wait(flags->cond, flags->mutex, timeout);
  }
  kl_mutex_unlock(flags->mutex);
  return ret;
}

#endif  // KLITE_CFG_OPT_EVENT_FLAGS

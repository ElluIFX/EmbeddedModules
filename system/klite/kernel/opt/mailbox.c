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

#if KLITE_CFG_OPT_MAILBOX

#include "klite_fifo.h"

kl_mailbox_t kl_mailbox_create(kl_size_t size) {
  struct kl_mailbox *mailbox;
  mailbox = kl_heap_alloc(sizeof(struct kl_mailbox) + size);
  if (mailbox != NULL) {
    fifo_init(&mailbox->fifo, mailbox + 1, size);

    mailbox->mutex = kl_mutex_create();
    if (mailbox->mutex == NULL) {
      kl_heap_free(mailbox);
      return NULL;
    }

    mailbox->write = kl_cond_create();
    if (mailbox->write == NULL) {
      kl_mutex_delete(mailbox->mutex);
      kl_heap_free(mailbox);
      return NULL;
    }

    mailbox->read = kl_cond_create();
    if (mailbox->read == NULL) {
      kl_mutex_delete(mailbox->mutex);
      kl_cond_delete(mailbox->write);
      kl_heap_free(mailbox);
      return NULL;
    }
  }
  return mailbox;
}

void kl_mailbox_delete(kl_mailbox_t mailbox) {
  kl_mutex_delete(mailbox->mutex);
  kl_cond_delete(mailbox->write);
  kl_cond_delete(mailbox->read);
  kl_heap_free(mailbox);
}

void kl_mailbox_clear(kl_mailbox_t mailbox) {
  kl_mutex_lock(mailbox->mutex);
  fifo_clear(&mailbox->fifo);
  kl_mutex_unlock(mailbox->mutex);
  kl_cond_broadcast(mailbox->write);
}

kl_size_t kl_mailbox_post(kl_mailbox_t mailbox, void *buf, kl_size_t len,
                          kl_tick_t timeout) {
  kl_size_t ret;
  kl_size_t ttl;
  ttl = len + sizeof(kl_size_t);
  if (ttl > mailbox->fifo.size) return 0;
  kl_mutex_lock(mailbox->mutex);
  while (1) {
    ret = fifo_get_free(&mailbox->fifo);
    if (ret >= ttl) {
      fifo_write(&mailbox->fifo, &len, sizeof(kl_size_t));
      fifo_write(&mailbox->fifo, buf, len);
      kl_mutex_unlock(mailbox->mutex);
      kl_cond_broadcast(mailbox->read);
      return len;
    }
    if (timeout > 0) {
      timeout = kl_cond_timed_wait(mailbox->write, mailbox->mutex, timeout);
      if (timeout != 0) continue;
    }
    kl_mutex_unlock(mailbox->mutex);
    return 0;
  }
}

kl_size_t kl_mailbox_read(kl_mailbox_t mailbox, void *buf, kl_size_t len,
                          kl_tick_t timeout) {
  kl_size_t ret;
  kl_size_t ttl;
  kl_size_t over;
  uint8_t dummy;
  kl_mutex_lock(mailbox->mutex);
  while (1) {
    ret = fifo_read(&mailbox->fifo, &ttl, sizeof(kl_size_t));
    if (ret != 0) {
      ret = fifo_read(&mailbox->fifo, buf, (len < ttl) ? len : ttl);
      if (ret < ttl) {
        over = ttl - ret;
        while (over--) {
          fifo_read(&mailbox->fifo, &dummy, 1);
        }
      }
      kl_mutex_unlock(mailbox->mutex);
      kl_cond_broadcast(mailbox->write);
      return ttl;
    }
    if (timeout > 0) {
      timeout = kl_cond_timed_wait(mailbox->read, mailbox->mutex, timeout);
      if (timeout != 0) continue;
    }
    kl_mutex_unlock(mailbox->mutex);
    return 0;
  }
}

#endif  // KLITE_CFG_OPT_MAILBOX

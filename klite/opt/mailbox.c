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
#include "mailbox.h"

#include "fifo.h"
#include "kernel.h"

struct mailbox {
  fifo_t fifo;
  mutex_t mutex;
  event_t empty;
  event_t full;
};

mailbox_t mailbox_create(uint32_t size) {
  struct mailbox *mailbox;
  mailbox = heap_alloc( sizeof(struct mailbox) + size);
  if (mailbox != NULL) {
    fifo_init(&mailbox->fifo, mailbox + 1, size);
    mailbox->mutex = mutex_create();
    if (mailbox->mutex == NULL) {
      heap_free( mailbox);
      return NULL;
    }
    mailbox->empty = event_create(true);
    if (mailbox->empty == NULL) {
      mutex_delete(mailbox->mutex);
      heap_free( mailbox);
      return NULL;
    }
    mailbox->full = event_create(true);
    if (mailbox->full == NULL) {
      mutex_delete(mailbox->mutex);
      event_delete(mailbox->empty);
      heap_free( mailbox);
      return NULL;
    }
  }
  return mailbox;
}

void mailbox_delete(mailbox_t mailbox) {
  mutex_delete(mailbox->mutex);
  event_delete(mailbox->empty);
  event_delete(mailbox->full);
  heap_free( mailbox);
}

void mailbox_clear(mailbox_t mailbox) {
  mutex_lock(mailbox->mutex);
  fifo_clear(&mailbox->fifo);
  mutex_unlock(mailbox->mutex);
  event_set(mailbox->empty);
}

uint32_t mailbox_post(mailbox_t mailbox, void *buf, uint32_t len,
                      uint32_t timeout) {
  uint32_t ret;
  uint32_t ttl;
  ttl = len + sizeof(uint32_t);
  while (1) {
    mutex_lock(mailbox->mutex);
    ret = fifo_get_free(&mailbox->fifo);
    if (ret >= ttl) {
      fifo_write(&mailbox->fifo, &len, sizeof(uint32_t));
      fifo_write(&mailbox->fifo, buf, len);
      mutex_unlock(mailbox->mutex);
      event_set(mailbox->full);
      return len;
    }
    if (timeout > 0) {
      mutex_unlock(mailbox->mutex);
      timeout = event_timed_wait(mailbox->empty, timeout);
      continue;
    }
    mutex_unlock(mailbox->mutex);
    return 0;
  }
}

uint32_t mailbox_wait(mailbox_t mailbox, void *buf, uint32_t len,
                      uint32_t timeout) {
  uint32_t ret;
  uint32_t ttl;
  uint32_t over;
  uint8_t dummy;
  while (1) {
    mutex_lock(mailbox->mutex);
    ret = fifo_read(&mailbox->fifo, &ttl, sizeof(uint32_t));
    if (ret != 0) {
      ret = fifo_read(&mailbox->fifo, buf, (len < ttl) ? len : ttl);
      if (ret < ttl) {
        over = ttl - ret;
        while (over--) {
          fifo_read(&mailbox->fifo, &dummy, 1);
        }
      }
      mutex_unlock(mailbox->mutex);
      event_set(mailbox->empty);
      return ttl;
    }
    if (timeout > 0) {
      mutex_unlock(mailbox->mutex);
      timeout = event_timed_wait(mailbox->full, timeout);
      continue;
    }
    mutex_unlock(mailbox->mutex);
    return 0;
  }
}

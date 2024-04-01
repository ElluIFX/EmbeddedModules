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

#if KLITE_CFG_OPT_MAILBOX

#include "klite_internal_fifo.h"

struct mailbox {
  fifo_t fifo;
  mutex_t mutex;
  cond_t write;
  cond_t read;
};

mailbox_t mailbox_create(uint32_t size) {
  struct mailbox *mailbox;
  mailbox = heap_alloc(sizeof(struct mailbox) + size);
  if (mailbox != NULL) {
    fifo_init(&mailbox->fifo, mailbox + 1, size);

    mailbox->mutex = mutex_create();
    if (mailbox->mutex == NULL) {
      heap_free(mailbox);
      return NULL;
    }

    mailbox->write = cond_create();
    if (mailbox->write == NULL) {
      mutex_delete(mailbox->mutex);
      heap_free(mailbox);
      return NULL;
    }

    mailbox->read = cond_create();
    if (mailbox->read == NULL) {
      mutex_delete(mailbox->mutex);
      cond_delete(mailbox->write);
      heap_free(mailbox);
      return NULL;
    }
  }
  return mailbox;
}

void mailbox_delete(mailbox_t mailbox) {
  mutex_delete(mailbox->mutex);
  cond_delete(mailbox->write);
  cond_delete(mailbox->read);
  heap_free(mailbox);
}

void mailbox_clear(mailbox_t mailbox) {
  mutex_lock(mailbox->mutex);
  fifo_clear(&mailbox->fifo);
  mutex_unlock(mailbox->mutex);
  cond_broadcast(mailbox->write);
}

uint32_t mailbox_post(mailbox_t mailbox, void *buf, uint32_t len,
                      klite_tick_t timeout) {
  uint32_t ret;
  uint32_t ttl;
  ttl = len + sizeof(uint32_t);
  mutex_lock(mailbox->mutex);
  while (1) {
    ret = fifo_get_free(&mailbox->fifo);
    if (ret >= ttl) {
      fifo_write(&mailbox->fifo, &len, sizeof(uint32_t));
      fifo_write(&mailbox->fifo, buf, len);
      mutex_unlock(mailbox->mutex);
      cond_broadcast(mailbox->read);
      return len;
    }
    if (timeout > 0) {
      timeout = cond_timed_wait(mailbox->write, mailbox->mutex, timeout);
      if (timeout != 0) continue;
    }
    mutex_unlock(mailbox->mutex);
    return 0;
  }
}

uint32_t mailbox_read(mailbox_t mailbox, void *buf, uint32_t len,
                      klite_tick_t timeout) {
  uint32_t ret;
  uint32_t ttl;
  uint32_t over;
  uint8_t dummy;
  mutex_lock(mailbox->mutex);
  while (1) {
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
      cond_broadcast(mailbox->write);
      return ttl;
    }
    if (timeout > 0) {
      timeout = cond_timed_wait(mailbox->read, mailbox->mutex, timeout);
      if (timeout != 0) continue;
    }
    mutex_unlock(mailbox->mutex);
    return 0;
  }
}

#endif  // KLITE_CFG_OPT_MAILBOX

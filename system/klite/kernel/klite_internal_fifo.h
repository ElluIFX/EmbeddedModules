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
#ifndef __KLITE_FIFO_H
#define __KLITE_FIFO_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t *buf;
  uint32_t size;
  uint32_t wp;
  uint32_t rp;
} fifo_t;

static void fifo_init(void *fifo, void *buf, uint32_t size) {
  ((fifo_t *)fifo)->buf = buf;
  ((fifo_t *)fifo)->size = size;
  ((fifo_t *)fifo)->rp = 0;
  ((fifo_t *)fifo)->wp = 0;
}

static inline uint32_t fifo_read(void *fifo, void *buf, uint32_t size) {
  uint32_t i;
  for (i = 0; i < size; i++) {
    if (((fifo_t *)fifo)->rp == ((fifo_t *)fifo)->wp) {
      break;
    }
    ((uint8_t *)buf)[i] = ((fifo_t *)fifo)->buf[((fifo_t *)fifo)->rp++];
    if (((fifo_t *)fifo)->rp == ((fifo_t *)fifo)->size) {
      ((fifo_t *)fifo)->rp = 0;
    }
  }
  return i;
}

static inline uint32_t fifo_write(void *fifo, void *buf, uint32_t size) {
  uint32_t i;
  uint32_t pos;
  for (i = 0; i < size; i++) {
    pos = ((fifo_t *)fifo)->wp + 1;
    if (pos == ((fifo_t *)fifo)->size) {
      pos = 0;
    }
    if (pos == ((fifo_t *)fifo)->rp) {
      break;
    }
    ((fifo_t *)fifo)->buf[((fifo_t *)fifo)->wp] = ((uint8_t *)buf)[i];
    ((fifo_t *)fifo)->wp = pos;
  }
  return i;
}

static inline void fifo_clear(void *fifo) {
  ((fifo_t *)fifo)->wp = 0;
  ((fifo_t *)fifo)->rp = 0;
}

static inline uint32_t fifo_get_free(void *fifo) {
  if (((fifo_t *)fifo)->rp > ((fifo_t *)fifo)->wp) {
    return ((fifo_t *)fifo)->rp - ((fifo_t *)fifo)->wp - 1;
  } else {
    return ((fifo_t *)fifo)->rp + ((fifo_t *)fifo)->size - ((fifo_t *)fifo)->wp;
  }
}

static uint32_t fifo_get_used(fifo_t *fifo) {
  if (((fifo_t *)fifo)->wp >= ((fifo_t *)fifo)->rp) {
    return ((fifo_t *)fifo)->wp - ((fifo_t *)fifo)->rp;
  } else {
    return ((fifo_t *)fifo)->wp + ((fifo_t *)fifo)->size - ((fifo_t *)fifo)->rp;
  }
}

#endif

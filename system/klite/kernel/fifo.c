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
#include "klite_internal_fifo.h"

void fifo_init(fifo_t *fifo, void *buf, uint32_t size) {
  fifo->buf = buf;
  fifo->size = size;
  fifo->rp = 0;
  fifo->wp = 0;
}

uint32_t fifo_read(fifo_t *fifo, void *buf, uint32_t size) {
  uint32_t i;
  for (i = 0; i < size; i++) {
    if (fifo->rp == fifo->wp) {
      break;
    }
    ((uint8_t *)buf)[i] = fifo->buf[fifo->rp++];
    if (fifo->rp == fifo->size) {
      fifo->rp = 0;
    }
  }
  return i;
}

uint32_t fifo_write(fifo_t *fifo, void *buf, uint32_t size) {
  uint32_t i;
  uint32_t pos;
  for (i = 0; i < size; i++) {
    pos = fifo->wp + 1;
    if (pos == fifo->size) {
      pos = 0;
    }
    if (pos == fifo->rp) {
      break;
    }
    fifo->buf[fifo->wp] = ((uint8_t *)buf)[i];
    fifo->wp = pos;
  }
  return i;
}

void fifo_clear(fifo_t *fifo) {
  fifo->wp = 0;
  fifo->rp = 0;
}

uint32_t fifo_get_free(fifo_t *fifo) {
  if (fifo->rp > fifo->wp) {
    return fifo->rp - fifo->wp - 1;
  } else {
    return fifo->rp + fifo->size - fifo->wp;
  }
}

uint32_t fifo_get_used(fifo_t *fifo) {
  if (fifo->wp >= fifo->rp) {
    return fifo->wp - fifo->rp;
  } else {
    return fifo->wp + fifo->size - fifo->rp;
  }
}

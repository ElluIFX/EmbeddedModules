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
#include "mpool.h"

#include <string.h>

#include "kernel.h"

struct mpool {
  mutex_t mutex;
  uint8_t **block_list;
  uint32_t block_count;
  uint32_t free_count;
  uint32_t free_head;
  uint32_t free_tail;
};

mpool_t mpool_create(uint32_t block_size, uint32_t block_count) {
  uint32_t i;
  uint32_t msize;
  mpool_t mpool;
  uint8_t *block;
  msize =
      sizeof(struct mpool) + block_count * (sizeof(uint32_t *) + block_size);
  mpool = heap_alloc( msize);
  if (mpool != NULL) {
    memset(mpool, 0, msize);
    mpool->mutex = mutex_create();
    if (mpool->mutex != NULL) {
      mpool->block_list = (uint8_t **)(mpool + 1);
      mpool->block_count = block_count;
      block = mpool->block_list[block_count];
      for (i = 0; i < block_count; i++) {
        mpool_free(mpool, block);
        block += block_size;
      }
      return mpool;
    }
    heap_free( mpool);
  }
  return NULL;
}

void mpool_delete(mpool_t mpool) {
  mutex_delete(mpool->mutex);
  heap_free( mpool);
}

void *mpool_alloc(mpool_t mpool) {
  void *block = NULL;
  mutex_lock(mpool->mutex);
  if (mpool->free_count > 0) {
    block = mpool->block_list[mpool->free_head];
    mpool->free_count--;
    mpool->free_head++;
    if (mpool->free_head >= mpool->block_count) {
      mpool->free_head = 0;
    }
  }
  mutex_unlock(mpool->mutex);
  return block;
}

void mpool_free(mpool_t mpool, void *block) {
  mutex_lock(mpool->mutex);
  mpool->block_list[mpool->free_tail] = block;
  mpool->free_count++;
  mpool->free_tail++;
  if (mpool->free_tail >= mpool->block_count) {
    mpool->free_tail = 0;
  }
  mutex_unlock(mpool->mutex);
}

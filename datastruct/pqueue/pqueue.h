// Copyright 2023 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#ifndef PQUEUE_H
#define PQUEUE_H
#include <stdbool.h>
#include <stddef.h>

#include "modules.h"

struct pqueue;

/**
 * @brief 返回一个新的队列，如果内存不足，则返回 NULL
 */
struct pqueue* pqueue_new(size_t elsize,
                          int (*compare)(const void* a, const void* b,
                                         void* udata),
                          void* udata);

/**
 * @brief 释放队列
 */
void pqueue_free(struct pqueue* queue);

/**
 * @brief 清空队列
 */
void pqueue_clear(struct pqueue* queue);

/**
 * @brief 将一个项插入队列。如果内存不足，返回 false
 */
bool pqueue_push(struct pqueue* queue, const void* item);

/**
 * @brief 返回队列中的项数
 */
size_t pqueue_count(const struct pqueue* queue);

/**
 * @brief 返回最小项，但不删除它
 */
const void* pqueue_peek(const struct pqueue* queue);

/**
 * @brief 删除最小项并返回它
 */
const void* pqueue_pop(struct pqueue* queue);

#endif

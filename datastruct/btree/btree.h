// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#ifndef BTREE_H
#define BTREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "modules.h"
struct btree;
typedef struct btree btree_t;

/**
 * @brief 返回一个新的B树
 *
 * @param elsize 树中每个元素的大小。插入、删除或搜索的每个元素都将是此大小。
 * @param max_items 每个节点的最大项数。将此设置为零将默认为256。
 * @param compare 一个比较树中项的函数。参见 qsort
 * 标准库函数以了解此函数的工作方式。
 * @param udata 用户定义的数据，传递给比较回调和在 btree_set_item_callbacks
 * 中定义的项回调。
 *
 * @return 一个新的B树
 *
 * @note 必须使用btree_free()释放B树。
 */
struct btree *btree_new(size_t elsize, size_t max_items,
                        int (*compare)(const void *a, const void *b,
                                       void *udata),
                        void *udata);

/**
 * @brief 设置B树在插入和删除项时内部调用的项克隆和释放回调
 *
 * 这些回调是可选的，但是需要使用btree_clone函数的需要复制写入支持的程序可能需要这些回调。
 *
 * 如果克隆成功，克隆函数应返回 true，否则，如果系统内存不足，则返回 false。
 */
void btree_set_item_callbacks(struct btree *btree,
                              bool (*clone)(const void *item, void *into,
                                            void *udata),
                              void (*free)(const void *item, void *udata));

/**
 * @brief 从B树中删除所有项并释放任何已分配的内存。
 *
 * @note 此函数用于清空B树并释放相关的内存资源。
 */
void btree_free(struct btree *btree);

/**
 * @brief 从B树中删除所有项
 *
 * @note 此函数用于清空B树中的所有项，但不释放B树本身的内存。
 */
void btree_clear(struct btree *btree);

/**
 * @brief 如果最后一个写操作失败是因为系统没有更多可用的内存，则返回 true
 *
 * 具有非常量 btree 接收器作为第一个参数的函数可能会出现内存不足的情况，如
 * btree_set，btree_delete，btree_load 等。
 *
 * @note 此函数用于检测是否由于内存不足导致最后一个B树操作失败。
 */
bool btree_oom(const struct btree *btree);

/**
 * @brief 返回从根到叶的B树的高度，如果B树为空，则返回零
 *
 * @return 如果B树为空，返回0，否则返回B树的高度
 *
 * @note 此函数用于获取B树的高度信息。
 */
size_t btree_height(const struct btree *btree);

/**
 * @brief 返回B树中的项数
 *
 * @return 当前存储在B树中的项数
 *
 * @note 此函数用于获取B树中项的数量。
 */
size_t btree_count(const struct btree *btree);

/**
 * @brief 创建B树的即时副本
 *
 * 此操作使用影子复制/写时复制技术。
 *
 * @return B树的新副本
 *
 * @note 此函数用于快速复制B树结构。
 */
struct btree *btree_clone(struct btree *btree);

/**
 * @brief 在B树中插入或替换一项。如果一项被替换，则返回该项，否则返回 NULL。
 *
 * 如果系统无法分配所需的内存，则返回 NULL，btree_oom() 返回 true。
 *
 * @note 此函数用于在B树中插入新项或替换现有项。可能会因为内存不足而失败。
 */
const void *btree_set(struct btree *btree, const void *item);

/**
 * @brief 从B树中删除一项并返回它
 *
 * 如果未找到项，则返回 NULL。
 * 如果使用 btree_clone 克隆了B树，此操作可能触发节点复制。
 * 如果系统无法分配所需的内存，则返回 NULL，并且 btree_oom() 返回 true。
 *
 * @note
 * 此函数用于从B树中删除项。可能会因为内存不足或B树被克隆而触发额外的操作。
 */
const void *btree_delete(struct btree *btree, const void *key);

/**
 * @brief
 * 与btree_set相同，但针对顺序批量加载进行了优化。当项按精确顺序排列时，它的速度可能是btree_set的速度的10倍，但当项未按精确顺序排列时，其速度可能慢25%。
 *
 * 如果系统无法分配所需的内存，则返回 NULL，并且 btree_oom() 返回 true。
 *
 * @note
 * 此函数用于在B树中批量加载数据。它在处理排序数据时表现最好，但对于无序数据，可能比正常的btree_set操作慢。
 */
const void *btree_load(struct btree *btree, const void *item);

/**
 * @brief 删除B树中的第一项并返回它
 *
 * 如果B树为空，则返回 NULL。
 * 如果使用 btree_clone 克隆了B树，此操作可能会触发节点复制。
 * 如果系统无法分配所需的内存，则返回 NULL，并且 btree_oom() 返回 true。
 *
 * @note
 * 此函数用于从B树中删除最小的项（即第一项）。可能会因为内存不足或B树被克隆而触发额外的操作。
 */
const void *btree_pop_min(struct btree *btree);

/**
 * @brief 删除B树中的最后一项并返回它
 *
 * 如果B树为空，则返回 NULL。
 * 如果使用 btree_clone 克隆了B树，此操作可能会触发节点复制。
 * 如果系统无法分配所需的内存，则返回 NULL，并且 btree_oom() 返回 true。
 *
 * @note
 * 此函数用于从B树中删除最大的项（即最后一项）。可能会因为内存不足或B树被克隆而触发额外的操作。
 */
const void *btree_pop_max(struct btree *btree);

/**
 * @brief 返回B树中的第一项，如果B树为空，则返回 NULL。
 *
 * @note 此函数用于检索B树中的最小项（即第一项），不会从B树中删除该项。
 */
const void *btree_min(const struct btree *btree);

/**
 * @brief 返回B树中的最后一项，如果B树为空，则返回 NULL。
 *
 * @note 此函数用于检索B树中的最大项（即最后一项），不会从B树中删除该项。
 */
const void *btree_max(const struct btree *btree);

/**
 * @brief 根据提供的键返回项
 *
 * 如果未找到项，则返回 NULL。
 *
 * @note 此函数用于根据键在B树中检索特定项。
 */
const void *btree_get(const struct btree *btree, const void *key);

/**
 * @brief 在范围 [pivot, last] 内扫描树
 *
 * 换句话说，btree_ascend 按升序迭代所有大于或等于 pivot 的项目。
 *
 * @param pivot 可以为 NULL，表示迭代所有项目。
 * @param iter 可以返回 false 来提前停止迭代。
 *
 * @return 如果迭代提前停止，则返回 false。
 *
 * @note 此函数用于在给定范围内遍历B树中的项目，可以提前停止迭代。
 */
bool btree_ascend(const struct btree *btree, const void *pivot,
                  bool (*iter)(const void *item, void *udata), void *udata);

/**
 * @brief 在范围 [pivot, first] 内扫描树
 *
 * 换句话说，btree_descend() 按降序迭代所有小于或等于 pivot 的项目。
 *
 * @param pivot 可以为 NULL，表示迭代所有项目。
 * @param iter 可以返回 false 来提前停止迭代。
 *
 * @return 如果迭代提前停止，则返回 false。
 *
 * @note 此函数用于在给定范围内遍历B树中的项目，可以提前停止迭代。
 */
bool btree_descend(const struct btree *btree, const void *pivot,
                   bool (*iter)(const void *item, void *udata), void *udata);

/**
 * @brief 与 btree_set 相同，只是可以提供一个可选的
 * "hint"，当作为批处理或在用户空间上下文中进行操作时可能使操作更快。
 *
 * @note
 * 此函数用于在B树中插入或替换项，并可以提供一个可能的提示，以提高批量操作或用户空间操作的性能。
 */
const void *btree_set_hint(struct btree *btree, const void *item,
                           uint64_t *hint);

/**
 * @brief 与 btree_get 相同，只是可以提供一个可选的
 * "hint"，当作为批处理或在用户空间上下文中进行操作时可能使操作更快。
 *
 * @note
 * 此函数用于根据键在B树中检索项，并可以提供一个可能的提示，以提高批量操作或用户空间操作的性能。
 */
const void *btree_get_hint(const struct btree *btree, const void *key,
                           uint64_t *hint);

/**
 * @brief 与 btree_delete 相同，但可以提供一个可选的
 * "hint"，在作为批处理或在用户空间上下文中执行操作时，可能使操作更快。
 *
 * @note
 * 此函数用于从B树中删除项，并可以提供一个可能的提示，以提高批量操作或用户空间操作的性能。
 */
const void *btree_delete_hint(struct btree *btree, const void *key,
                              uint64_t *hint);

/**
 * @brief 与 btree_ascend 相同，但可以提供一个可选的
 * "hint"，在作为批处理或在用户空间上下文中执行操作时，可能使操作更快。
 *
 * @note
 * 此函数用于按升序迭代B树中的项，并可以提供一个可能的提示，以提高批量操作或用户空间操作的性能。
 */
bool btree_ascend_hint(const struct btree *btree, const void *pivot,
                       bool (*iter)(const void *item, void *udata), void *udata,
                       uint64_t *hint);

/**
 * @brief 与 btree_descend 相同，但可以提供一个可选的
 * "hint"，在作为批处理或在用户空间上下文中执行操作时，可能使操作更快。
 *
 * @note
 * 此函数用于按降序迭代B树中的项，并可以提供一个可能的提示，以提高批量操作或用户空间操作的性能。
 */
bool btree_descend_hint(const struct btree *btree, const void *pivot,
                        bool (*iter)(const void *item, void *udata),
                        void *udata, uint64_t *hint);

/**
 * @brief 允许设置自定义搜索函数
 *
 * @note 此函数用于在B树操作中设置一个自定义的搜索函数，以满足特定的搜索需求。
 */
void btree_set_searcher(struct btree *btree,
                        int (*searcher)(const void *items, size_t nitems,
                                        const void *key, bool *found,
                                        void *udata));

// Loop-based iterator
struct btree_iter *btree_iter_new(const struct btree *btree);
void btree_iter_free(struct btree_iter *iter);
bool btree_iter_first(struct btree_iter *iter);
bool btree_iter_last(struct btree_iter *iter);
bool btree_iter_next(struct btree_iter *iter);
bool btree_iter_prev(struct btree_iter *iter);
bool btree_iter_seek(struct btree_iter *iter, const void *key);
const void *btree_iter_item(struct btree_iter *iter);

#endif

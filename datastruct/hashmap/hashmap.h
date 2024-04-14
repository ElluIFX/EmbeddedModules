// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#ifndef HASHMAP_H
#define HASHMAP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "modules.h"

struct hashmap;
typedef struct hashmap hashmap_t;

/**
 * @brief 创建并返回一个新的哈希表
 *
 * @param elsize
 * 哈希表中每个元素的大小。插入、删除或检索的每个元素都将是这个大小。
 * @param cap 哈希表的默认最低容量。如果设置为零，它将默认为16。
 * @param seed0 可选的种子值，传递给哈希函数。可以是任何值。
 * @param seed1 可选的第二个种子值，传递给哈希函数。可以是任何值。
 * @param hash
 * 一个为项目生成哈希值的函数。良好的哈希函数对于优良的性能和安全性至关重要。
 * @param compare
 * 一个比较哈希表中项目的函数。参见标准库函数qsort以了解此函数的工作方式。
 * @param elfree
 * 一个用于释放特定项目的函数。除非在哈希表中存储某种引用数据，否则这应该为NULL。
 * @param udata 一个指向用户数据的指针，传递给compare和elfree
 *
 * @return 一个指针，指向新创建的哈希表。必须使用hashmap_free()来释放该哈希表。
 *
 * @note 哈希表带有两个用于哈希的辅助函数：hashmap_sip() 和 hashmap_murmur()。
 */
hashmap_t* hashmap_new(
    size_t elsize, size_t cap, uint64_t seed0, uint64_t seed1,
    uint64_t (*hash)(const void* item, uint64_t seed0, uint64_t seed1),
    int (*compare)(const void* a, const void* b, void* udata),
    void (*elfree)(void* item), void* udata);

/**
 * @brief 释放哈希表
 *
 * 如果存在，此函数将在哈希表中的每个项目上调用在 hashmap_new
 * 中提供的元素释放函数。这是为了释放哈希表元素中引用的任何数据。
 *
 * @note 当哈希表不再需要时，应调用此函数，以释放分配的内存并防止内存泄漏。
 */
void hashmap_free(hashmap_t* map);

/**
 * @brief 快速清理哈希表
 *
 * 如果存在，每个元素都会调用在 hashmap_new
 * 中给定的元素释放函数，以释放哈希表元素中引用的任何数据。
 *
 * @param update_cap
 * 如果提供，哈希表的容量将更新以匹配当前分配的桶的数量。这是确保此操作不执行任何内存分配的优化。
 *
 * @note 此函数负责清除哈希表中的数据，并可能更新容量以优化性能。
 */
void hashmap_clear(hashmap_t* map, bool update_cap);

/**
 * @brief 返回哈希表中的项数
 *
 * @return 当前存储在哈希表中的项数
 *
 * @note 此函数可以用来检查给定时间点的哈希表的大小。
 */
size_t hashmap_count(hashmap_t* map);

/**
 * @brief 检查最后一次 hashmap_set() 调用是否由于内存不足而失败
 *
 * @return 如果最后一次 hashmap_set() 调用由于系统内存不足而失败，则返回
 * true，否则返回 false。
 *
 * @note 此函数用于检测最后一次 hashmap set 操作中是否存在内存不足的错误。
 */
bool hashmap_oom(hashmap_t* map);

/**
 * @brief 根据提供的键返回一项
 *
 * @param key 与要检索的项关联的键
 *
 * @return 如果找到则返回与键关联的项，否则返回 NULL
 *
 * @note 该函数用于使用键从哈希表中检索特定项。如果未找到该项，则返回 NULL。
 */
const void* hashmap_get(hashmap_t* map, const void* item);

/**
 * @brief 在哈希表中插入或替换一项
 *
 * 如果一项被替换，则返回该项。如果插入了新项目，则返回
 * NULL。此操作可能需要额外的内存分配。如果系统无法分配额外的内存，返回 NULL
 * 并且 hashmap_oom() 将返回 true。
 *
 * @return 如果替换了一项，则返回被替换的项，否则返回 NULL。
 *
 * @note
 * 此函数用于在哈希表中插入新项或替换现有项。可能需要内存分配以在必要时扩展哈希表。
 */
const void* hashmap_set(hashmap_t* map, const void* item);

/**
 * @brief 从哈希表中删除一项并返回它
 *
 * @return 如果找到则返回已删除的项，否则返回 NULL
 *
 * @note 该函数用于在哈希表中找到特定项并删除它。如果找不到该项，它将返回 NULL。
 */
const void* hashmap_delete(hashmap_t* map, const void* item);

/**
 * @brief 返回位于特定位置的桶中的项，或者如果桶为空，则返回 NULL
 *
 * @param position 哈希表中桶的位置。位置由哈希表中的桶数进行“取模”。
 *
 * @return 如果存在，则返回指定桶中的项，否则返回 NULL
 *
 * @note
 * 此函数可以用于直接索引哈希表而无需搜索。如果您知道桶的位置，可以用于有效的直接访问。
 */
const void* hashmap_probe(hashmap_t* map, uint64_t position);

/**
 * @brief 遍历哈希表中的所有项目
 *
 * @param iter 可以返回 false 以提前停止迭代的函数
 *
 * @return 如果迭代提前停止，返回 false
 *
 * @note
 * 该函数用于在哈希表中的所有项目上执行操作，并在需要时提供提前停止迭代的功能。
 */
bool hashmap_scan(hashmap_t* map, bool (*iter)(const void* item, void* udata),
                  void* udata);

/**
 * @brief 一次遍历哈希表中的一个键，每次迭代都会产生一个条目引用
 *
 * 此函数对于编写简单的循环很有用，可以避免需要编写专用的回调和用户数据结构，就像在
 * hashmap_scan 中可能会执行的操作那样。
 *
 * @param map 指向哈希表句柄的指针。
 * @param i 指向 size_t 游标的指针，应该在循环开始时初始化为 0。
 * @param item  用检索到的项填充的 void
 * 双指针。这不是存储在哈希表中的项的副本，可以直接修改。
 *
 * @return 如果检索到一个项目，则返回 true；如果已经到达迭代的结尾，则返回
 * false。
 *
 * @note 如果在迭代的哈希表上调用
 * hashmap_delete()，则桶将被重新排列，迭代器必须重置为
 * 0，否则可能会在删除后返回意外的结果。
 *
 * @warning 该函数未经线程安全测试。
 */
bool hashmap_iter(hashmap_t* map, size_t* i, void** item);

/**
 * @brief 根据提供的键和哈希返回一项
 *
 * 此函数的工作方式类似于
 * hashmap_get，但它要求用户提供自己的哈希。将不会调用提供给 hashmap_new 函数的
 * 'hash' 回调。
 *
 * @param key 与要检索的项关联的键
 * @param hash 用户提供的哈希函数
 *
 * @return 如果找到，则返回与键关联的项，否则返回 NULL
 *
 * @note 这是 hashmap_get 的一个专门版本，允许用户定义哈希函数。
 */
const void* hashmap_get_with_hash(hashmap_t* map, const void* key,
                                  uint64_t hash);

/**
 * @brief 使用提供的哈希从哈希表中删除一项并返回它
 *
 * 此函数的工作方式类似于
 * hashmap_delete，但它要求用户提供自己的哈希。将不会调用提供给 hashmap_new
 * 函数的 'hash' 回调。
 *
 * @return 如果找到，则返回已删除的项，否则返回 NULL
 *
 * @note 这是 hashmap_delete 的一个专门版本，允许用户定义哈希函数。
 */
const void* hashmap_delete_with_hash(hashmap_t* map, const void* key,
                                     uint64_t hash);

/**
 * @brief 使用提供的哈希向哈希表中设置值
 *
 * 此函数的工作方式类似于
 * hashmap_set，但它要求用户提供自己的哈希。将不会调用提供给 hashmap_new 函数的
 * 'hash' 回调。
 *
 * @note 这是 hashmap_set 的一个专门版本，允许用户定义哈希函数。
 */
const void* hashmap_set_with_hash(hashmap_t* map, const void* item,
                                  uint64_t hash);

void hashmap_set_grow_by_power(hashmap_t* map, size_t power);
void hashmap_set_load_factor(hashmap_t* map, double load_factor);

uint64_t hashmap_sip(const void* data, size_t len, uint64_t seed0,
                     uint64_t seed1);
uint64_t hashmap_murmur(const void* data, size_t len, uint64_t seed0,
                        uint64_t seed1);
uint64_t hashmap_xxhash3(const void* data, size_t len, uint64_t seed0,
                         uint64_t seed1);

#endif

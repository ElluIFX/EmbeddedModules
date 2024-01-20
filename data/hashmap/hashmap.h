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
 * @brief Create and return a new hash map
 *
 * @param elsize Size of each element in the tree. Every element that is
 * inserted, deleted, or retrieved will be this size.
 * @param cap Default lower capacity of the hashmap. If set to zero, it will
 * default to 16.
 * @param seed0 Optional seed value that is passed to the hash function. Can be
 * any value.
 * @param seed1 Optional second seed value that is passed to the hash function.
 * Can be any value.
 * @param hash A function that generates a hash value for an item. A good hash
 * function is crucial for good performance and security.
 * @param compare A function that compares items in the tree. See the qsort
 * stdlib function for an example of how this function works.
 * @param elfree A function that frees a specific item. This should be NULL
 * unless storing some kind of reference data in the hash.
 * @param udata A pointer to user data that is passed to the compare and elfree
 *
 * @return A pointer to the newly created hashmap. This hashmap must be freed
 * using hashmap_free().
 *
 * @note The hashmap comes with two helper functions for hashing: hashmap_sip()
 * and hashmap_murmur().
 */
hashmap_t *hashmap_new(
    size_t elsize, size_t cap, uint64_t seed0, uint64_t seed1,
    uint64_t (*hash)(const void *item, uint64_t seed0, uint64_t seed1),
    int (*compare)(const void *a, const void *b, void *udata),
    void (*elfree)(void *item), void *udata);

/**
 * @brief Frees the hash map
 *
 * This function will call the element-freeing function provided in hashmap_new
 * on every item, if present. This is to free any data referenced in the
 * elements of the hashmap.
 *
 * @note This function should be called when the hash map is no longer needed,
 * to free the allocated memory and prevent memory leaks.
 */
void hashmap_free(hashmap_t *map);

/**
 * @brief Quickly clears the map
 *
 * Every item is called with the element-freeing function given in hashmap_new,
 * if present, to free any data referenced in the elements of the hashmap.
 *
 * @param update_cap If provided, the map's capacity will be updated to match
 * the currently number of allocated buckets. This is an optimization to ensure
 * that this operation does not perform any allocations.
 *
 * @note This function is responsible for clearing the data in the map and
 * potentially updating the capacity to optimize performance.
 */
void hashmap_clear(hashmap_t *map, bool update_cap);

/**
 * @brief Returns the number of items in the hash map
 *
 * @return The number of items currently stored in the hash map
 *
 * @note This function can be used to check the size of the hash map at a given
 * point in time.
 */
size_t hashmap_count(hashmap_t *map);

/**
 * @brief Checks if the last hashmap_set() call failed due to out of memory
 * error
 *
 * @return Returns true if the last hashmap_set() call failed due to the system
 * being out of memory, otherwise returns false.
 *
 * @note This function is used to detect if there was an out of memory error in
 * the last hashmap set operation.
 */
bool hashmap_oom(hashmap_t *map);

/**
 * @brief Returns an item based on the provided key
 *
 * @param key The key associated with the item to retrieve
 *
 * @return The item associated with the key if found, otherwise returns NULL
 *
 * @note This function is used to retrieve a specific item from the hash map
 * using a key.  If the item is not found, NULL is returned.
 */
const void *hashmap_get(hashmap_t *map, const void *item);

/**
 * @brief Inserts or replaces an item in the hash map
 *
 * If an item is replaced, it is returned. If a new item is inserted, NULL is
 * returned. This operation may require additional memory allocation. If the
 * system is unable to allocate additional memory, NULL is returned and
 * hashmap_oom() will return true.
 *
 * @return The replaced item if an item is replaced, otherwise returns NULL.
 *
 * @note This function is used to insert a new item or replace an existing item
 * in the hash map. It may require memory allocation to expand the hash map if
 * necessary.
 */
const void *hashmap_set(hashmap_t *map, const void *item);

/**
 * @brief Removes an item from the hash map and returns it
 *
 * @return The removed item if found, otherwise returns NULL
 *
 * @note The function is used to find a specific item in the hash map and remove
 * it. If the item is not found, it will return NULL.
 */
const void *hashmap_delete(hashmap_t *map, const void *item);

/**
 * @brief Returns the item in the bucket at a certain position or NULL if the
 * bucket is empty
 *
 * @param position The position of the bucket in the hash map. The position is
 * 'moduloed' by the number of buckets in the hash map.
 *
 * @return The item in the specified bucket if it exists, otherwise returns NULL
 *
 * @note This function can be used to directly index into the hash map without
 * searching. It can be used for efficient direct access if you know the bucket
 * position.
 */
const void *hashmap_probe(hashmap_t *map, uint64_t position);

/**
 * @brief Iterates over all items in the hash map.
 *
 * @param iter Function that can return false to stop iteration early.
 *
 * @return Returns false if the iteration has been stopped early.
 *
 * @note This function is used to perform an operation on all items in the hash
 * map, and provides the ability to stop iteration early if needed.
 */
bool hashmap_scan(hashmap_t *map, bool (*iter)(const void *item, void *udata),
                  void *udata);

/**
 * @brief Iterates over the hashmap one key at a time, yielding a reference to
 * an entry at each iteration
 *
 * This function is useful for writing simple loops and avoiding the need to
 * write dedicated callbacks and user data structures, as you might do with
 * hashmap_scan.
 *
 * @param map Pointer to the hashmap handle.
 * @param i Pointer to a size_t cursor that should be initialized to 0 at the
 * beginning of the loop.
 * @param item Void double pointer that is populated with the retrieved item.
 * This is NOT a copy of the item stored in the hash map and can be directly
 * modified.
 *
 * @return Returns true if an item was retrieved; false if the end of the
 * iteration has been reached.
 *
 * @note If hashmap_delete() is called on the hashmap being iterated, the
 * buckets are rearranged and the iterator must be reset to 0, otherwise
 * unexpected results may be returned after deletion.
 *
 * @warning This function has not been tested for thread safety.
 */
bool hashmap_iter(hashmap_t *map, size_t *i, void **item);

/**
 * @brief Returns an item based on the provided key and hash
 *
 * This function works similarly to hashmap_get, but it requires the user to
 * provide their own hash. The 'hash' callback provided to the hashmap_new
 * function will not be called.
 *
 * @param key The key associated with the item to retrieve
 * @param hash The user provided hash function
 *
 * @return The item associated with the key if found, otherwise returns NULL
 *
 * @note This is a specialized version of hashmap_get that allows user-defined
 * hash functions.
 */
const void *hashmap_get_with_hash(hashmap_t *map, const void *key,
                                  uint64_t hash);

/**
 * @brief Removes an item from the hash map using a provided hash and returns it
 *
 * This function works similarly to hashmap_delete, but it requires the user to
 * provide their own hash. The 'hash' callback provided to the hashmap_new
 * function will not be called.
 *
 * @return The removed item if found, otherwise returns NULL
 *
 * @note This is a specialized version of hashmap_delete that allows
 * user-defined hash functions.
 */
const void *hashmap_delete_with_hash(hashmap_t *map, const void *key,
                                     uint64_t hash);

/**
 * @brief Sets a value into the hashmap using a provided hash
 *
 * This function works similarly to hashmap_set, but it requires the user to
 * provide their own hash. The 'hash' callback provided to the hashmap_new
 * function will not be called.
 */
const void *hashmap_set_with_hash(hashmap_t *map, const void *item,
                                  uint64_t hash);

void hashmap_set_grow_by_power(hashmap_t *map, size_t power);
void hashmap_set_load_factor(hashmap_t *map, double load_factor);

uint64_t hashmap_sip(const void *data, size_t len, uint64_t seed0,
                     uint64_t seed1);
uint64_t hashmap_murmur(const void *data, size_t len, uint64_t seed0,
                        uint64_t seed1);
uint64_t hashmap_xxhash3(const void *data, size_t len, uint64_t seed0,
                         uint64_t seed1);

#endif

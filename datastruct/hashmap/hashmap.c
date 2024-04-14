// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "hashmap.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GROW_AT 0.60   /* 60% */
#define SHRINK_AT 0.10 /* 10% */

#ifndef HASHMAP_LOAD_FACTOR
#define HASHMAP_LOAD_FACTOR GROW_AT
#endif

struct bucket {
    uint64_t hash : 48;
    uint64_t dib : 16;
};

// hashmap is an open addressed hash map using robinhood hashing.
struct hashmap {
    size_t elsize;
    size_t cap;
    uint64_t seed0;
    uint64_t seed1;
    uint64_t (*hash)(const void* item, uint64_t seed0, uint64_t seed1);
    int (*compare)(const void* a, const void* b, void* udata);
    void (*elfree)(void* item);
    void* udata;
    size_t bucketsz;
    size_t nbuckets;
    size_t count;
    size_t mask;
    size_t growat;
    size_t shrinkat;
    uint8_t loadfactor;
    uint8_t growpower;
    bool oom;
    void* buckets;
    void* spare;
    void* edata;
};

void hashmap_set_grow_by_power(hashmap_t* map, size_t power) {
    map->growpower = power < 1 ? 1 : power > 16 ? 16 : power;
}

static double clamp_load_factor(double factor, double default_factor) {
    // Check for NaN and clamp between 50% and 90%
    return factor != factor ? default_factor
           : factor < 0.50  ? 0.50
           : factor > 0.95  ? 0.95
                            : factor;
}

void hashmap_set_load_factor(hashmap_t* map, double factor) {
    factor = clamp_load_factor(factor, map->loadfactor / 100.0);
    map->loadfactor = factor * 100;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
}

static struct bucket* bucket_at0(void* buckets, size_t bucketsz, size_t i) {
    return (struct bucket*)(((char*)buckets) + (bucketsz * i));
}

static struct bucket* bucket_at(hashmap_t* map, size_t index) {
    return bucket_at0(map->buckets, map->bucketsz, index);
}

static void* bucket_item(struct bucket* entry) {
    return ((char*)entry) + sizeof(struct bucket);
}

static uint64_t clip_hash(uint64_t hash) {
    return hash & 0xFFFFFFFFFFFF;
}

static uint64_t get_hash(hashmap_t* map, const void* key) {
    return clip_hash(map->hash(key, map->seed0, map->seed1));
}

hashmap_t* hashmap_new(
    size_t elsize, size_t cap, uint64_t seed0, uint64_t seed1,
    uint64_t (*hash)(const void* item, uint64_t seed0, uint64_t seed1),
    int (*compare)(const void* a, const void* b, void* udata),
    void (*elfree)(void* item), void* udata) {
    size_t ncap = 16;
    if (cap < ncap) {
        cap = ncap;
    } else {
        while (ncap < cap) {
            ncap *= 2;
        }
        cap = ncap;
    }
    size_t bucketsz = sizeof(struct bucket) + elsize;
    while (bucketsz & (sizeof(uintptr_t) - 1)) {
        bucketsz++;
    }
    // hashmap + spare + edata
    size_t size = sizeof(hashmap_t) + bucketsz * 2;
    hashmap_t* map = m_alloc(size);
    if (!map) {
        return NULL;
    }
    memset(map, 0, sizeof(hashmap_t));
    map->elsize = elsize;
    map->bucketsz = bucketsz;
    map->seed0 = seed0;
    map->seed1 = seed1;
    map->hash = hash;
    map->compare = compare;
    map->elfree = elfree;
    map->udata = udata;
    map->spare = ((char*)map) + sizeof(hashmap_t);
    map->edata = (char*)map->spare + bucketsz;
    map->cap = cap;
    map->nbuckets = cap;
    map->mask = map->nbuckets - 1;
    map->buckets = m_alloc(map->bucketsz * map->nbuckets);
    if (!map->buckets) {
        m_free(map);
        return NULL;
    }
    memset(map->buckets, 0, map->bucketsz * map->nbuckets);
    map->growpower = 1;
    map->loadfactor = clamp_load_factor(HASHMAP_LOAD_FACTOR, GROW_AT) * 100;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;
    return map;
}

static void free_elements(hashmap_t* map) {
    if (map->elfree) {
        for (size_t i = 0; i < map->nbuckets; i++) {
            struct bucket* bucket = bucket_at(map, i);
            if (bucket->dib)
                map->elfree(bucket_item(bucket));
        }
    }
}

void hashmap_clear(hashmap_t* map, bool update_cap) {
    map->count = 0;
    free_elements(map);
    if (update_cap) {
        map->cap = map->nbuckets;
    } else if (map->nbuckets != map->cap) {
        void* new_buckets = m_alloc(map->bucketsz * map->cap);
        if (new_buckets) {
            m_free(map->buckets);
            map->buckets = new_buckets;
        }
        map->nbuckets = map->cap;
    }
    memset(map->buckets, 0, map->bucketsz * map->nbuckets);
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;
}

static bool resize0(hashmap_t* map, size_t new_cap) {
    hashmap_t* map2 =
        hashmap_new(map->elsize, new_cap, map->seed0, map->seed1, map->hash,
                    map->compare, map->elfree, map->udata);
    if (!map2)
        return false;
    for (size_t i = 0; i < map->nbuckets; i++) {
        struct bucket* entry = bucket_at(map, i);
        if (!entry->dib) {
            continue;
        }
        entry->dib = 1;
        size_t j = entry->hash & map2->mask;
        while (1) {
            struct bucket* bucket = bucket_at(map2, j);
            if (bucket->dib == 0) {
                memcpy(bucket, entry, map->bucketsz);
                break;
            }
            if (bucket->dib < entry->dib) {
                memcpy(map2->spare, bucket, map->bucketsz);
                memcpy(bucket, entry, map->bucketsz);
                memcpy(entry, map2->spare, map->bucketsz);
            }
            j = (j + 1) & map2->mask;
            entry->dib += 1;
        }
    }
    m_free(map->buckets);
    map->buckets = map2->buckets;
    map->nbuckets = map2->nbuckets;
    map->mask = map2->mask;
    map->growat = map2->growat;
    map->shrinkat = map2->shrinkat;
    m_free(map2);
    return true;
}

static bool resize(hashmap_t* map, size_t new_cap) {
    return resize0(map, new_cap);
}

const void* hashmap_set_with_hash(hashmap_t* map, const void* item,
                                  uint64_t hash) {
    hash = clip_hash(hash);
    map->oom = false;
    if (map->count >= map->growat) {
        if (!resize(map, map->nbuckets * (1 << map->growpower))) {
            map->oom = true;
            return NULL;
        }
    }

    struct bucket* entry = map->edata;
    entry->hash = hash;
    entry->dib = 1;
    void* eitem = bucket_item(entry);
    memcpy(eitem, item, map->elsize);

    void* bitem;
    size_t i = entry->hash & map->mask;
    while (1) {
        struct bucket* bucket = bucket_at(map, i);
        if (bucket->dib == 0) {
            memcpy(bucket, entry, map->bucketsz);
            map->count++;
            return NULL;
        }
        bitem = bucket_item(bucket);
        if (entry->hash == bucket->hash &&
            (!map->compare || map->compare(eitem, bitem, map->udata) == 0)) {
            memcpy(map->spare, bitem, map->elsize);
            memcpy(bitem, eitem, map->elsize);
            return map->spare;
        }
        if (bucket->dib < entry->dib) {
            memcpy(map->spare, bucket, map->bucketsz);
            memcpy(bucket, entry, map->bucketsz);
            memcpy(entry, map->spare, map->bucketsz);
            eitem = bucket_item(entry);
        }
        i = (i + 1) & map->mask;
        entry->dib += 1;
    }
}

const void* hashmap_set(hashmap_t* map, const void* item) {
    return hashmap_set_with_hash(map, item, get_hash(map, item));
}

const void* hashmap_get_with_hash(hashmap_t* map, const void* key,
                                  uint64_t hash) {
    hash = clip_hash(hash);
    size_t i = hash & map->mask;
    while (1) {
        struct bucket* bucket = bucket_at(map, i);
        if (!bucket->dib)
            return NULL;
        if (bucket->hash == hash) {
            void* bitem = bucket_item(bucket);
            if (!map->compare || map->compare(key, bitem, map->udata) == 0) {
                return bitem;
            }
        }
        i = (i + 1) & map->mask;
    }
}

const void* hashmap_get(hashmap_t* map, const void* key) {
    return hashmap_get_with_hash(map, key, get_hash(map, key));
}

const void* hashmap_probe(hashmap_t* map, uint64_t position) {
    size_t i = position & map->mask;
    struct bucket* bucket = bucket_at(map, i);
    if (!bucket->dib) {
        return NULL;
    }
    return bucket_item(bucket);
}

const void* hashmap_delete_with_hash(hashmap_t* map, const void* key,
                                     uint64_t hash) {
    hash = clip_hash(hash);
    map->oom = false;
    size_t i = hash & map->mask;
    while (1) {
        struct bucket* bucket = bucket_at(map, i);
        if (!bucket->dib) {
            return NULL;
        }
        void* bitem = bucket_item(bucket);
        if (bucket->hash == hash &&
            (!map->compare || map->compare(key, bitem, map->udata) == 0)) {
            memcpy(map->spare, bitem, map->elsize);
            bucket->dib = 0;
            while (1) {
                struct bucket* prev = bucket;
                i = (i + 1) & map->mask;
                bucket = bucket_at(map, i);
                if (bucket->dib <= 1) {
                    prev->dib = 0;
                    break;
                }
                memcpy(prev, bucket, map->bucketsz);
                prev->dib--;
            }
            map->count--;
            if (map->nbuckets > map->cap && map->count <= map->shrinkat) {
                // Ignore the return value. It's ok for the resize operation to
                // fail to allocate enough memory because a shrink operation
                // does not change the integrity of the data.
                resize(map, map->nbuckets / 2);
            }
            return map->spare;
        }
        i = (i + 1) & map->mask;
    }
}

const void* hashmap_delete(hashmap_t* map, const void* key) {
    return hashmap_delete_with_hash(map, key, get_hash(map, key));
}

size_t hashmap_count(hashmap_t* map) {
    return map->count;
}

void hashmap_free(hashmap_t* map) {
    if (!map)
        return;
    free_elements(map);
    m_free(map->buckets);
    m_free(map);
}

bool hashmap_oom(hashmap_t* map) {
    return map->oom;
}

bool hashmap_scan(hashmap_t* map, bool (*iter)(const void* item, void* udata),
                  void* udata) {
    for (size_t i = 0; i < map->nbuckets; i++) {
        struct bucket* bucket = bucket_at(map, i);
        if (bucket->dib && !iter(bucket_item(bucket), udata)) {
            return false;
        }
    }
    return true;
}

bool hashmap_iter(hashmap_t* map, size_t* i, void** item) {
    struct bucket* bucket;
    do {
        if (*i >= map->nbuckets)
            return false;
        bucket = bucket_at(map, *i);
        (*i)++;
    } while (!bucket->dib);
    *item = bucket_item(bucket);
    return true;
}

//-----------------------------------------------------------------------------
// SipHash reference C implementation
//
// Copyright (c) 2012-2016 Jean-Philippe Aumasson
// <jeanphilippe.aumasson@gmail.com>
// Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
//
// To the extent possible under law, the author(s) have dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along
// with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
//
// default: SipHash-2-4
//-----------------------------------------------------------------------------
static uint64_t SIP64(const uint8_t* in, const size_t inlen, uint64_t seed0,
                      uint64_t seed1) {
#define U8TO64_LE(p)                                            \
    {(((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |        \
      ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) | \
      ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) | \
      ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))}
#define U64TO8_LE(p, v)                            \
    {                                              \
        U32TO8_LE((p), (uint32_t)((v)));           \
        U32TO8_LE((p) + 4, (uint32_t)((v) >> 32)); \
    }
#define U32TO8_LE(p, v)                \
    {                                  \
        (p)[0] = (uint8_t)((v));       \
        (p)[1] = (uint8_t)((v) >> 8);  \
        (p)[2] = (uint8_t)((v) >> 16); \
        (p)[3] = (uint8_t)((v) >> 24); \
    }
#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))
#define SIPROUND           \
    {                      \
        v0 += v1;          \
        v1 = ROTL(v1, 13); \
        v1 ^= v0;          \
        v0 = ROTL(v0, 32); \
        v2 += v3;          \
        v3 = ROTL(v3, 16); \
        v3 ^= v2;          \
        v0 += v3;          \
        v3 = ROTL(v3, 21); \
        v3 ^= v0;          \
        v2 += v1;          \
        v1 = ROTL(v1, 17); \
        v1 ^= v2;          \
        v2 = ROTL(v2, 32); \
    }
    uint64_t k0 = U8TO64_LE((uint8_t*)&seed0);
    uint64_t k1 = U8TO64_LE((uint8_t*)&seed1);
    uint64_t v3 = UINT64_C(0x7465646279746573) ^ k1;
    uint64_t v2 = UINT64_C(0x6c7967656e657261) ^ k0;
    uint64_t v1 = UINT64_C(0x646f72616e646f6d) ^ k1;
    uint64_t v0 = UINT64_C(0x736f6d6570736575) ^ k0;
    const uint8_t* end = in + inlen - (inlen % sizeof(uint64_t));
    for (; in != end; in += 8) {
        uint64_t m = U8TO64_LE(in);
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    switch (left) {
        case 7:
            b |= ((uint64_t)in[6]) << 48; /* fall through */
        case 6:
            b |= ((uint64_t)in[5]) << 40; /* fall through */
        case 5:
            b |= ((uint64_t)in[4]) << 32; /* fall through */
        case 4:
            b |= ((uint64_t)in[3]) << 24; /* fall through */
        case 3:
            b |= ((uint64_t)in[2]) << 16; /* fall through */
        case 2:
            b |= ((uint64_t)in[1]) << 8; /* fall through */
        case 1:
            b |= ((uint64_t)in[0]);
            break;
        case 0:
            break;
    }
    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    b = v0 ^ v1 ^ v2 ^ v3;
    uint64_t out = 0;
    U64TO8_LE((uint8_t*)&out, b);
    return out;
}

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// Murmur3_86_128
//-----------------------------------------------------------------------------
static uint64_t MM86128(const void* key, const int len, uint32_t seed) {
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))
#define FMIX32(h)    \
    h ^= h >> 16;    \
    h *= 0x85ebca6b; \
    h ^= h >> 13;    \
    h *= 0xc2b2ae35; \
    h ^= h >> 16;
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks = len / 16;
    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;
    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;
    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 16);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i * 4 + 0];
        uint32_t k2 = blocks[i * 4 + 1];
        uint32_t k3 = blocks[i * 4 + 2];
        uint32_t k4 = blocks[i * 4 + 3];
        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        h1 = ROTL32(h1, 19);
        h1 += h2;
        h1 = h1 * 5 + 0x561ccd1b;
        k2 *= c2;
        k2 = ROTL32(k2, 16);
        k2 *= c3;
        h2 ^= k2;
        h2 = ROTL32(h2, 17);
        h2 += h3;
        h2 = h2 * 5 + 0x0bcaa747;
        k3 *= c3;
        k3 = ROTL32(k3, 17);
        k3 *= c4;
        h3 ^= k3;
        h3 = ROTL32(h3, 15);
        h3 += h4;
        h3 = h3 * 5 + 0x96cd1c35;
        k4 *= c4;
        k4 = ROTL32(k4, 18);
        k4 *= c1;
        h4 ^= k4;
        h4 = ROTL32(h4, 13);
        h4 += h1;
        h4 = h4 * 5 + 0x32ac3b17;
    }
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);
    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;
    switch (len & 15) {
        case 15:
            k4 ^= tail[14] << 16; /* fall through */
        case 14:
            k4 ^= tail[13] << 8; /* fall through */
        case 13:
            k4 ^= tail[12] << 0;
            k4 *= c4;
            k4 = ROTL32(k4, 18);
            k4 *= c1;
            h4 ^= k4;
            /* fall through */
        case 12:
            k3 ^= tail[11] << 24; /* fall through */
        case 11:
            k3 ^= tail[10] << 16; /* fall through */
        case 10:
            k3 ^= tail[9] << 8; /* fall through */
        case 9:
            k3 ^= tail[8] << 0;
            k3 *= c3;
            k3 = ROTL32(k3, 17);
            k3 *= c4;
            h3 ^= k3;
            /* fall through */
        case 8:
            k2 ^= tail[7] << 24; /* fall through */
        case 7:
            k2 ^= tail[6] << 16; /* fall through */
        case 6:
            k2 ^= tail[5] << 8; /* fall through */
        case 5:
            k2 ^= tail[4] << 0;
            k2 *= c2;
            k2 = ROTL32(k2, 16);
            k2 *= c3;
            h2 ^= k2;
            /* fall through */
        case 4:
            k1 ^= tail[3] << 24; /* fall through */
        case 3:
            k1 ^= tail[2] << 16; /* fall through */
        case 2:
            k1 ^= tail[1] << 8; /* fall through */
        case 1:
            k1 ^= tail[0] << 0;
            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
            /* fall through */
    };
    h1 ^= len;
    h2 ^= len;
    h3 ^= len;
    h4 ^= len;
    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;
    FMIX32(h1);
    FMIX32(h2);
    FMIX32(h3);
    FMIX32(h4);
    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;
    return (((uint64_t)h2) << 32) | h1;
}

//-----------------------------------------------------------------------------
// xxHash Library
// Copyright (c) 2012-2021 Yann Collet
// All rights reserved.
//
// BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
//
// xxHash3
//-----------------------------------------------------------------------------
#define XXH_PRIME_1 11400714785074694791ULL
#define XXH_PRIME_2 14029467366897019727ULL
#define XXH_PRIME_3 1609587929392839161ULL
#define XXH_PRIME_4 9650029242287828579ULL
#define XXH_PRIME_5 2870177450012600261ULL

static uint64_t XXH_read64(const void* memptr) {
    uint64_t val;
    memcpy(&val, memptr, sizeof(val));
    return val;
}

static uint32_t XXH_read32(const void* memptr) {
    uint32_t val;
    memcpy(&val, memptr, sizeof(val));
    return val;
}

static uint64_t XXH_rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

static uint64_t xxh3(const void* data, size_t len, uint64_t seed) {
    const uint8_t* p = (const uint8_t*)data;
    const uint8_t* const end = p + len;
    uint64_t h64;

    if (len >= 32) {
        const uint8_t* const limit = end - 32;
        uint64_t v1 = seed + XXH_PRIME_1 + XXH_PRIME_2;
        uint64_t v2 = seed + XXH_PRIME_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - XXH_PRIME_1;

        do {
            v1 += XXH_read64(p) * XXH_PRIME_2;
            v1 = XXH_rotl64(v1, 31);
            v1 *= XXH_PRIME_1;

            v2 += XXH_read64(p + 8) * XXH_PRIME_2;
            v2 = XXH_rotl64(v2, 31);
            v2 *= XXH_PRIME_1;

            v3 += XXH_read64(p + 16) * XXH_PRIME_2;
            v3 = XXH_rotl64(v3, 31);
            v3 *= XXH_PRIME_1;

            v4 += XXH_read64(p + 24) * XXH_PRIME_2;
            v4 = XXH_rotl64(v4, 31);
            v4 *= XXH_PRIME_1;

            p += 32;
        } while (p <= limit);

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) +
              XXH_rotl64(v4, 18);

        v1 *= XXH_PRIME_2;
        v1 = XXH_rotl64(v1, 31);
        v1 *= XXH_PRIME_1;
        h64 ^= v1;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v2 *= XXH_PRIME_2;
        v2 = XXH_rotl64(v2, 31);
        v2 *= XXH_PRIME_1;
        h64 ^= v2;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v3 *= XXH_PRIME_2;
        v3 = XXH_rotl64(v3, 31);
        v3 *= XXH_PRIME_1;
        h64 ^= v3;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v4 *= XXH_PRIME_2;
        v4 = XXH_rotl64(v4, 31);
        v4 *= XXH_PRIME_1;
        h64 ^= v4;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;
    } else {
        h64 = seed + XXH_PRIME_5;
    }

    h64 += (uint64_t)len;

    while (p + 8 <= end) {
        uint64_t k1 = XXH_read64(p);
        k1 *= XXH_PRIME_2;
        k1 = XXH_rotl64(k1, 31);
        k1 *= XXH_PRIME_1;
        h64 ^= k1;
        h64 = XXH_rotl64(h64, 27) * XXH_PRIME_1 + XXH_PRIME_4;
        p += 8;
    }

    if (p + 4 <= end) {
        h64 ^= (uint64_t)(XXH_read32(p)) * XXH_PRIME_1;
        h64 = XXH_rotl64(h64, 23) * XXH_PRIME_2 + XXH_PRIME_3;
        p += 4;
    }

    while (p < end) {
        h64 ^= (*p) * XXH_PRIME_5;
        h64 = XXH_rotl64(h64, 11) * XXH_PRIME_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= XXH_PRIME_2;
    h64 ^= h64 >> 29;
    h64 *= XXH_PRIME_3;
    h64 ^= h64 >> 32;

    return h64;
}

// hashmap_sip returns a hash value for `data` using SipHash-2-4.
uint64_t hashmap_sip(const void* data, size_t len, uint64_t seed0,
                     uint64_t seed1) {
    return SIP64((uint8_t*)data, len, seed0, seed1);
}

// hashmap_murmur returns a hash value for `data` using Murmur3_86_128.
uint64_t hashmap_murmur(const void* data, size_t len, uint64_t seed0,
                        uint64_t seed1) {
    (void)seed1;
    return MM86128(data, len, seed0);
}

uint64_t hashmap_xxhash3(const void* data, size_t len, uint64_t seed0,
                         uint64_t seed1) {
    (void)seed1;
    return xxh3(data, len, seed0);
}

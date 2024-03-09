# unordered_set - CTL - C Container Template library

Defined in header **<ctl_unordered_set.h>**, CTL prefix **uset**,
parent of [unordered_map](unordered_map.md)

# SYNOPSIS

    size_t int_hash(int* x) { return abs(*x); }
    int int_eq(int* a, int* b) { return *a == *b; }

    #define POD
    #define T int
    #include <ctl_unordered_set.h>

    uset_int a = uset_int_init(int_hash, int_eq);
    for (int i=0; i < 120; i++)
      uset_int_insert(&a, rand());

    printf ("5 is %s included\n", uset_int_contains(&a, 5) ? "" : "not");
    uset_digi_node* n = uset_int_find(&a, 5));
    uset_int_erase(&a, 5);

    foreach(uset_int, &a, it) { printf("GOT %d\n", *it.ref); }
    foreach(uset_int, &a, it) { printf("SIZE %lu\n", uset_int_node_bucket_size(it.node)); }
    printf("load_factor: %f\n", uset_int_load_factor(&a));

    uset_int_free(&a);

# DESCRIPTION

`unordered_set` is an associative container (chained hash table) that contains a
set of unique objects of type Key. Search, insertion, and removal have average
constant-time complexity.

The function names are composed of the prefix **uset_**, the user-defined type
**T** and the method name. E.g `uset_int` with `#define T int`.

Internally, the elements are not sorted in any particular order, but organized
into buckets. Which bucket an element is placed into depends entirely on the
hash of its value. This allows fast access to individual elements, since once a
hash is computed, it refers to the exact bucket the element is placed into.

We need the slow chained hash table to guarantee that pointers into nodes and
values stay the same. For faster open-adressing hash tables a seperate
[hashtable](hashtable.md) container is in work.
Container elements may not be modified since modification could change an
element's hash and corrupt the container.

# Member types

`T`                      value type

`A` being `uset_T`       container type

`B` being `uset_T_node`  node type

`I` being `uset_T_it`    iterator type

## Member functions

    A init (T_hash(T*), T_equal(T*, T*))

constructs the hash table.
With INTEGRAL types the members may be NULL, and are then set to default
methods.

    free (A* self)

destructs the hash table.

    assign_range (A* self, I* range)

replaces the contents of the container with elements from a different
container, if possible. _(NYI)_

    A copy (A* self)

returns a copy of the container.

## Iterators

    I begin (A* self)

constructs an iterator to the beginning.

    I end (A* self)

constructs an iterator to the end.

`unordered_set` does not support ranges, as this does not make sense.
Our `uset` iterator just supports `foreach`.

## Capacity

    int empty (A* self)

checks whether the container is empty

    size_t size (A* self)

returns the number of non-empty and non-deleted elements

    size_t bucket_count (A* self)

returns the size of the array. Same as capacity

    size_t max_size ()

returns the maximum possible number of elements, hard-coded to 2GB (32bit).

## Modifiers

    clear (A* self)

clears the contents

    insert (A* self, T value)

inserts the element. (C++17)

    I insert_found (A* self, T value, *foundp)

inserts the element and set *foundp if it already existed.

    insert_generic (A* self, GI* range2)

inserts copies of values from generic range2. _(NYI)_

    I emplace (A* self, T *value)

constructs elements in-place. _(NY)_

    I emplace_hint (I* pos, T *value)

constructs elements in-place, hopefully at position. _(NY)_

    I emplace_found (A* self, T *value, int* foundp)

constructs elements in-place and sets *foundp if it already existed. _(NY)_

    erase (A* self, T key)

erases the element by key

    size_t erase_if (A* self, int match(T*))

erases the element by match.

    erase_range (A* self, I* range)

erases elements. _(NYI)_

    erase_generic (A* self, GI* range)

erases elements by value from another container.

    swap (A* self, A* other)

swaps the contents

    extract (A* self, T key)

extracts a node from the container. _(NYI)_

    merge (A* self, A* other)

splices nodes from another container

## Member fields

    .hash

Hash method `int (_hash)(T_)

    .equal

equal method `int (_equal)(T_, T*)

## Lookup

    size_t count (A* self)

returns the number of elements matching specific key. It will always be 1,
unless your equal method is broken.

    I find (A* self, T key)
    B* find_node (A* self, T key)

finds bucket with specific key.

    bool contains (A* self, T key)

checks if the container contains element with specific key. (C++20)

    int equal (A* self, A* other)

if all elements are equal.

## Bucket interface

    size_t bucket_count (A* self)

returns the number of buckets

    size_t max_bucket_count (A* self)

returns the maximum number of buckets of the set.

    size_t bucket_size (A* self, size_t bucket_index)
    size_t bucket_size (B* bucket)

returns the number of elements in the specific bucket, the collisions.

    size_t bucket (A* self, T value)

returns the bucket index for the key.

## Hash policy

Growth policy defines:

```C
#define CTL_USET_GROWTH_PRIMED
/* slower but more secure. uses all hash bits. (default) */
#define CTL_USET_GROWTH_POWER2
/* faster, but less secure. uses only some lower bits.
   not recommended with public inet access (json, ...) */
```

`CTL_USET_GROWTH_POWER2` rehashes with bucket_count _2,
`CTL_USET_GROWTH_PRIMED` rehashes with the next prime at bucket_count_ 1.618.

`CTL_USET_GROWTH_FACTOR` defaults to above.

`CTL_USET_CACHED_HASH` stores the hash of each value in the bucket and is used
to short-circuit slower equal value comparisons. It trades memory for faster
unsuccesful searches, such as with insert with high load factor and many collisions.

Planned:

- `CTL_USET_MOVE_TO_FRONT` moves a bucket in a chain not at the top
position to the top in each access, such as find and contains, not only insert.

**Security policies**:

`CTL_USET_SECURITY_COLLCOUNTING` against DDOS attacks, overflowing the chained list:

A seeded hash might need a 2nd hash arg (esp. with threads), but random hash
seeds are only security theatre.

0: ignore `CTL_USET_SECURITY_COLLCOUNTING 0`

1: sorted vector. `CTL_USET_SECURITY_COLLCOUNTING 1` _(NYI)_

2: collision counting with sleep. `CTL_USET_SECURITY_COLLCOUNTING 2`

3: collision counting with abort. `CTL_USET_SECURITY_COLLCOUNTING 3`

4: collision counting with change to sorted vector. `CTL_USET_SECURITY_COLLCOUNTING 4` _(NYI)_

5: collision counting with change to tree (as in java). `CTL_USET_SECURITY_COLLCOUNTING 5` _(NYI)_

With 2 and 3 you may also override `CTL_USET_SECURITY_ACTION` which default to
`sleep(1)` for 2 on non-windows, `Sleep(500)` on Windows, and `abort()`
for 3. E.g. to include extra logging for `fail2ban`.

Methods:

    float load_factor (A* self)

returns average number of elements per bucket

    max_load_factor (A* self, float factor)

Sets maximum average number of elements per bucket. Defaults to 1.0

    rehash (A* self, size_t bucket_count)

reserves at least the specified number of buckets.
This might regenerate the hash table, but not the buckets.

    reserve (A* self, size_t desired_size)

reserves space for at least the specified number of elements.
This might regenerate the hash table, but not the buckets.

## Non-member functions

Note that we support no `_range` variants here. ranges make no sense
with random order.

    swap (A* self)

specializes the swap algorithm

    size_t remove_if (A* self, int match(T*))

Removes all elements satisfying specific criteria.

    I find_if (A* self, int match(T*))

finds element by predicate

    I find_if_not (A* self, int match(T*))

finds element by predicate

    A intersection (A* self, A* other)
    A union (A* self, A* other)
    A difference (A* self, A* other)
    A symmetric_difference (A* self, A* other)

Specialized set algorithm variants which do work here, unlike with the STL.

    bool all_of (A* self, int match(T*))
    bool any_of (A* self, int match(T*))
    bool none_of (A* self, int match(T*))

    generate (A* self, T _gen(void))
    generate_n (A* self, size_t n, T _gen(void))
    A transform (A* self, T unop(T*))

Unlike with the STL, `generate_n` shrinks to n elements.

See [algorithm](algorithm.md) for more.

# Performance

Compared to the STL, our `CTL_USET_GROWTH_POWER2` policy does not use the slow
`%` modulo operator in the hot loop.  Using Lemire's fastmod hack turned out to
be worse, so it's not yet enabled.  `CTL_USET_CACHED_HASH` is faster with high
load factors.  Overall the STL `unordered_set` performance is bad, and our is
also not much better.  That's why we will add better hash tables and a btree,
with less stability guarantees.

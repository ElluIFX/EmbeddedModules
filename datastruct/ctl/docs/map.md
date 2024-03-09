# map - CTL - C Container Template library

Defined in header **<ctl_map.h>**, CTL prefix **map**,
deriving from [set](set.md).

## SYNOPSIS

    typedef struct {
      char *key;
      int value;
    } charint;

    static inline int
    charint_cmp(charint *a, charint *b) {
      return strcmp(a->key, b->key);
    }
    static inline void
    charint_free(charint *a) {
      free(a->key);
    }
    static inline charint
    charint_copy(charint *self) {
      char *copy_key = (char*) CTL_MALLOC(strlen(self->key) + 1);
      strcpy (copy_key, self->key);
      charint copy = {
        copy_key,
        self->value,
      };
      return copy;
    }

    #define T charint
    #include <ctl_map.h>

    map_charint a = map_charint_init(charint_compare);

    char c_char[36];
    for (int i=0; i<1000; i++) {
      snprintf(c_char, 36, "%c%d", 48 + (rand() % 74), rand());
      map_charint_insert(&a, charint_copy(&(charint){ c_char, i }));
    }

    foreach(map_charint, &a, it) { strcpy (c_char, it.ref->key); }
    printf("last key \"%s\", ", c_char);

    map_charint_node *b = map_charint_begin(&a);
    printf("min {\"%s\", %d} ", b->key.key, b->key.value);
    b = map_charint_node_max(b);
    printf("max {\"%s\", %d}\n", b->key.key, b->key.value);

    map_charint_free(&a);

## DESCRIPTION

map is a sorted associative container that contains key-value pairs with unique
keys. Keys are sorted by using the comparison function Compare. Search, removal,
and insertion operations have logarithmic complexity. map is an extended set
with a struct as value, and implemented as red-black trees.

The function names are composed of the prefix **map_**, the user-defined type
**T** and the method name. E.g `map_charint` with `#define T charint`.

Everywhere the CTL uses the Compare requirements, uniqueness is
determined by using the equivalence relation. In imprecise terms, two objects a
and b are considered equivalent (not unique) if neither compares less than the
other: !comp(a, b) && !comp(b, a).

## Member types

`T`                     value type

`A` being `map_T`       container type

`B` being `map_T_node`  node type

`I` being `map_T_it`    iterator type

## Member functions

    A init (int compare(T*, T*))

constructs the map

    free (A* self)

destructs the map

    assign (A* self, A* other)

replaces the contents of the container

    get_allocator (A* self)

returns the associated allocator

## Element access

    T* at (A* self, size_t index)

access specified element with bounds checking

## Iterators

    I begin (A* self)

constructs an iterator to the beginning

    I end (A* self)

constructs an iterator to the end

    I* next (I* iter)

Advances the iterator by 1 forwards. There's no prev yet.

    I* advance (I* iter, long i)

All our variants accepts negative `i` to move back. The return value may be ignored.

See [iterators](iterators.md) for more.

## Capacity

    int empty (A* self)

checks whether the container is empty

    size_t size (A* self)

returns the number of elements

    size_t max_size ()

returns the maximum possible number of elements

## Modifiers

    clear (A* self)

clears the contents

    insert (A* self, T pair)

inserts a key-value pair (since C++17)

    insert_or_assign (A* self, T pair)

inserts an element or assigns to the current element if the key already exists

    emplace (A* self, T* pair)

constructs element in-place

    emplace_hint (I *pos, T* pair)

constructs elements in-place at position. _(NYI)_

    try_emplace (A* self, T* pair)

inserts in-place if the key does not exist, does nothing if the key exists

    erase (A* self, T key)

erases the element by pair.first

    erase_it (I* pos)

erases the element at position

    erase_range (I* range)

erases elements

    swap (A* self, A* other)

swaps the contents

    extract (A* self, T key)

extracts a node from the container. NYI

    extract_it (I* pos)

extracts a node from the container. NYI

    merge (A* self, A* other)

merges two containers

## Lookup

    size_t count (A* self, T key)

returns the number of elements matching specific key

    I find (A* self, T key)
    B* find_node (A* self, T key)

finds element with specific key, i.e. pair.first

    bool contains (A* self, T key)

checks if the container contains element with specific key. (C++20)

## Observers

    int (*cmp)(T*,T*) value_comp (A* self)

Returns the function that compares keys in objects of type T.  _(NYI)_

## Non-member functions

    swap (A* self, A* other)

specializes the swap algorithm

    size_t remove_if (A* self, int match(T*))
    size_t erase_if (A* self, int (*_match)(T*))

Remove all elements satisfying specific criteria.

    A intersection (A* self, A* other)
    A union (A* self, A* other)
    A difference (A* self, A* other)
    A symmetric_difference (A* self, A* other)

See [algorithm](algorithm.md) for more.

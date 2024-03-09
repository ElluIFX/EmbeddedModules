# iterators - CTL - C Container Template library

Defined automatically for all containers, CTL suffix **_it**.

## SYNOPSIS

    #define POD
    #define T int
    #include <ctl_list.h>

    list_int a = list_int_init();
    // ...
    list_int_it first = list_digi_begin(&a);
    long i1 = rand() % a.size;
    list_int_it_advance(&first, i1);
    printf("first: [%ld, ", list_int_it_distance(list_digi_begin(&a), &first);

    list_int_it last = list_digi_end(&a);
    long i2 = i1 + (rand() % (a.size + i1));
    list_int_it_advance(&last, -i2);
    printf("%ld)\n", list_int_it_distance(list_digi_begin(&a), &last);

    printf("values: (%d, %d)\n", *first.ref. *last.ref);

    // restrict first to last (optional)
    list_int_it_range(&first, &last);

    some_method_range(first, last);
    // set the current position to the end position
    list_int_set_done(&first);

## DESCRIPTION

Iterators hold state for all containers, i.e. the container, the current
position, the end position and additional private fields per container to
support its methods.

Some iterators advance on linked nodes (_"B iters"_), some others on value
refs (_"T iters"_). The deque additionally holds the `index`, the unordered_set
the `buckets` pointer.

Each iterator also holds the end position so we can hold full ranges. And we
added API's to work with the addional end position.

We also support for certain algorithm methods generic iterators as 2nd range,
abstracting different containers. So we can insert a vector into a deque, or use
a mixed set algorithm with different container types. They are denoted as `GI*`,
generic iters, and can be simply casted from container-specific iterators.

We don't fully support **output iterators**, like `back_inserter` or `inserter` yet.
They are currently only defined for some algorithms, and are problematic for `set`.

We don't fully support `reverse_iterator` via `I prev` yet.

## Iterators

    I begin (A* self)

Constructs an iterator to the begin.

    I end (A* self)

Constructs an iterator to the end.

    int done (I* iter)

returns 1 if the iterator reached its end. With ranges this might not be the container end.

    I* next (I* iter)

Advances the iterator by 1 forwards. There's no prev yet.

    I* advance (I* iter, long i)

All our variants accepts negative `i` to move back. The return value may be ignored.

    long distance (I* first, I* last)

When first is not before last, list returns -1, the other containers wrap around and
return negative numbers.

    range (I* first, I* last)

range sets the first and last end positions. For `unordered_set` the API
is still the old `(A* container, B* begin, B* end)`, but we don't support
algorithm iterators on ranges, as they make no sense for unordered containers.

    T* ref (I* pos)

returns the value reference.

    set_pos (I* iter, I* other)

sets the position to the position of the other. Cheaper than copying the whole iterator.

    set_done (I* iter)

sets the position to the end, so it's done.

    set_end (I* iter, I* last)

sets the end field from the current position in last.

    I* advance_end (I* iter, long i)

Advance the end position.

    I iter (I* iter, size_t index/B* node)

Creates a new iterator at the initial position.

    size_t index (I* iter)

Returns the distance from begin.

    size_t distance_range (I* range)

Returns the distance between range and `range->end`.

# Performance

Compared to the old ctl, our iterators are about twice as fast, just our
`unordered_set` iterator is a bit slower.

Our `set` is O(1), whilst the STL set iterator is logarithmic, the rest is as
fast as in the STL.

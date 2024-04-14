/* Fixed-size.
   SPDX-License-Identifier: MIT */

#ifndef T
#error "Template type T undefined for <./array.h>"
#endif
#ifndef N
#error "Size N undefined for <ctl_array.h>"
#endif
#if N < 0 || N > (4294967296 / 8)
#error "Size N invalid for <ctl_array.h>"
#endif

#include "ctl.h"

// stack allocated if N < 2048, else heap. FIXME: 4k / sizeof(T)
#define CUTOFF 2047
#define CTL_ARR
#define C PASTE(arr, N)
#define A JOIN(C, T)
#define I JOIN(A, it)
#define GI JOIN(A, it)

typedef struct A {
#if N > CUTOFF
    T* vector;
#else
    T vector[N];
#endif
    void (*free)(T*);
    T (*copy)(T*);
    int (*compare)(T*, T*);  // 2-way operator<
    int (*equal)(T*, T*);    // optional
} A;

typedef int (*JOIN(A, compare_fn))(T*, T*);

#include <./bits/iterator_vtable.h>

typedef struct I {
    CTL_T_ITER_FIELDS;
} I;

#include <./bits/iterators.h>

static inline I JOIN(I, iter)(A* self, size_t index);

static inline size_t JOIN(A, size)(A* self) {
    (void)self;
    return N;
}

static inline int JOIN(A, empty)(A* self) {
    (void)self;
    return N == 0;
}

static inline size_t JOIN(A, max_size)(void) {
    return N;
}

static inline T* JOIN(A, at)(A* self, size_t index) {
#if defined(_ASSERT_H) && !defined(NDEBUG)
    assert(index < N || !"out of range");
#endif
    return index < N ? &self->vector[index] : NULL;
}

static inline T JOIN(A, get)(A* self, size_t index) {
#if defined(_ASSERT_H) && !defined(NDEBUG)
    assert(index < N || !"out of range");
#endif
    return self->vector[index];
}

static inline T* JOIN(A, front)(A* self) {
    return &self->vector[0];  // not bounds-checked
}

static inline T* JOIN(A, back)(A* self) {
    return &self->vector[N - 1];
}

static inline I JOIN(A, begin)(A* self) {
    return JOIN(I, iter)(self, 0);
}

static inline I JOIN(A, end)(A* self) {
    return JOIN(I, iter)(self, N);
}

static inline T* JOIN(I, ref)(I* iter) {
    return iter->ref;
}

static inline size_t JOIN(I, index)(I* iter) {
    return iter->ref - JOIN(A, front)(iter->container);
}

static inline int JOIN(I, done)(I* iter) {
    return iter->ref == iter->end;
}

static inline void JOIN(I, set_done)(I* iter) {
    iter->ref = iter->end;
}

static inline void JOIN(I, set_pos)(I* iter, I* other) {
    iter->ref = other->ref;
}

static inline void JOIN(I, next)(I* iter) {
    iter->ref++;
}

static inline void JOIN(I, prev)(I* iter) {
    iter->ref--;
}

static inline void JOIN(I, range)(I* first, I* last) {
    last->end = first->end = last->ref;
}

static inline void JOIN(I, set_end)(I* iter, I* last) {
    iter->end = last->ref;
}

static inline I* JOIN(I, advance)(I* iter, long i) {
    // error case: overflow => end or NULL?
    if (iter->ref + i > iter->end ||
        iter->ref + i < JOIN(A, front)(iter->container))
        iter->ref = iter->end;
    else
        iter->ref += i;
    return iter;
}

// advance end only (*_n algos)
static inline void JOIN(I, advance_end)(I* iter, long n) {
    if (iter->ref + n <= iter->end &&
        iter->ref + n >= JOIN(A, front)(iter->container))
        iter->end += n;
}

static inline long JOIN(I, distance)(I* iter, I* other) {
    return other->ref - iter->ref;
}

static inline long JOIN(I, distance_range)(I* range) {
    return range->end - range->ref;
}

static inline A JOIN(A, init_from)(A* copy);
static inline A JOIN(A, copy)(A* self);

#include <./bits/container.h>

static inline I JOIN(I, iter)(A* self, size_t index) {
    static I zero;
    I iter = zero;
    iter.ref = &self->vector[index];
    iter.container = self;
    iter.end = &self->vector[N];
    // iter.vtable = { JOIN(I, next), JOIN(I, ref), JOIN(I, done) };
    iter.vtable.next = JOIN(I, next);
    iter.vtable.ref = JOIN(I, ref);
    iter.vtable.done = JOIN(I, done);
    return iter;
}

static inline A JOIN(A, init)(void) {
    static A zero;
    A self = zero;
#if N > CUTOFF
    self.vector = (T*)CTL_CALLOC(N, sizeof(T));
#else
    memset(self.vector, 0, N * sizeof(T));
#endif
#ifdef POD
    self.copy = JOIN(A, implicit_copy);
    _JOIN(A, _set_default_methods)(&self);
#else
    self.free = JOIN(T, free);
    self.copy = JOIN(T, copy);
#endif
    return self;
}

static inline A JOIN(A, init_from)(A* copy) {
    static A zero;
    A self = zero;
    self.free = copy->free;
    self.copy = copy->copy;
    self.compare = copy->compare;
    self.equal = copy->equal;
    return self;
}

static inline int JOIN(A, zero)(T* ref) {
#ifndef POD
    static T zero;
    return memcmp(ref, &zero, sizeof(T)) == 0;
#else
    (void)ref;
    return 1;
#endif
}

// not bounds-checked. like operator[]
static inline void JOIN(A, set)(A* self, size_t index, T value) {
    T* ref = &self->vector[index];
#ifndef POD
    if (self->free && !JOIN(A, zero)(ref))
        self->free(ref);
#endif
    *ref = value;
}

static inline void JOIN(A, fill)(A* self, T value) {
#if defined(POD) && !defined(NOT_INTEGRAL)
    if (sizeof(T) <= sizeof(char))  // only for bytes
        memset(self->vector, value, N * sizeof(T));
    else
        for (size_t i = 0; i < N; i++)
            self->vector[i] = value;
#else
    for (size_t i = 0; i < N; i++)
        JOIN(A, set)(self, i, self->copy(&value));
#endif
#ifndef POD
    if (self->free)
        self->free(&value);
#endif
}

static inline void JOIN(A, fill_range)(I* range, T value) {
    size_t n = JOIN(I, distance_range)(range);
#if defined(POD) && !defined(NOT_INTEGRAL)
    if (sizeof(T) <= sizeof(char))  // only for bytes
        memset(range->ref, value, n * sizeof(T));
    else
        for (I it = *range; it.ref < it.end; it.ref++)
            *it.ref = value;
#else
    A* self = range->container;
    for (size_t i = JOIN(I, index)(range); i < n; i++)
        JOIN(A, set)(self, i, self->copy(&value));
#endif
#ifndef POD
#ifdef NOT_INTEGRAL
    A* self = range->container;
#endif
    if (self->free)
        self->free(&value);
#endif
}

static inline void JOIN(A, fill_zero)(I* range) {
    static T zero;
#ifdef POD
    if (sizeof(T) <= sizeof(char))  // only for bytes
    {
        memset(range->ref, 0, JOIN(I, distance_range)(range) * sizeof(T));
        return;
    }
#endif
    for (I it = *range; it.ref < it.end; it.ref++)
        *it.ref = zero;
}

static inline void JOIN(A, fill_n)(A* self, size_t n, T value) {
    if (n >= N)
        return;
#if defined(POD) && !defined(NOT_INTEGRAL)
    if (sizeof(T) <= sizeof(char))
        memset(self->vector, value, n * sizeof(T));
    else
        for (size_t i = 0; i < n; i++)
            self->vector[i] = value;
#else
    for (size_t i = 0; i < n; i++)
        JOIN(A, set)(self, i, self->copy(&value));
#endif
#ifndef POD
    if (self->free)
        self->free(&value);
#endif
}

#ifdef POD
static inline void JOIN(A, clear)(A* self) {
#ifndef NOT_INTEGRAL
    memset(self->vector, 0, N * sizeof(T));
#else
    static T zero;
    JOIN(A, fill)(self, zero);
#endif
}
#endif

static inline void JOIN(A, free)(A* self) {
#ifndef POD
    if (self->free)
        for (size_t i = 0; i < N; i++) {
            T* ref = &self->vector[i];
            if (!JOIN(A, zero)(ref))
                self->free(ref);
            else
                break;
        }
#endif
        // for security reasons?
        // memset (self->vector, 0, N * sizeof(T));
#if N > CUTOFF
    CTL_FREE(self->vector);  // heap allocated
    self->vector = NULL;
#else
    (void)self;
#endif
}

static inline void JOIN(A, assign)(A* self, size_t count, T value) {
    for (size_t i = 0; i < count; i++)
        JOIN(A, set)(self, i, self->copy(&value));
#ifndef POD
    if (self->free)
        self->free(&value);
#endif
}

static inline void JOIN(A, assign_range)(A* self, T* from, T* last) {
    size_t l = last - JOIN(A, front)(self);
    if (l >= N)
        l = N;
    for (size_t i = from - JOIN(A, front)(self); i < l; i++)
        JOIN(A, set)(self, i, *JOIN(A, at)(self, i));
}

static inline void JOIN(A, assign_generic)(A* self, GI* range) {
    size_t i = 0;
    void (*next)(struct I*) = range->vtable.next;
    T* (*ref)(struct I*) = range->vtable.ref;
    int (*done)(struct I*) = range->vtable.done;

    while (!done(range) && i < N) {
        JOIN(A, set)(self, i++, self->copy(ref(range)));
        next(range);
    }
}

static inline T* JOIN(A, data)(A* self) {
    return JOIN(A, front)(self);
}

static inline void JOIN(A, swap)(A* self, A* other) {
    A temp = *self;
    *self = *other;
    *other = temp;
}

static inline void JOIN(A, _ranged_sort)(A* self, size_t a, size_t b,
                                         int _compare(T*, T*)) {
    if (UNLIKELY(a >= b))
        return;
    // TODO insertion_sort cutoff
    // long mid = (a + b) / 2; // overflow!
    // Dietz formula http://aggregate.org/MAGIC/#Average%20of%20Integers
    size_t mid = ((a ^ b) >> 1) + (a & b);
    // CTL_LOG("sort \"%s\" %ld, %ld\n", self->vector, a, b);
    SWAP(T, &self->vector[a], &self->vector[mid]);
    size_t z = a;
    // check overflow of a + 1
    if (LIKELY(a + 1 > a))
        for (size_t i = a + 1; i <= b; i++)
            if (_compare(&self->vector[i], &self->vector[a])) {
                z++;
                SWAP(T, &self->vector[z], &self->vector[i]);
            }
    SWAP(T, &self->vector[a], &self->vector[z]);
    if (LIKELY(z))
        JOIN(A, _ranged_sort)(self, a, z - 1, _compare);
    // check overflow of z + 1
    if (LIKELY(z + 1 > z))
        JOIN(A, _ranged_sort)(self, z + 1, b, _compare);
}

static inline void JOIN(A, sort)(A* self) {
    CTL_ASSERT_COMPARE
    // TODO insertion_sort cutoff
    if (LIKELY(N > 1))
        JOIN(A, _ranged_sort)(self, 0, N - 1, self->compare);
}

static inline A JOIN(A, copy)(A* self) {
    A other = JOIN(A, init)();
#ifdef POD
    memcpy(other.vector, self->vector, N * sizeof(T));
#else
    for (size_t i = 0; i < N; i++)
        JOIN(A, set)(&other, i, self->copy(&self->vector[i]));
#endif
    return other;
}

static inline T* JOIN(A, find)(A* self, T key) {
    ctl_foreach(A, self,
                it) if (JOIN(A, _equal)(self, it.ref, &key)) return it.ref;
    return NULL;
}

static inline A JOIN(A, transform_it)(A* self, I* pos, T _binop(T*, T*)) {
    A other = JOIN(A, init)();
    size_t i = 0;
    ctl_foreach(A, self, it) {
        if (pos->ref == pos->end)
            break;
#ifdef POD
        JOIN(A, set)(&other, i, _binop(it.ref, pos->ref));
#else
        T copy = self->copy(it.ref);
        T tmp = _binop(&copy, pos->ref);
        JOIN(A, set)(&other, i, tmp);
        if (self->free)
            self->free(&copy);
#endif
        i++;
        pos->ref++;
    }
    return other;
}

static inline A JOIN(A, copy_if_range)(I* range, int _match(T*)) {
    A out = JOIN(A, init_from)(range->container);
    size_t i = JOIN(I, index)(range);
    while (!JOIN(I, done)(range)) {
        if (_match(range->ref))
            JOIN(A, set)(&out, i, out.copy(range->ref));
        i++;
        JOIN(I, next)(range);
    }
    return out;
}

static inline A JOIN(A, copy_if)(A* self, int _match(T*)) {
    A out = JOIN(A, init_from)(self);
    I range = JOIN(A, begin)(self);
    size_t i = 0;
    while (!JOIN(I, done)(&range)) {
        if (_match(range.ref))
            JOIN(A, set)(&out, i, out.copy(range.ref));
        i++;
        JOIN(I, next)(&range);
    }
    return out;
}

// Set algos return a custom range, not the full array.
// Add to it and return the range, with it->end set.
// All refs after end are invalid.
static inline I JOIN(A, copy_range)(I* it, GI* from) {
    A* self = it->container;
    A* other = from->container;
    void (*next2)(struct I*) = from->vtable.next;
    T* (*ref2)(struct I*) = from->vtable.ref;
    int (*done2)(struct I*) = from->vtable.done;

    if (!JOIN(I, done)(it)) {
        while (!done2(from)) {
            *it->ref = other->copy(ref2(from));
            JOIN(I, next)(it);
            next2(from);
        }
    }
    // JOIN(A, fill_zero)(it);
    it->end = it->ref;
    it->ref = &self->vector[0];
    // memset(it->end, 0, (N - (it->end - it->ref)) * sizeof(T));
    return *it;
}

// These three set algos return an iterator pointing to the end of the new
// result container.
// All values starting with end are zeroe'd and invalid, esp. with non-POD
// values. The resulting container is always heap-allocated, not stack.
static inline I JOIN(A, intersection_range)(I* r1, GI* r2) {
    A* self = (A*)CTL_CALLOC(1, sizeof(A));
    I it;
    void (*next2)(struct I*) = r2->vtable.next;
    T* (*ref2)(struct I*) = r2->vtable.ref;
    int (*done2)(struct I*) = r2->vtable.done;
    *self = JOIN(A, init)();
    self->free = r1->container->free;
    self->copy = r1->container->copy;
    self->compare = r1->container->compare;
    self->equal = r1->container->equal;
    it = JOIN(A, begin)(self);

    while (!JOIN(I, done)(r1) && !done2(r2)) {
        if (self->compare(r1->ref, ref2(r2)))
            JOIN(I, next)(r1);
        else {
            if (!self->compare(ref2(r2), r1->ref)) {
                *it.ref = self->copy(r1->ref);
                JOIN(I, next)(&it);
                JOIN(I, next)(r1);
            }
            next2(r2);
        }
    }
    // JOIN(A, fill_zero)(&it);
    it.end = it.ref;
    it.ref = &self->vector[0];
    // memset(it.end, 0, (N - (it.end - it.ref)) * sizeof(T));
    return it;
}

// Warning: fails with 3-way compare! And with generic r2 also.
static inline I JOIN(A, difference_range)(I* r1, I* r2) {
    // make the resulting container heap-allocated, not stack
    A* self = (A*)CTL_CALLOC(1, sizeof(A));
    I it;
    void (*next2)(struct I*) = r2->vtable.next;
    T* (*ref2)(struct I*) = r2->vtable.ref;
    int (*done2)(struct I*) = r2->vtable.done;
    *self = JOIN(A, init)();
    self->free = r1->container->free;
    self->copy = r1->container->copy;
    self->compare = r1->container->compare;
    self->equal = r1->container->equal;
    it = JOIN(A, begin)(self);

    while (!JOIN(I, done)(r1)) {
        if (done2(r2))
            return JOIN(A, copy_range)(&it, r1);
        // r1 < r2 (fails with 3-way compare)
        if (self->compare(r1->ref, ref2(r2))) {
            *it.ref = self->copy(r1->ref);
            JOIN(I, next)(&it);
            JOIN(I, next)(r1);
        } else {
            if (!self->compare(ref2(r2), r1->ref))
                JOIN(I, next)(r1);
            next2(r2);
        }
    }
    // JOIN(A, fill_zero)(&it);
    it.end = it.ref;
    it.ref = &self->vector[0];
    // memset(it.end, 0, (N - (it.end - it.ref)) * sizeof(T));
    return it;
}

static inline I JOIN(A, symmetric_difference_range)(I* r1, GI* r2) {
    // make the resulting container heap-allocated, not stack
    A* self = (A*)CTL_CALLOC(1, sizeof(A));
    I it;
    void (*next2)(struct I*) = r2->vtable.next;
    T* (*ref2)(struct I*) = r2->vtable.ref;
    int (*done2)(struct I*) = r2->vtable.done;
    *self = JOIN(A, init)();
    self->free = r1->container->free;
    self->copy = r1->container->copy;
    self->compare = r1->container->compare;
    self->equal = r1->container->equal;
    it = JOIN(A, begin)(self);

    while (!JOIN(I, done)(r1)) {
        if (done2(r2))
            return JOIN(A, copy_range)(&it, r1);
        if (self->compare(r1->ref, ref2(r2))) {
            *it.ref = self->copy(r1->ref);
            JOIN(I, next)(&it);
            JOIN(I, next)(r1);
        } else {
            if (self->compare(ref2(r2), r1->ref)) {
                *it.ref = self->copy(ref2(r2));
                JOIN(I, next)(&it);
            } else
                JOIN(I, next)(r1);
            next2(r2);
        }
    }
    return JOIN(A, copy_range)(&it, r2);
}

#undef A
#undef I
#undef GI
#undef N
#undef CUTOFF

#undef C
#undef T
#undef POD
#undef NOT_INTEGRAL
#undef CTL_ARR

#ifndef __MACRO_H__
#define __MACRO_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// for IAR
#undef __IS_COMPILER_IAR__
#if defined(__IAR_SYSTEMS_ICC__)
#define __IS_COMPILER_IAR__ 1
#endif

// for arm compiler 5
#undef __IS_COMPILER_ARM_COMPILER_5__
#if ((__ARMCC_VERSION >= 5000000) && (__ARMCC_VERSION < 6000000))
#define __IS_COMPILER_ARM_COMPILER_5__ 1
#endif

// for arm compiler 6

#undef __IS_COMPILER_ARM_COMPILER_6__
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define __IS_COMPILER_ARM_COMPILER_6__ 1
#endif
#undef __IS_COMPILER_ARM_COMPILER__
#if defined(__IS_COMPILER_ARM_COMPILER_5__) && \
        __IS_COMPILER_ARM_COMPILER_5__ ||      \
    defined(__IS_COMPILER_ARM_COMPILER_6__) && __IS_COMPILER_ARM_COMPILER_6__
#define __IS_COMPILER_ARM_COMPILER__ 1
#endif

// for clang
#undef __IS_COMPILER_LLVM__
#if defined(__clang__) && !__IS_COMPILER_ARM_COMPILER_6__
#define __IS_COMPILER_LLVM__ 1
#else

// for gcc
#undef __IS_COMPILER_GCC__
#if defined(__GNUC__) &&                       \
    !(defined(__IS_COMPILER_ARM_COMPILER__) || \
      defined(__IS_COMPILER_LLVM__) || defined(__IS_COMPILER_IAR__))
#define __IS_COMPILER_GCC__ 1
#endif

#endif

#define __PLOOC_VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                                 _11, _12, _13, _14, _15, _16, __N, ...)      \
    __N

#define __PLOOC_VA_NUM_ARGS(...)                                              \
    __PLOOC_VA_NUM_ARGS_IMPL(0, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, \
                             8, 7, 6, 5, 4, 3, 2, 1, 0)

// 各种数量的连接宏
#define __CONNECT2(__A, __B) __A##__B
#define __CONNECT3(__A, __B, __C) __A##__B##__C
#define __CONNECT4(__A, __B, __C, __D) __A##__B##__C##__D
#define __CONNECT5(__A, __B, __C, __D, __E) __A##__B##__C##__D##__E
#define __CONNECT6(__A, __B, __C, __D, __E, __F) __A##__B##__C##__D##__E##__F
#define __CONNECT7(__A, __B, __C, __D, __E, __F, __G) \
    __A##__B##__C##__D##__E##__F##__G
#define __CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H) \
    _A##__B##__C##__D##__E##__F##__G##__H
#define __CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I) \
    __A##__B##__C##__D##__E##__F##__G##__H##__I
#define CONNECT2(__A, __B) __CONNECT2(__A, __B)
#define CONNECT3(__A, __B, __C) __CONNECT3(__A, __B, __C)
#define CONNECT4(__A, __B, __C, __D) __CONNECT4(__A, __B, __C, __D)
#define CONNECT5(__A, __B, __C, __D, __E) __CONNECT5(__A, __B, __C, __D, __E)
#define CONNECT6(__A, __B, __C, __D, __E, __F) \
    __CONNECT6(__A, __B, __C, __D, __E, __F)
#define CONNECT7(__A, __B, __C, __D, __E, __F, __G) \
    __CONNECT7(__A, __B, __C, __D, __E, __F, __G)
#define CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H) \
    __CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H)
#define CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I)

/**
 * @brief 获取可变参数个数
 */
#define VA_NUM_ARGS(...) __PLOOC_VA_NUM_ARGS(__VA_ARGS__)

/**
 * @brief 安全的局部变量名
 */
#define SAFE_NAME(__NAME) CONNECT3(__, __NAME, __LINE__)

/**
 * @brief 连接宏
 */
#define CONNECT(...)                            \
    CONNECT2(CONNECT, VA_NUM_ARGS(__VA_ARGS__)) \
    (__VA_ARGS__)

/**
 * @brief 选择宏, 根据参数个数N调用对应的__FUMC_N
 */
#define EVAL(__FUNC_, ...) CONNECT2(__FUNC_, VA_NUM_ARGS(__VA_ARGS__))

#define __using_1(__declare)                      \
    for (__declare, *SAFE_NAME(using_ptr) = NULL; \
         SAFE_NAME(using_ptr)++ == NULL;)

#define __using_2(__declare, __on_leave_expr)     \
    for (__declare, *SAFE_NAME(using_ptr) = NULL; \
         SAFE_NAME(using_ptr)++ == NULL; __on_leave_expr)

#define __using_3(__declare, __on_enter_expr, __on_leave_expr)        \
    for (__declare, *SAFE_NAME(using_ptr) = NULL;                     \
         SAFE_NAME(using_ptr)++ == NULL ? ((__on_enter_expr), 1) : 0; \
         __on_leave_expr)

#define __using_4(__dcl1, __dcl2, __on_enter_expr, __on_leave_expr)   \
    for (__dcl1, __dcl2, *SAFE_NAME(using_ptr) = NULL;                \
         SAFE_NAME(using_ptr)++ == NULL ? ((__on_enter_expr), 1) : 0; \
         (__on_leave_expr))

/**
 * @brief 局部变量
 * @param __declare 局部变量
 * @param __on_enter_expr 进入操作 [可选]
 * @param __on_leave_expr 离开操作 [可选]
 */
#define using(...)              \
    EVAL(__using_, __VA_ARGS__) \
    (__VA_ARGS__)

/**
 * @brief 原子操作代码块
 */
#define SAFE_ATOM_CODE                                     \
    using(uint32_t SAFE_NAME(temp) = ({                    \
              uint32_t SAFE_NAME(temp2) = __get_PRIMASK(); \
              __disable_irq();                             \
              SAFE_NAME(temp2);                            \
          }),                                              \
          __set_PRIMASK(SAFE_NAME(temp)))

#define UNUSED_PARAM(__VAR) (void)(__VAR)
#define __IRQ_SAFE SAFE_ATOM_CODE

#define __dim_of_1(__array) (sizeof(__array) / sizeof((__array[0]))
#define __dim_of_2(__array, __type) (sizeof(__array) / sizeof(__type))
/**
 * @brief 获取数组长度
 * @param __array 数组
 * @param __type 元素类型 (可选)
 */
#define dim_of(...)              \
    EVAL(__dim_of_, __VA_ARGS__) \
    (__VA_ARGS__)

#define __foreach_2(__array, __type)                                \
    using(__type * _ = __array) for (uint_fast32_t SAFE_NAME(cnt) = \
                                         dimof(__array, __type);    \
                                     SAFE_NAME(cnt) > 0;            \
                                     _++, SAFE_NAME(cnt)--)

#define __foreach_1(__array) __foreach_2(__array, typeof(*(__array)))
#define __foreach_3(__array, __type, __pt)                            \
    using(__type * __pt =                                             \
              __array) for (uint_fast32_t CONNECT2(count, __LINE__) = \
                                dimof(__array, __type);               \
                            SAFE_NAME(cnt) > 0; __pt++, SAFE_NAME(cnt)--)
#define __foreach_reverse_2(__array, __type)                  \
    using(__type * _ = __array + dimof(__array, __type) -     \
                       1) for (uint_fast32_t SAFE_NAME(cnt) = \
                                   dimof(__array, __type);    \
                               SAFE_NAME(cnt) > 0; _--, SAFE_NAME(cnt)--)
#define __foreach_reverse_1(__array) \
    __foreach_reverse_2(__array, typeof(*(__array)))
#define __foreach_reverse_3(__array, __type, __pt)                           \
    using(__type * __pt =                                                    \
              __array + dimof(__array, __type) -                             \
              1) for (uint_fast32_t SAFE_NAME(cnt) = dimof(__array, __type); \
                      SAFE_NAME(cnt) > 0; __pt--, SAFE_NAME(cnt)--)
/**
 * @brief 遍历数组
 * @param __array 数组
 * @param __type 元素类型 (可选)
 * @param __pt 元素指针名 (可选)
 */
#define foreach(...)              \
    EVAL(__foreach_, __VA_ARGS__) \
    (__VA_ARGS__)

/**
 * @brief 反向遍历数组
 * @param __array 数组
 * @param __type 元素类型 (可选)
 * @param __pt 元素指针名 (可选)
 */
#define foreach_reverse(...)              \
    EVAL(__foreach_reverse_, __VA_ARGS__) \
    (__VA_ARGS__)

/**
 * @brief Get the absolute value of the specified value
 */
#define CABS(x) ((x) >= 0 ? (x) : -(x))

#define __MIN_2(__a, __b) ((__a) < (__b) ? (__a) : (__b))
#define __MIN_3(__a, __b, __c) __MIN_2(__MIN_2(__a, __b), __c)
#define __MIN_4(__a, __b, __c, __d) \
    __MIN_2(__MIN_2(__a, __b), __MIN_2(__c, __d))

/**
 * @brief Get the minimum value of the specified values
 */
#define CMIN(...)             \
    EVAL(__MIN_, __VA_ARGS__) \
    (__VA_ARGS__)

#define __MAX_2(__a, __b) ((__a) > (__b) ? (__a) : (__b))
#define __MAX_3(__a, __b, __c) __MAX_2(__MAX_2(__a, __b), __c)
#define __MAX_4(__a, __b, __c, __d) \
    __MAX_2(__MAX_2(__a, __b), __MAX_2(__c, __d))

/**
 * @brief Judge whether the specified value is close to the specified value
 */
#define CEQUAL(__a, __b, __eps) (CABS((__a) - (__b)) < (__eps))

/**
 * @brief Get the maximum value of the specified values
 */
#define CMAX(...)             \
    EVAL(__MAX_, __VA_ARGS__) \
    (__VA_ARGS__)

/**
 * @brief Round a float to the nearest integer
 */
#define ROUND(__f) ((int)((__f) + 0.5f))

/**
 * @brief Clamp a value to the specified range
 */
#define CLAMP(__x, __min, __max) CMIN(CMAX(__x, __min), __max)

/**
 * @brief Linear mapping input to the specified range
 */
#define MAP(__x, __in_min, __in_max, __out_min, __out_max)                \
    ((__x - __in_min) * (__out_max - __out_min) / (__in_max - __in_min) + \
     __out_min)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

/**
 * @brief make compiler know the expression is likely to be true
 */
#define likeyly(x) __builtin_expect(!!(x), 1)

/**
 * @brief make compiler know the expression is likely to be false
 */
#define unlikely(x) __builtin_expect(!!(x), 0)

#define TRIGGER_DEBUG_HALT()    \
    if (CoreDebug->DHCSR & 1) { \
        __breakpoint(0);        \
    }

/*
 * List code bynbulischeck
 * https://github.com/nbulischeck/list.h/blob/master/src/list.h
 */

/* Singly-linked Lists */

#define SL_EMPTY(head) ((head == NULL) ? 1 : 0);

#define SL_APPEND(head, entry)            \
    do {                                  \
        if (head) {                       \
            __typeof__(head) _tmp = head; \
            while (_tmp->next != NULL) {  \
                _tmp = _tmp->next;        \
            }                             \
            _tmp->next = entry;           \
        } else {                          \
            head = entry;                 \
        }                                 \
    } while (0)

#define SL_PREPEND(head, entry) \
    do {                        \
        if (head)               \
            entry->next = head; \
        head = entry;           \
    } while (0)

/*
 * Sorting algorithm by Simon Tatham
 * https://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html
 */

#define SL_SORT(head, cmp)                                     \
    do {                                                       \
        __typeof__(head) _p, _q, _e, _tail;                    \
        int _insize, _nmerges, _psize, _qsize, _i;             \
        if (head) {                                            \
            _insize = 1;                                       \
            while (1) {                                        \
                _p = head;                                     \
                head = NULL;                                   \
                _tail = NULL;                                  \
                _nmerges = 0;                                  \
                while (_p) {                                   \
                    _nmerges++;                                \
                    _q = _p;                                   \
                    _psize = 0;                                \
                    for (_i = 0; _i < _insize; _i++) {         \
                        _psize++;                              \
                        _q = _q->next;                         \
                        if (!_q)                               \
                            break;                             \
                    }                                          \
                    _qsize = _insize;                          \
                    while (_psize > 0 || (_qsize > 0 && _q)) { \
                        if (_psize == 0) {                     \
                            _e = _q;                           \
                            _q = _q->next;                     \
                            _qsize--;                          \
                        } else if (_qsize == 0 || !_q) {       \
                            _e = _p;                           \
                            _p = _p->next;                     \
                            _psize--;                          \
                        } else if (cmp(_p, _q) <= 0) {         \
                            _e = _p;                           \
                            _p = _p->next;                     \
                            _psize--;                          \
                        } else {                               \
                            _e = _q;                           \
                            _q = _q->next;                     \
                            _qsize--;                          \
                        }                                      \
                        if (_tail) {                           \
                            _tail->next = _e;                  \
                        } else {                               \
                            head = _e;                         \
                        }                                      \
                        _tail = _e;                            \
                    }                                          \
                    _p = _q;                                   \
                }                                              \
                if (_tail)                                     \
                    _tail->next = NULL;                        \
                if (_nmerges <= 1)                             \
                    break;                                     \
                _insize *= 2;                                  \
            }                                                  \
        }                                                      \
    } while (0)

#define SL_REVERSE(head)                                   \
    do {                                                   \
        __typeof__(head) _cur = head, _prev = NULL, _next; \
        while (_cur) {                                     \
            _next = _cur->next;                            \
            _cur->next = _prev;                            \
            _prev = _cur;                                  \
            _cur = _next;                                  \
        }                                                  \
        head = _prev;                                      \
    } while (0)

#define SL_LAST(head, node)    \
    do {                       \
        node = head;           \
        while (node->next) {   \
            node = node->next; \
        }                      \
    } while (0)

#define SL_CONCAT(head1, head2)             \
    do {                                    \
        if (head1) {                        \
            __typeof__(head1) _tmp = head1; \
            while (_tmp->next) {            \
                _tmp = _tmp->next;          \
            }                               \
            _tmp->next = head2;             \
        } else {                            \
            head1 = head2;                  \
        }                                   \
    } while (0)

#define SL_LENGTH(head, length)       \
    do {                              \
        length = 0;                   \
        __typeof__(head) _cur = head; \
        while (_cur) {                \
            length++;                 \
            _cur = _cur->next;        \
        }                             \
    } while (0)

#define SL_FOREACH(head, node) for (node = head; node; node = node->next)

#define SL_FOREACH_SAFE(head, node, tmp)      \
    for (node = head, tmp = node->next; node; \
         node = tmp, tmp = tmp == NULL ? NULL : tmp->next)

#define SL_INDEX(head, node, target)  \
    do {                              \
        int _i = target;              \
        __typeof__(head) _cur = head; \
        while (_cur) {                \
            if (!_i--)                \
                node = _cur;          \
            _cur = _cur->next;        \
        }                             \
    } while (0)

#define SL_SEARCH(head, cmp, query, node) \
    do {                                  \
        __typeof__(head) _cur = head;     \
        while (_cur) {                    \
            if (cmp(_cur, query) == 0)    \
                break;                    \
            _cur = _cur->next;            \
        }                                 \
        node = _cur;                      \
    } while (0)

#define SL_DELETE(head, node)                        \
    do {                                             \
        __typeof__(head) _cur = head;                \
        if (node == head) {                          \
            head = head->next;                       \
        } else {                                     \
            while (_cur->next && _cur->next != node) \
                _cur = _cur->next;                   \
            _cur->next = node->next;                 \
        }                                            \
    } while (0)

/* Doubly-linked Lists */

#define DL_EMPTY SL_EMPTY

#define DL_APPEND(head, entry)            \
    do {                                  \
        if (head) {                       \
            __typeof__(head) _tmp = head; \
            while (_tmp->next != NULL) {  \
                _tmp = _tmp->next;        \
            }                             \
            entry->prev = _tmp;           \
            _tmp->next = entry;           \
        } else {                          \
            head = entry;                 \
        }                                 \
    } while (0)

#define DL_PREPEND(head, entry) \
    do {                        \
        if (head) {             \
            entry->next = head; \
            head->prev = entry; \
        }                       \
        head = entry;           \
    } while (0)

#define DL_SORT SL_SORT

#define DL_REVERSE(head)             \
    do {                             \
        __typeof__(head) _temp;      \
        while (head) {               \
            _temp = head->prev;      \
            head->prev = head->next; \
            head->next = _temp;      \
            if (!head->prev)         \
                break;               \
            head = head->prev;       \
        }                            \
    } while (0)

#define DL_LAST SL_LAST

#define DL_CONCAT SL_CONCAT

#define DL_LENGTH SL_LENGTH

#define DL_FOREACH SL_FOREACH

#define DL_FOREACH_SAFE SL_FOREACH_SAFE

#define DL_INDEX SL_INDEX

#define DL_SEARCH SL_SEARCH

#define DL_DELETE(head, node)                  \
    do {                                       \
        if (head == node) {                    \
            head = head->next;                 \
            if (head) {                        \
                head->prev = NULL;             \
            }                                  \
        } else {                               \
            if (node->prev)                    \
                node->prev->next = node->next; \
            if (node->next)                    \
                node->next->prev = node->prev; \
        }                                      \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif /* __MACRO_H__ */

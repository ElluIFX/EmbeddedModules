#ifndef MACRO_H
#define MACRO_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define __PLOOC_VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                                 _11, _12, _13, _14, _15, _16, __N, ...)      \
    __N

#define __PLOOC_VA_NUM_ARGS(...)                                                 \
    __PLOOC_VA_NUM_ARGS_IMPL(0, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, \
                             7, 6, 5, 4, 3, 2, 1, 0)

// 各种数量的连接宏
#define __CONNECT2(__A, __B)                     __A##__B
#define __CONNECT3(__A, __B, __C)                __A##__B##__C
#define __CONNECT4(__A, __B, __C, __D)           __A##__B##__C##__D
#define __CONNECT5(__A, __B, __C, __D, __E)      __A##__B##__C##__D##__E
#define __CONNECT6(__A, __B, __C, __D, __E, __F) __A##__B##__C##__D##__E##__F
#define __CONNECT7(__A, __B, __C, __D, __E, __F, __G) \
    __A##__B##__C##__D##__E##__F##__G
#define __CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H) \
    _A##__B##__C##__D##__E##__F##__G##__H
#define __CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I) \
    __A##__B##__C##__D##__E##__F##__G##__H##__I
#define CONNECT2(__A, __B)                __CONNECT2(__A, __B)
#define CONNECT3(__A, __B, __C)           __CONNECT3(__A, __B, __C)
#define CONNECT4(__A, __B, __C, __D)      __CONNECT4(__A, __B, __C, __D)
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

#define __using_1(__declare) \
    for (__declare, *SAFE_NAME(using_ptr) = NULL; SAFE_NAME(using_ptr)++ == NULL;)

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
#define __IRQ_SAFE          SAFE_ATOM_CODE

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
                                     SAFE_NAME(cnt) > 0; _++, SAFE_NAME(cnt)--)

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
#define __foreach_reverse_3(__array, __type, __pt)               \
    using(__type * __pt = __array + dimof(__array, __type) -     \
                          1) for (uint_fast32_t SAFE_NAME(cnt) = \
                                      dimof(__array, __type);    \
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

#define __MIN_2(__a, __b)      ((__a) < (__b) ? (__a) : (__b))
#define __MIN_3(__a, __b, __c) __MIN_2(__MIN_2(__a, __b), __c)
#define __MIN_4(__a, __b, __c, __d) \
    __MIN_2(__MIN_2(__a, __b), __MIN_2(__c, __d))

/**
 * @brief Get the minimum value of the specified values
 */
#define CMIN(...)             \
    EVAL(__MIN_, __VA_ARGS__) \
    (__VA_ARGS__)

#define __MAX_2(__a, __b)      ((__a) > (__b) ? (__a) : (__b))
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

/************** 语法糖 END *********************/
#ifdef __cplusplus
}
#endif
#endif // MACRO_H

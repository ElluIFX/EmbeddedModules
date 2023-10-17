/**
 * @file log.h
 * @brief  日志系统
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-05-08
 *
 * THINK DIFFERENTLY
 */

#ifndef __LOG_H__
#define __LOG_H__

#include "modules.h"
#include "uart_pack.h"

// 日志输出设置
#define _LOG_PRINTF printf  // 调试日志输出函数
#define _TIMESTAMP ((double)(m_time_us() / 100) / 10000)  // 时间戳获取方式
#define _TIMESTAMP_FMT "%.4lf"                            // 时间戳格式

#if _LOG_ENABLE
#if _LOG_ENABLE_TIMESTAMP && _LOG_ENABLE_COLOR
#define _DBG_LOG(level, color, fmt, args...)                        \
  _LOG_PRINTF("\033[" #color "m[" level "/" _TIMESTAMP_FMT "] " fmt \
              "\033[0m\r\n",                                        \
              _TIMESTAMP, ##args)
#elif !_LOG_ENABLE_TIMESTAMP && _LOG_ENABLE_COLOR
#define _DBG_LOG(level, color, fmt, args...) \
  _LOG_PRINTF("\033[" #color "m[" level "] " fmt "\033[0m\r\n", ##args)
#elif _LOG_ENABLE_TIMESTAMP && !_LOG_ENABLE_COLOR
#define _DBG_LOG(level, color, fmt, args...) \
  _LOG_PRINTF("[" level "/" _TIMESTAMP_FMT "] " fmt "\r\n", _TIMESTAMP, ##args)
#elif !_LOG_ENABLE_TIMESTAMP && !_LOG_ENABLE_COLOR
#define _DBG_LOG(level, color, fmt, args...) \
  _LOG_PRINTF("[" level "] " fmt "\r\n", ##args)
#endif
#if _LOG_ENABLE_FUNC_LINE
#define _DBG_LOG_FUNC(level, color, fmt, args...) \
  _DBG_LOG(level, color, "%s:%d " fmt, __func__, __LINE__, ##args)
#else
#define _DBG_LOG_FUNC _DBG_LOG
#endif  // _LOG_ENABLE_FUNC_LINE
#if _LOG_ENABLE_DEBUG
#define LOG_D(fmt, args...) _DBG_LOG_FUNC("D", 36, fmt, ##args)
#endif
#if _LOG_ENABLE_INFO
#define LOG_I(fmt, args...) _DBG_LOG_FUNC("I", 32, fmt, ##args)
#endif
#if _LOG_ENABLE_WARN
#define LOG_W(fmt, args...) _DBG_LOG_FUNC("W", 33, fmt, ##args)
#endif
#if _LOG_ENABLE_ERROR
#define LOG_E(fmt, args...) _DBG_LOG_FUNC("E", 31, fmt, ##args)
#endif
#if _LOG_ENABLE_FATAL
#define LOG_F(fmt, args...) _DBG_LOG_FUNC("F", 35, fmt, ##args)
#endif
#define LOG_RAW(fmt, args...) _LOG_PRINTF(fmt, ##args)

#if _LOG_ENABLE_COLOR
#define LOG_REFRESH(fmt, args...) \
  _LOG_PRINTF("\r\033[34m[R] " fmt "   \033[0m", ##args)
#define LOG_LIMIT(limit_ms, fmt, args...)                    \
  {                                                          \
    static m_time_t SAFE_NAME(limited_log_t) = 0;            \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) { \
      SAFE_NAME(limited_log_t) = m_time_ms();                \
      _LOG_PRINTF("\033[34m[L] " fmt "\033[0m\r\n", ##args); \
    }                                                        \
  }
#else
#define LOG_REFRESH(fmt, args...) _LOG_PRINTF("\r[R] " fmt "   ", ##args)
#define LOG_LIMIT(limit_ms, fmt, args...)                    \
  {                                                          \
    static m_time_t SAFE_NAME(limited_log_t) = 0;            \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) { \
      SAFE_NAME(limited_log_t) = m_time_ms();                \
      _LOG_PRINTF("[L] " fmt "\r\n", ##args);                \
    }                                                        \
  }
#endif  // _LOG_ENABLE_COLOR
#define LOG_ENDL() LOG_RAW("\r\n")
#endif  // _LOG_ENABLE

#ifndef LOG_D
#define LOG_D(...) ((void)0)
#endif
#ifndef LOG_I
#define LOG_I(...) ((void)0)
#endif
#ifndef LOG_W
#define LOG_W(...) ((void)0)
#endif
#ifndef LOG_E
#define LOG_E(...) ((void)0)
#endif
#ifndef LOG_F
#define LOG_F(...) ((void)0)
#endif
#ifndef LOG_RAW
#define LOG_RAW(...) ((void)0)
#endif
#ifndef LOG_REFRESH
#define LOG_REFRESH(...) ((void)0)
#endif
#ifndef LOG_LIMIT
#define LOG_LIMIT(...) ((void)0)
#endif
#ifndef LOG_ENDL
#define LOG_ENDL(...) ((void)0)
#endif

#define LOG_DEBUG LOG_D
#define LOG_INFO LOG_I
#define LOG_WARN LOG_W
#define LOG_ERROR LOG_E
#define LOG_FATAL LOG_F

#define _TERM_COLOR(color) "\033[" #color "m"
#define _TERM_RESET _TERM_COLOR(0)
#define _TERM_RED _TERM_COLOR(31)
#define _TERM_GREEN _TERM_COLOR(32)
#define _TERM_YELLOW _TERM_COLOR(33)
#define _TERM_BLUE _TERM_COLOR(34)
#define _TERM_MAGENTA _TERM_COLOR(35)
#define _TERM_CYAN _TERM_COLOR(36)
#define _TERM_WHITE _TERM_COLOR(37)

#if _LOG_ENABLE_ASSERT
#define __ASSERT_PRINT(text) _DBG_LOG("A", 31, text)
#define __ASSERT_0(expr)                       \
  if (!(expr)) {                               \
    __ASSERT_PRINT("Failed expr: " #expr);     \
    Assert_Failed_Handler(__FILE__, __LINE__); \
  }
#define __ASSERT_1(expr, text)                 \
  if (!(expr)) {                               \
    __ASSERT_PRINT(text);                      \
    Assert_Failed_Handler(__FILE__, __LINE__); \
  }
#define __ASSERT_2(expr, text, cmd) \
  if (!(expr)) {                    \
    __ASSERT_PRINT(text);           \
    cmd;                            \
  }

/**
 * @brief 断言失败处理函数, 需用户实现
 * @param  file 文件名
 * @param  line 行号
 */
extern void Assert_Failed_Handler(char *file, uint32_t line);

// 断言, param: 表达式, 错误日志(可选), 自定义语句(可选)
// 断言失败时默认调用Assert_Failed_Handler
#define ASSERT(expr, ...)      \
  EVAL(__ASSERT_, __VA_ARGS__) \
  (expr, ##__VA_ARGS__)
#else
#define ASSERT(expr, ...) ((void)0)
#endif

#if _MOD_USE_PERF_COUNTER
#define timeit(NAME)                                                         \
  __cycleof__("", {                                                          \
    LOG_D("timeit(" NAME ")=%fus", (double)_ / (SystemCoreClock / 1000000)); \
  })

#define cycleit(NAME) __cycleof__("", { LOG_D("cycleit(" NAME ")=%d", _); })

#define timeit_limit(NAME, limit_ms)                  \
  __cycleof__("", {                                   \
    LOG_LIMIT(limit_ms, "timeit(" NAME ")=%fus",      \
              (double)_ * 1000000 / SystemCoreClock); \
  })
#define cycleit_limit(NAME, limit_ms) \
  __cycleof__("", { LOG_LIMIT(limit_ms, "cycleit(" NAME ")=%d", _); })

#define timeit_avg(NAME, N)                                               \
  double SAFE_NAME(timeit_avg) = N;                                       \
  __cycleof__("", {                                                       \
    LOG_D("timeit_avg(" NAME ")=%fus",                                    \
          (double)_ * 1000000 / SystemCoreClock / SAFE_NAME(timeit_avg)); \
  })
#endif

#endif  // __LOG_H

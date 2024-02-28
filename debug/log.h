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

#ifdef __cplusplus
extern "C" {
#endif
#include "macro.h"
#include "modules.h"
#include "uart_pack.h"

/****************************    日志设置     ***********************/
// 日志输出
#define _LOG_PRINTF printf  // 日志输出函数 (必须为类printf函数)
#define _LOG_TIMESTAMP ((float)((uint64_t)m_time_ms()) / 1000)  // 时间戳获取
#define _LOG_TIMESTAMP_FMT "%.3fs"  // 时间戳格式

#if 0
#define _LOG_PREFIX "\r"          // 日志前缀 (移动光标到行首)
#define _LOG_SUFFIX "\033[K\r\n"  // 日志后缀 (清空光标到行尾并换行)
#else
#define _LOG_PREFIX ""      // 日志前缀
#define _LOG_SUFFIX "\r\n"  // 日志后缀 (仅换行)
#endif

// 日志等级颜色
#define _LOG_D_COLOR T_CYAN     // 调试日志
#define _LOG_P_COLOR T_LGREEN   // 通过日志
#define _LOG_I_COLOR T_GREEN    // 信息日志
#define _LOG_W_COLOR T_YELLOW   // 警告日志
#define _LOG_E_COLOR T_RED      // 错误日志
#define _LOG_F_COLOR T_MAGENTA  // 致命错误日志
#define _LOG_L_COLOR T_BLUE     // 限频日志
#define _LOG_R_COLOR T_BLUE     // 单行刷新日志
#define _LOG_A_COLOR T_RED      // 断言日志
#define _LOG_T_COLOR T_YELLOW   // 计时日志
// 日志等级名称
#define _LOG_D_STR "DEBUG"    // 调试日志
#define _LOG_P_STR "PASS"     // 通过日志
#define _LOG_I_STR "INFO"     // 信息日志
#define _LOG_W_STR "WARN"     // 警告日志
#define _LOG_E_STR "ERROR"    // 错误日志
#define _LOG_F_STR "FATAL"    // 致命错误日志
#define _LOG_L_STR "LIMIT"    // 限频日志
#define _LOG_R_STR "REFRESH"  // 单行刷新日志
#define _LOG_A_STR "ASSERT"   // 断言日志
#define _LOG_T_STR "TIMEIT"   // 计时日志
/*********************************************************************/

// 终端颜色代码
#define T_BLACK "30"
#define T_RED "31"
#define T_GREEN "32"
#define T_YELLOW "33"
#define T_BLUE "34"
#define T_MAGENTA "35"
#define T_CYAN "36"
#define T_WHITE "37"
// 扩展颜色代码
#define T_GRAY "90"
#define T_LRED "91"
#define T_LGREEN "92"
#define T_LYELLOW "93"
#define T_LBLUE "94"
#define T_LMAGENTA "95"
#define T_LCYAN "96"
#define T_LWHITE "97"
// 终端背景代码
#define T_BLACK_BK "40"
#define T_RED_BK "41"
#define T_GREEN_BK "42"
#define T_YELLOW_BK "43"
#define T_BLUE_BK "44"
#define T_MAGENTA_BK "45"
#define T_CYAN_BK "46"
#define T_WHITE_BK "47"
// 终端格式代码
#define T_RESET "0"
#define T_BOLD "1"
#define T_LIGHT "2"
#define T_ITALIC "3"
#define T_UNDERLINE "4"
#define T_BLINK "5"
#define T_REVERSE "7"
#define T_HIDE "8"
#define T_CROSS "9"

#if _LOG_ENABLE_COLOR
#define __T_FMT1(fmt) "\033[" fmt "m"
#define __T_FMT2(fmt1, fmt2) "\033[" fmt1 ";" fmt2 "m"
#define __T_FMT3(fmt1, fmt2, fmt3) "\033[" fmt1 ";" fmt2 ";" fmt3 "m"
#define __T_FMT4(fmt1, fmt2, fmt3, fmt4) \
  "\033[" fmt1 ";" fmt2 ";" fmt3 ";" fmt4 "m"
#else
#define __T_FMT1(fmt) ""
#define __T_FMT2(fmt1, fmt2) ""
#define __T_FMT3(fmt1, fmt2, fmt3) ""
#define __T_FMT4(fmt1, fmt2, fmt3, fmt4) ""
#endif

/**
 * @brief 设定终端输出格式, 可以混合使用
 * @note 颜色: T_BLACK/T_RED/T_GREEN/T_YELLOW/T_BLUE/T_MAGENTA/T_CYAN/T_WHITE
 * @note 背景: 在颜色后加 _BK
 * @note 格式: T_RESET/T_BOLD/T_UNDERLINE/T_BLINK/T_REVERSE/T_HIDE
 */
#define T_FMT(...)           \
  EVAL(__T_FMT, __VA_ARGS__) \
  (__VA_ARGS__)
#define T_RST T_FMT(T_RESET)

#ifndef __FUNCTION__
#define __FUNCTION__ __func__
#endif

#if _LOG_ENABLE
#define _DBG_LOG_FINAL(pre, ts, fl, level, color, suf, fmt, args...) \
  _LOG_PRINTF(pre ts fl T_FMT(color) "[" level "]" T_RST ":" fmt suf, ##args)
#else
#define _DBG_LOG_FINAL(pre, ts, fl, level, color, suf, fmt, args...) ((void)0)
#endif

#if _LOG_ENABLE_TIMESTAMP
#define _DBG_LOG_TS(pre, fl, level, color, suf, fmt, args...)                  \
  _DBG_LOG_FINAL(pre, "[" _LOG_TIMESTAMP_FMT "]:", fl, level, color, suf, fmt, \
                 _LOG_TIMESTAMP, ##args)
#elif !_LOG_ENABLE_TIMESTAMP
#define _DBG_LOG_TS(pre, fl, level, color, suf, fmt, args...) \
  _DBG_LOG_FINAL(pre, "", fl, level, color, suf, fmt, ##args)
#endif

#if _LOG_ENABLE_FUNC_LINE
#define _DBG_LOG_FL(pre, level, color, suf, fmt, args...)                      \
  _DBG_LOG_TS(pre, "[%s:%d]:", level, color, suf, fmt, __FUNCTION__, __LINE__, \
              ##args)
#else
#define _DBG_LOG_FL(pre, level, color, suf, fmt, args...) \
  _DBG_LOG_TS(pre, "", level, color, suf, fmt, ##args)
#endif  // _LOG_ENABLE_FUNC_LINE

#define _DBG_LOG_OUTPUT(pre, level, color, suf, fmt, args...) \
  _DBG_LOG_FL(pre, level, color, suf, fmt, ##args)

#if _LOG_ENABLE_DEBUG
/**
 * @brief 调试日志
 */
#define LOG_DEBUG(fmt, args...)                                            \
  _DBG_LOG_OUTPUT(_LOG_PREFIX, _LOG_D_STR, _LOG_D_COLOR, _LOG_SUFFIX, fmt, \
                  ##args)
#endif
#if _LOG_ENABLE_PASS
/**
 * @brief 通过日志
 */
#define LOG_PASS(fmt, args...)                                             \
  _DBG_LOG_OUTPUT(_LOG_PREFIX, _LOG_P_STR, _LOG_P_COLOR, _LOG_SUFFIX, fmt, \
                  ##args)
#endif
#if _LOG_ENABLE_INFO
/**
 * @brief 信息日志
 */
#define LOG_INFO(fmt, args...)                                             \
  _DBG_LOG_OUTPUT(_LOG_PREFIX, _LOG_I_STR, _LOG_I_COLOR, _LOG_SUFFIX, fmt, \
                  ##args)
#endif
#if _LOG_ENABLE_WARN
/**
 * @brief 警告日志
 */
#define LOG_WARN(fmt, args...)                                             \
  _DBG_LOG_OUTPUT(_LOG_PREFIX, _LOG_W_STR, _LOG_W_COLOR, _LOG_SUFFIX, fmt, \
                  ##args)
#endif
#if _LOG_ENABLE_ERROR
/**
 * @brief 错误日志
 */
#define LOG_ERROR(fmt, args...)                                            \
  _DBG_LOG_OUTPUT(_LOG_PREFIX, _LOG_E_STR, _LOG_E_COLOR, _LOG_SUFFIX, fmt, \
                  ##args)
#endif
#if _LOG_ENABLE_FATAL
/**
 * @brief 致命错误日志
 */
#define LOG_FATAL(fmt, args...)                                            \
  _DBG_LOG_OUTPUT(_LOG_PREFIX, _LOG_F_STR, _LOG_F_COLOR, _LOG_SUFFIX, fmt, \
                  ##args)
#endif

#ifndef LOG_DEBUG
// 该日志等级已禁用
#define LOG_DEBUG(...) ((void)0)
#endif
#ifndef LOG_PASS
// 该日志等级已禁用
#define LOG_PASS(...) ((void)0)
#endif
#ifndef LOG_INFO
// 该日志等级已禁用
#define LOG_INFO(...) ((void)0)
#endif
#ifndef LOG_WARN
// 该日志等级已禁用
#define LOG_WARN(...) ((void)0)
#endif
#ifndef LOG_ERROR
// 该日志等级已禁用
#define LOG_ERROR(...) ((void)0)
#endif
#ifndef LOG_FATAL
// 该日志等级已禁用
#define LOG_FATAL(...) ((void)0)
#endif

/**
 * @brief 自定义日志输出
 * @param  level            日志等级(字符串)
 * @param  color            日志颜色(T_<COLOR>)
 */
#define LOG_CUSTOM(level, color, fmt, args...) \
  _DBG_LOG_OUTPUT(_LOG_PREFIX, level, color, _LOG_SUFFIX, fmt, ##args)

/**
 * @brief 原始日志输出
 */
#define LOG_RAW(fmt, args...) _LOG_PRINTF(fmt, ##args)
/**
 * @brief 原始日志输出并换行
 */
#define LOG_RAWLN(fmt, args...) _LOG_PRINTF(fmt _LOG_SUFFIX, ##args)
/**
 * @brief 输出换行
 */
#define LOG_ENDL() LOG_RAW(_LOG_SUFFIX)
/**
 * @brief 在同一行刷新日志
 */
#define LOG_REFRESH(fmt, args...) \
  _DBG_LOG_OUTPUT("\r", "R", _LOG_R_COLOR, "", fmt, ##args)
/**
 * @brief 限频日志
 * @param  limit_ms         输出周期(ms)
 */
#define LOG_LIMIT(limit_ms, fmt, args...)                                      \
  {                                                                            \
    static m_time_t SAFE_NAME(limited_log_t) = 0;                              \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) {                   \
      SAFE_NAME(limited_log_t) = m_time_ms();                                  \
      _DBG_LOG_OUTPUT("", _LOG_L_STR, _LOG_L_COLOR, _LOG_SUFFIX, fmt, ##args); \
    }                                                                          \
  }
#define _LOG_TIMEIT(fmt, args...) \
  _DBG_LOG_OUTPUT("", _LOG_T_STR, _LOG_T_COLOR, _LOG_SUFFIX, fmt, ##args)

#define __ASSERT_PRINT(text, args...) \
  _DBG_LOG_OUTPUT("", _LOG_A_STR, _LOG_A_COLOR, _LOG_SUFFIX, text, ##args)

#if _LOG_ENABLE_FUNC_LINE
#define __ASSERT_COMMON(expr) __ASSERT_PRINT("'" #expr "' failed")
#else
#define __ASSERT_COMMON(expr) \
  __ASSERT_PRINT("'" #expr "' failed at %s:%d", __FILE__, __LINE__)
#endif

#if _LOG_ENABLE_ASSERT
#define __ASSERT_0(expr)   \
  if (!(expr)) {           \
    __ASSERT_COMMON(expr); \
  }
#define __ASSERT_MORE(expr, text, args...) \
  if (!(expr)) {                           \
    __ASSERT_PRINT(text, ##args);          \
  }
#else
#define __ASSERT_0(text) ((void)0)
#define __ASSERT_MORE(expr, text, args...) ((void)0)
#endif

#define __ASSERT_1 __ASSERT_MORE
#define __ASSERT_2 __ASSERT_MORE
#define __ASSERT_3 __ASSERT_MORE
#define __ASSERT_4 __ASSERT_MORE
#define __ASSERT_5 __ASSERT_MORE
#define __ASSERT_6 __ASSERT_MORE
#define __ASSERT_7 __ASSERT_MORE
#define __ASSERT_8 __ASSERT_MORE
#define __ASSERT_9 __ASSERT_MORE

#if _LOG_ENABLE_ASSERT
#define __ASSERT_CMD_0(expr, cmd) \
  if (!(expr)) {                  \
    __ASSERT_COMMON(expr);        \
    cmd;                          \
  }
#define __ASSERT_CMD_MORE(expr, cmd, text, args...) \
  if (!(expr)) {                                    \
    __ASSERT_PRINT(text, ##args);                   \
    cmd;                                            \
  }
#else
#define __ASSERT_CMD_0(expr, cmd) \
  if (!(expr)) {                  \
    cmd;                          \
  }
#define __ASSERT_CMD_MORE(expr, cmd, text, args...) \
  if (!(expr)) {                                    \
    cmd;                                            \
  }
#endif

#define __ASSERT_CMD_1 __ASSERT_CMD_MORE
#define __ASSERT_CMD_2 __ASSERT_CMD_MORE
#define __ASSERT_CMD_3 __ASSERT_CMD_MORE
#define __ASSERT_CMD_4 __ASSERT_CMD_MORE
#define __ASSERT_CMD_5 __ASSERT_CMD_MORE
#define __ASSERT_CMD_6 __ASSERT_CMD_MORE
#define __ASSERT_CMD_7 __ASSERT_CMD_MORE
#define __ASSERT_CMD_8 __ASSERT_CMD_MORE
#define __ASSERT_CMD_9 __ASSERT_CMD_MORE

/**
 * @brief 断言日志
 * @param  expr             断言表达式
 * @param  text             断言失败时输出的文本(可选)
 */
#define LOG_ASSERT(expr, ...)  \
  EVAL(__ASSERT_, __VA_ARGS__) \
  (expr, ##__VA_ARGS__)

/**
 * @brief 断言日志，断言失败时执行命令
 * @param  expr             断言表达式
 * @param  cmd              断言失败时执行的命令
 * @param  text             断言失败时输出的文本(可选)
 */
#define LOG_ASSERT_CMD(expr, cmd, ...) \
  EVAL(__ASSERT_CMD_, __VA_ARGS__)     \
  (expr, cmd, ##__VA_ARGS__)

// 别名

#define LOG_D LOG_DEBUG
#define LOG_P LOG_PASS
#define LOG_I LOG_INFO
#define LOG_W LOG_WARN
#define LOG_E LOG_ERROR
#define LOG_F LOG_FATAL
#define LOG_A LOG_ASSERT
#define LOG_AC LOG_ASSERT_CMD
#define LOG_L LOG_LIMIT

#if _MOD_TIME_MATHOD == 1  // perf_counter
/**
 * @brief 测量代码块执行时间
 * @param  NAME             测量名称
 */
#define timeit(NAME)                                                     \
  __cycleof__("", {                                                      \
    _LOG_TIMEIT(NAME ": %fus", (double)_ / (SystemCoreClock / 1000000)); \
  })

/**
 * @brief 测量代码块执行周期数
 * @param  NAME             测量名称
 */
#define cycleit(NAME) __cycleof__("", { _LOG_TIMEIT(NAME ": %dcycles", _); })

/**
 * @brief 测量代码块执行时间, 并限制输出频率
 * @param  NAME             测量名称
 * @param  limit_ms         输出周期(ms)
 */
#define timeit_limit(NAME, limit_ms)                                     \
  __cycleof__("", {                                                      \
    static m_time_t SAFE_NAME(limited_log_t) = 0;                        \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) {             \
      SAFE_NAME(limited_log_t) = m_time_ms();                            \
      _LOG_TIMEIT(NAME ": %fus", (double)_ * 1000000 / SystemCoreClock); \
    }                                                                    \
  })

/**
 * @brief 测量代码块执行周期数, 并限制输出频率
 * @param  NAME             测量名称
 * @param  limit_ms         输出周期(ms)
 */
#define cycleit_limit(NAME, limit_ms)                        \
  __cycleof__("", {                                          \
    static m_time_t SAFE_NAME(limited_log_t) = 0;            \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) { \
      SAFE_NAME(limited_log_t) = m_time_ms();                \
      _LOG_TIMEIT(NAME ": %dcycles", _);                     \
    }                                                        \
  })

/**
 * @brief 测量代码块在指定周期内的最大单次执行时间
 * @param  NAME             测量名称
 * @param  duration_ms      测量周期(ms)
 */
#define timeit_max(NAME, duration_ms)                                         \
  static m_time_t SAFE_NAME(timeit_max) = 0;                                  \
  static m_time_t SAFE_NAME(timeit_max_last) = 0;                             \
  __cycleof__("", {                                                           \
    if (_ > SAFE_NAME(timeit_max)) SAFE_NAME(timeit_max) = _;                 \
    if (m_time_ms() > SAFE_NAME(timeit_max_last) + duration_ms) {             \
      _LOG_TIMEIT(NAME "(max): %fus",                                         \
                  (double)SAFE_NAME(timeit_max) * 1000000 / SystemCoreClock); \
      SAFE_NAME(timeit_max) = 0;                                              \
      SAFE_NAME(timeit_max_last) = m_time_ms();                               \
    }                                                                         \
  })

/**
 * @brief 对测量时间求取平均值
 * @param  NAME             测量名称
 * @param  N                测量次数
 */
#define timeit_avg(NAME, N)                                             \
  double SAFE_NAME(timeit_avg) = N;                                     \
  __cycleof__("", {                                                     \
    _LOG_TIMEIT(                                                        \
        NAME "(avg in %d): %fus", N,                                    \
        (double)_ * 1000000 / SystemCoreClock / SAFE_NAME(timeit_avg)); \
  })
#endif

#ifdef __cplusplus
}
#endif
#endif  // __LOG_H

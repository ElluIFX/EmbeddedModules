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

#include "macro.h"
#include "modules.h"
#include "uart_pack.h"

/****************************    日志格式设置     ***********************/
/*********************************************************************/

// 终端颜色代码
#define _TF_BLACK "30"
#define _TF_RED "31"
#define _TF_GREEN "32"
#define _TF_YELLOW "33"
#define _TF_BLUE "34"
#define _TF_MAGENTA "35"
#define _TF_CYAN "36"
#define _TF_WHITE "37"
// 终端背景代码
#define _TF_BLACK_BK "40"
#define _TF_RED_BK "41"
#define _TF_GREEN_BK "42"
#define _TF_YELLOW_BK "43"
#define _TF_BLUE_BK "44"
#define _TF_MAGENTA_BK "45"
#define _TF_CYAN_BK "46"
#define _TF_WHITE_BK "47"
// 终端格式代码
#define _TF_RESET "0"
#define _TF_BOLD "1"
#define _TF_LIGHT "2"
#define _TF_ITALIC "3"
#define _TF_UNDERLINE "4"
#define _TF_BLINK "5"
#define _TF_REVERSE "7"
#define _TF_HIDE "8"
#define _TF_CROSS "9"

#define _MERGE(a, b) a##b
#define __MERGE(a, b) _MERGE(a, b)

#if _LOG_ENABLE_COLOR
#define __TERM_FMT1(fmt) "\033[" __MERGE(_TF_, fmt) "m"
#define __TERM_FMT2(fmt1, fmt2) \
  "\033[" __MERGE(_TF_, fmt1) ";" __MERGE(_TF_, fmt2) "m"
#define __TERM_FMT3(fmt1, fmt2, fmt3)                                   \
  "\033[" __MERGE(_TF_, fmt1) ";" __MERGE(_TF_, fmt2) ";" __MERGE(_TF_, \
                                                                  fmt3) "m"
#define __TERM_FMT4(fmt1, fmt2, fmt3, fmt4)                        \
  "\033[" __MERGE(_TF_, fmt1) ";" __MERGE(_TF_, fmt2) ";" __MERGE( \
      _TF_, fmt3) ";" __MERGE(_TF_, fmt4) "m"
#else
#define __TERM_FMT1(fmt) ""
#define __TERM_FMT2(fmt1, fmt2) ""
#define __TERM_FMT3(fmt1, fmt2, fmt3) ""
#define __TERM_FMT4(fmt1, fmt2, fmt3, fmt4) ""
#endif

/**
 * @brief 设定终端输出格式, 可以混合使用
 * @note 颜色: BLACK/RED/GREEN/YELLOW/BLUE/MAGENTA/CYAN/WHITE
 * @note 背景: 在颜色后加 _BK
 * @note 格式: RESET/BOLD/UNDERLINE/BLINK/REVERSE/HIDE
 */
#define TERM_FMT(...) EVAL(__TERM_FMT, __VA_ARGS__)(__VA_ARGS__)
#define TERM_RST TERM_FMT(RESET)

#if _LOG_ENABLE
#if _LOG_ENABLE_TIMESTAMP
#define _DBG_LOG_TS(hd, level, ts, color, end, fmt, args...)            \
  _LOG_PRINTF(hd TERM_FMT(color) "[" level "/" _LOG_TIMESTAMP_FMT "" ts \
                                 "] " fmt TERM_RST end,                 \
              _LOG_TIMESTAMP, ##args)
#elif !_LOG_ENABLE_TIMESTAMP
#define _DBG_LOG_TS(hd, level, ts, color, end, fmt, args...) \
  _LOG_PRINTF(hd TERM_FMT(color) "[" level "" ts "] " fmt TERM_RST end, ##args)
#endif

#if _LOG_ENABLE_FUNC_LINE
#define _DBG_LOG_FL(hd, level, color, end, fmt, args...) \
  _DBG_LOG_TS(hd, level, "/%s:%d", color, end, fmt, __func__, __LINE__, ##args)
#else
#define _DBG_LOG_FL(hd, level, color, end, fmt, args...) \
  _DBG_LOG_TS(hd, level, "", color, end, fmt, ##args)
#endif  // _LOG_ENABLE_FUNC_LINE
#if _LOG_ENABLE_DEBUG

/**
 * @brief 调试日志输出
 */
#define LOG_D(fmt, args...) \
  _DBG_LOG_FL("", "D", _LOG_D_COLOR, _LOG_ENDL, fmt, ##args)
#endif
#if _LOG_ENABLE_INFO
/**
 * @brief 信息日志输出
 */
#define LOG_I(fmt, args...) \
  _DBG_LOG_FL("", "I", _LOG_I_COLOR, _LOG_ENDL, fmt, ##args)
#endif
#if _LOG_ENABLE_WARN
/**
 * @brief 警告日志输出
 */
#define LOG_W(fmt, args...) \
  _DBG_LOG_FL("", "W", _LOG_W_COLOR, _LOG_ENDL, fmt, ##args)
#endif
#if _LOG_ENABLE_ERROR
/**
 * @brief 错误日志输出
 */
#define LOG_E(fmt, args...) \
  _DBG_LOG_FL("", "E", _LOG_E_COLOR, _LOG_ENDL, fmt, ##args)
#endif
#if _LOG_ENABLE_FATAL
/**
 * @brief 致命错误日志输出
 */
#define LOG_F(fmt, args...) \
  _DBG_LOG_FL("", "F", _LOG_F_COLOR, _LOG_ENDL, fmt, ##args)
#endif
/**
 * @brief 原始日志输出
 */
#define LOG_RAW(fmt, args...) _LOG_PRINTF(fmt, ##args)
/**
 * @brief 原始日志输出/换行
 */
#define LOG_RAWLN(fmt, args...) _LOG_PRINTF(fmt _LOG_ENDL, ##args)
/**
 * @brief 输出换行
 */
#define LOG_ENDL() LOG_RAW(_LOG_ENDL)
/**
 * @brief 在同一行刷新日志
 */
#define LOG_REFRESH(fmt, args...) \
  _DBG_LOG_FL("\r", "R", _LOG_R_COLOR, "", fmt, ##args)
/**
 * @brief 限制日志输出频率
 * @param  limit_ms         输出周期(ms)
 */
#define LOG_LIMIT(limit_ms, fmt, args...)                         \
  {                                                               \
    static m_time_t SAFE_NAME(limited_log_t) = 0;                 \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) {      \
      SAFE_NAME(limited_log_t) = m_time_ms();                     \
      _DBG_LOG_FL("", "L", _LOG_L_COLOR, _LOG_ENDL, fmt, ##args); \
    }                                                             \
  }
#define _LOG_TIMEIT(fmt, args...) \
  _DBG_LOG_FL("", "T", _LOG_T_COLOR, _LOG_ENDL, fmt, ##args)
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

#if _LOG_ENABLE_ASSERT
#define __ASSERT_PRINT(text) _DBG_LOG_FL("", "A", _LOG_A_COLOR, _LOG_ENDL, text)
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

/**
 * @brief 断言宏
 * @param  expr             断言表达式
 * @param  (可选)text       断言失败时输出的文本
 * @param  (可选)cmd        断言失败时执行的命令
 */
#define ASSERT(expr, ...) EVAL(__ASSERT_, __VA_ARGS__)(expr, ##__VA_ARGS__)
#else
#define ASSERT(expr, ...) ((void)0)
#endif

#if _MOD_USE_PERF_COUNTER
/**
 * @brief 测量代码块执行时间
 * @param  NAME             测量名称
 */
#define timeit(NAME)                                      \
  __cycleof__("", {                                       \
    _LOG_TIMEIT("timeit(" NAME ")=%fus",                  \
                (double)_ / (SystemCoreClock / 1000000)); \
  })

/**
 * @brief 测量代码块执行周期数
 * @param  NAME             测量名称
 */
#define cycleit(NAME) \
  __cycleof__("", { _LOG_TIMEIT("cycleit(" NAME ")=%d", _); })

/**
 * @brief 测量代码块执行时间, 并限制输出频率
 * @param  NAME             测量名称
 * @param  limit_ms         输出周期(ms)
 */
#define timeit_limit(NAME, limit_ms)                         \
  __cycleof__("", {                                          \
    static m_time_t SAFE_NAME(limited_log_t) = 0;            \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) { \
      SAFE_NAME(limited_log_t) = m_time_ms();                \
      _LOG_TIMEIT("timeit(" NAME ")=%fus",                   \
                  (double)_ * 1000000 / SystemCoreClock);    \
    }                                                        \
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
      _LOG_TIMEIT("cycleit(" NAME ")=%d", _);                \
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
      _LOG_TIMEIT("timeit_max(" NAME ")=%fus",                                \
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
        "timeit_avg(" NAME ", %d)=%fus", SAFE_NAME(timeit_avg),         \
        (double)_ * 1000000 / SystemCoreClock / SAFE_NAME(timeit_avg)); \
  })
#endif

#endif  // __LOG_H

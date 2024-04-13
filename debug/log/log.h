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

#if UIO_CFG_PRINTF_REDIRECT
#include "uni_io.h"
#endif

#include "log_color.h"

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_PASS 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_WARN 4
#define LOG_LEVEL_ERROR 5
#define LOG_LEVEL_FATAL 6
#define LOG_LEVEL_ASSERT 7

/****************************    日志设置     ***********************/
#if !KCONFIG_AVAILABLE  // 由Kconfig配置
// 调试日志设置
#define LOG_CFG_ENABLE 1            // 调试日志总开关
#define LOG_CFG_ENABLE_TIMESTAMP 1  // 调试日志是否添加时间戳
#define LOG_CFG_ENABLE_COLOR 1      // 调试日志是否按等级添加颜色
#define LOG_CFG_ENABLE_FUNC_LINE 0  // 调试日志是否添加函数名和行号
#define LOG_CFG_ENABLE_MODULE_NAME 1  // 调试日志是否添加模块名(如有)
#define LOG_CFG_ENABLE_ALIAS 0  // 调试日志是否支持别名(如LOG_E)
#define LOG_CFG_ENABLE_HOOK 0   // 调试日志是否支持输出钩子
// 调试日志等级
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_TRACE  // 调试日志全局等级
// 日志输出
#define LOG_CFG_PRINTF printf  // 日志输出函数 (必须为类printf函数)
#define LOG_CFG_TIMESTAMP_FUNC \
  ((float)((uint64_t)m_time_ms()) / 1000)  // 时间戳获取
#define LOG_CFG_TIMESTAMP_FMT "%.3fs"      // 时间戳格式
#if 0
#define LOG_CFG_PREFIX "\r"      // 日志前缀 (移动光标到行首)
#define LOG_CFG_SUFFIX "\033[K"  // 日志后缀 (清空光标到行尾)
#else
#define LOG_CFG_PREFIX ""
#define LOG_CFG_SUFFIX ""
#endif

#define LOG_CFG_NEWLINE "\r\n"  // 换行符

// 日志等级颜色
#define LOG_CFG_R_COLOR T_BLUE     // 追踪日志
#define LOG_CFG_D_COLOR T_CYAN     // 调试日志
#define LOG_CFG_P_COLOR T_LGREEN   // 操作成功日志
#define LOG_CFG_I_COLOR T_GREEN    // 信息日志
#define LOG_CFG_W_COLOR T_YELLOW   // 警告日志
#define LOG_CFG_E_COLOR T_RED      // 错误日志
#define LOG_CFG_F_COLOR T_MAGENTA  // 致命错误日志
#define LOG_CFG_A_COLOR T_RED      // 断言日志
#define LOG_CFG_T_COLOR T_YELLOW   // 计时日志
// 日志等级名称
#define LOG_CFG_C_STR "TRACE"   // 追踪日志
#define LOG_CFG_D_STR "DEBUG"   // 调试日志
#define LOG_CFG_P_STR "PASS"    // 操作成功日志
#define LOG_CFG_I_STR "INFO"    // 信息日志
#define LOG_CFG_W_STR "WARN"    // 警告日志
#define LOG_CFG_E_STR "ERROR"   // 错误日志
#define LOG_CFG_F_STR "FATAL"   // 致命错误日志
#define LOG_CFG_A_STR "ASSERT"  // 断言日志
#define LOG_CFG_T_STR "TIMEIT"  // 计时日志

#else

#if LOG_CFG_LEVEL_USE_TRACE
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_TRACE
#elif LOG_CFG_LEVEL_USE_DEBUG
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_DEBUG
#elif LOG_CFG_LEVEL_USE_PASS
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_PASS
#elif LOG_CFG_LEVEL_USE_INFO
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_INFO
#elif LOG_CFG_LEVEL_USE_WARN
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_WARN
#elif LOG_CFG_LEVEL_USE_ERROR
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_ERROR
#elif LOG_CFG_LEVEL_USE_FATAL
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_FATAL
#elif LOG_CFG_LEVEL_USE_ASSERT
#define LOG_CFG_GLOBAL_LEVEL LOG_LEVEL_ASSERT
#endif

#endif  // KCONFIG_AVAILABLE
/*********************************************************************/

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_CFG_GLOBAL_LEVEL
#endif

#define LOG_CFG_ENABLE_TRACE (LOG_LEVEL <= LOG_LEVEL_TRACE)
#define LOG_CFG_ENABLE_DEBUG (LOG_LEVEL <= LOG_LEVEL_DEBUG)
#define LOG_CFG_ENABLE_PASS (LOG_LEVEL <= LOG_LEVEL_PASS)
#define LOG_CFG_ENABLE_INFO (LOG_LEVEL <= LOG_LEVEL_INFO)
#define LOG_CFG_ENABLE_WARN (LOG_LEVEL <= LOG_LEVEL_WARN)
#define LOG_CFG_ENABLE_ERROR (LOG_LEVEL <= LOG_LEVEL_ERROR)
#define LOG_CFG_ENABLE_FATAL (LOG_LEVEL <= LOG_LEVEL_FATAL)
#define LOG_CFG_ENABLE_ASSERT (LOG_LEVEL <= LOG_LEVEL_ASSERT)

/**
 * @brief 原始日志输出(直接调用LOG_CFG_PRINTF)
 */
#define PRINT(fmt, args...) LOG_CFG_PRINTF(fmt, ##args)
/**
 * @brief 原始日志输出并换行(直接调用LOG_CFG_PRINTF)
 */
#define PRINTLN(fmt, args...) LOG_CFG_PRINTF(fmt LOG_CFG_NEWLINE, ##args)

#if LOG_CFG_ENABLE
#if !LOG_CFG_ENABLE_HOOK
#define __LOG_FINAL(pre, ts, fl, level, color, suf, fmt, args...) \
  LOG_CFG_PRINTF(pre T_FMT(color) "[" level "]" T_RST ":" ts fl fmt suf, ##args)
#else  // LOG_CFG_ENABLE_HOOK
/**
 * @brief 日志输出钩子
 * @param  fmt              格式化字符串(不包含换行符)
 */
extern void log_hook(const char *fmt, ...);
#define __LOG_FINAL(pre, ts, fl, level, color, suf, fmt, args...)      \
  do {                                                                 \
    LOG_CFG_PRINTF(pre T_FMT(color) "[" level "]" T_RST                \
                                    ":" ts fl fmt suf LOG_CFG_NEWLINE, \
                   ##args);                                            \
    log_hook(pre "[" level "]:" ts fl fmt suf, ##args);                \
  } while (0)
#endif  // LOG_CFG_ENABLE_HOOK
#endif  // LOG_CFG_ENABLE

#if LOG_CFG_ENABLE_TIMESTAMP
#define __LOG_TS(pre, fl, level, color, suf, fmt, args...)                     \
  __LOG_FINAL(pre, "[" LOG_CFG_TIMESTAMP_FMT "]:", fl, level, color, suf, fmt, \
              LOG_CFG_TIMESTAMP_FUNC, ##args)
#elif !_LOG_ENABLE_TIMESTAMP
#define __LOG_TS(pre, fl, level, color, suf, fmt, args...) \
  __LOG_FINAL(pre, "", fl, level, color, suf, fmt, ##args)
#endif  // LOG_CFG_ENABLE_TIMESTAMP

#if LOG_CFG_ENABLE_FUNC_LINE
#ifndef __FUNCTION__
#define __FUNCTION__ __func__
#endif
#define __LOG_FL(pre, level, color, suf, fmt, args...)                      \
  __LOG_TS(pre, "[%s:%d]:", level, color, suf, fmt, __FUNCTION__, __LINE__, \
           ##args)
#else
#define __LOG_FL(pre, level, color, suf, fmt, args...) \
  __LOG_TS(pre, "", level, color, suf, fmt, ##args)
#endif  // LOG_CFG_ENABLE_FUNC_LINE

#if defined(LOG_MODULE) && LOG_CFG_ENABLE_MODULE_NAME
#define __LOG_MOD(pre, level, color, suf, fmt, args...) \
  __LOG_FL(pre, level, color, suf, "[" LOG_MODULE "]:" fmt, ##args)
#else
#define __LOG_MOD(pre, level, color, suf, fmt, args...) \
  __LOG_FL(pre, level, color, suf, fmt, ##args)
#endif

#define __LOG(pre, level, color, suf, fmt, args...) \
  __LOG_MOD(pre, level, color, suf, fmt, ##args)

#define __LOG_LIMIT(_STR, _CLR, limit_ms, fmt, args...)                 \
  do {                                                                  \
    static m_time_t SAFE_NAME(limited_log_t) = 0;                       \
    static uint32_t SAFE_NAME(limited_log_count) = 0;                   \
    SAFE_NAME(limited_log_count)++;                                     \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) {            \
      SAFE_NAME(limited_log_t) = m_time_ms();                           \
      __LOG(LOG_CFG_PREFIX, _STR, _CLR, LOG_CFG_SUFFIX LOG_CFG_NEWLINE, \
            "[L/%d]:" fmt, SAFE_NAME(limited_log_count), ##args);       \
      SAFE_NAME(limited_log_count) = 0;                                 \
    }                                                                   \
  } while (0)
#define __LOG_REFRESH(_STR, _CLR, fmt, args...) \
  __LOG("\33[s\r\33[1A", _STR, _CLR, "\033[K\33[u", fmt, ##args)

#if LOG_CFG_ENABLE_TRACE
/**
 * @brief 追踪日志
 * @note 变体: LOG_TRACE_LIMIT - 限制日志输出周期(ms)
 * @note 变体: LOG_TRACE_REFRESH - 更新上一次输出的日志
 */
#define LOG_TRACE(fmt, args...)                         \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_R_STR, LOG_CFG_R_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, ##args)
#define LOG_TRACE_LIMIT(limit_ms, fmt, args...) \
  __LOG_LIMIT(LOG_CFG_R_STR, LOG_CFG_R_COLOR, limit_ms, fmt, ##args)
#define LOG_TRACE_REFRESH(fmt, args...) \
  __LOG_REFRESH(LOG_CFG_R_STR, LOG_CFG_R_COLOR, fmt, ##args)
#endif
#if LOG_CFG_ENABLE_DEBUG
/**
 * @brief 调试日志
 * @note 变体: LOG_DEBUG_LIMIT - 限制日志输出周期(ms)
 * @note 变体: LOG_DEBUG_REFRESH - 更新上一次输出的日志
 */
#define LOG_DEBUG(fmt, args...)                         \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_D_STR, LOG_CFG_D_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, ##args)
#define LOG_DEBUG_LIMIT(limit_ms, fmt, args...) \
  __LOG_LIMIT(LOG_CFG_D_STR, LOG_CFG_D_COLOR, limit_ms, fmt, ##args)
#define LOG_DEBUG_REFRESH(fmt, args...) \
  __LOG_REFRESH(LOG_CFG_D_STR, LOG_CFG_D_COLOR, fmt, ##args)
#endif
#if LOG_CFG_ENABLE_PASS
/**
 * @brief 操作成功日志
 * @note 变体: LOG_PASS_LIMIT - 限制日志输出周期(ms)
 * @note 变体: LOG_PASS_REFRESH - 更新上一次输出的日志
 */
#define LOG_PASS(fmt, args...)                          \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_P_STR, LOG_CFG_P_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, ##args)
#define LOG_PASS_LIMIT(limit_ms, fmt, args...) \
  __LOG_LIMIT(LOG_CFG_P_STR, LOG_CFG_P_COLOR, limit_ms, fmt, ##args)
#define LOG_PASS_REFRESH(fmt, args...) \
  __LOG_REFRESH(LOG_CFG_P_STR, LOG_CFG_P_COLOR, fmt, ##args)
#endif
#if LOG_CFG_ENABLE_INFO
/**
 * @brief 信息日志
 * @note 变体: LOG_INFO_LIMIT - 限制日志输出周期(ms)
 * @note 变体: LOG_INFO_REFRESH - 更新上一次输出的日志
 */
#define LOG_INFO(fmt, args...)                          \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_I_STR, LOG_CFG_I_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, ##args)
#define LOG_INFO_LIMIT(limit_ms, fmt, args...) \
  __LOG_LIMIT(LOG_CFG_I_STR, LOG_CFG_I_COLOR, limit_ms, fmt, ##args)
#define LOG_INFO_REFRESH(fmt, args...) \
  __LOG_REFRESH(LOG_CFG_I_STR, LOG_CFG_I_COLOR, fmt, ##args)
#endif
#if LOG_CFG_ENABLE_WARN
/**
 * @brief 警告日志
 * @note 变体: LOG_WARN_LIMIT - 限制日志输出周期(ms)
 * @note 变体: LOG_WARN_REFRESH - 更新上一次输出的日志
 */
#define LOG_WARN(fmt, args...)                          \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_W_STR, LOG_CFG_W_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, ##args)
#define LOG_WARN_LIMIT(limit_ms, fmt, args...) \
  __LOG_LIMIT(LOG_CFG_W_STR, LOG_CFG_W_COLOR, limit_ms, fmt, ##args)
#define LOG_WARN_REFRESH(fmt, args...) \
  __LOG_REFRESH(LOG_CFG_W_STR, LOG_CFG_W_COLOR, fmt, ##args)
#endif
#if LOG_CFG_ENABLE_ERROR
/**
 * @brief 错误日志
 * @note 变体: LOG_ERROR_LIMIT - 限制日志输出周期(ms)
 * @note 变体: LOG_ERROR_REFRESH - 更新上一次输出的日志
 */
#define LOG_ERROR(fmt, args...)                         \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_E_STR, LOG_CFG_E_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, ##args)
#define LOG_ERROR_LIMIT(limit_ms, fmt, args...) \
  __LOG_LIMIT(LOG_CFG_E_STR, LOG_CFG_E_COLOR, limit_ms, fmt, ##args)
#define LOG_ERROR_REFRESH(fmt, args...) \
  __LOG_REFRESH(LOG_CFG_E_STR, LOG_CFG_E_COLOR, fmt, ##args)
#endif
#if LOG_CFG_ENABLE_FATAL
/**
 * @brief 致命错误日志
 * @note 变体: LOG_FATAL_LIMIT - 限制日志输出周期(ms)
 * @note 变体: LOG_FATAL_REFRESH - 更新上一次输出的日志
 */
#define LOG_FATAL(fmt, args...)                         \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_F_STR, LOG_CFG_F_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, ##args)
#define LOG_FATAL_LIMIT(limit_ms, fmt, args...) \
  __LOG_LIMIT(LOG_CFG_F_STR, LOG_CFG_F_COLOR, limit_ms, fmt, ##args)
#define LOG_FATAL_REFRESH(fmt, args...) \
  __LOG_REFRESH(LOG_CFG_F_STR, LOG_CFG_F_COLOR, fmt, ##args)
#endif

#if !LOG_CFG_ENABLE_TRACE
// 该日志等级已禁用
#define LOG_TRACE(...) ((void)0)
#define LOG_TRACE_LIMIT(...) ((void)0)
#define LOG_TRACE_REFRESH(...) ((void)0)
#endif
#if !LOG_CFG_ENABLE_DEBUG
// 该日志等级已禁用
#define LOG_DEBUG(...) ((void)0)
#define LOG_DEBUG_LIMIT(...) ((void)0)
#define LOG_DEBUG_REFRESH(...) ((void)0)
#endif
#if !LOG_CFG_ENABLE_PASS
// 该日志等级已禁用
#define LOG_PASS(...) ((void)0)
#define LOG_PASS_LIMIT(...) ((void)0)
#define LOG_PASS_REFRESH(...) ((void)0)
#endif
#if !LOG_CFG_ENABLE_INFO
// 该日志等级已禁用
#define LOG_INFO(...) ((void)0)
#define LOG_INFO_LIMIT(...) ((void)0)
#define LOG_INFO_REFRESH(...) ((void)0)
#endif
#if !LOG_CFG_ENABLE_WARN
// 该日志等级已禁用
#define LOG_WARN(...) ((void)0)
#define LOG_WARN_LIMIT(...) ((void)0)
#define LOG_WARN_REFRESH(...) ((void)0)
#endif
#if !LOG_CFG_ENABLE_ERROR
// 该日志等级已禁用
#define LOG_ERROR(...) ((void)0)
#define LOG_ERROR_LIMIT(...) ((void)0)
#define LOG_ERROR_REFRESH(...) ((void)0)
#endif
#if !LOG_CFG_ENABLE_FATAL
// 该日志等级已禁用
#define LOG_FATAL(...) ((void)0)
#define LOG_FATAL_LIMIT(...) ((void)0)
#define LOG_FATAL_REFRESH(...) ((void)0)
#endif

/**
 * @brief 自定义日志输出
 * @param  level            日志等级(字符串)
 * @param  color            日志颜色(T_<COLOR>)
 */
#define LOG_CUSTOM(level, color, fmt, args...)                             \
  __LOG(LOG_CFG_PREFIX, level, color, LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, \
        ##args)

#define __ASSERT_PRINT(text, args...)                   \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_A_STR, LOG_CFG_A_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, text, ##args)

#if LOG_CFG_ENABLE_FUNC_LINE
#define __ASSERT_COMMON(expr) __ASSERT_PRINT("'" #expr "' failed")
#else
#define __ASSERT_COMMON(expr) \
  __ASSERT_PRINT("'" #expr "' failed at %s:%d", __FILE__, __LINE__)
#endif

#if LOG_CFG_ENABLE_ASSERT
#if !LOG_CFG_ASSERT_FAILED_BLOCK
#define __ASSERT_0(expr)   \
  if (!(expr)) {           \
    __ASSERT_COMMON(expr); \
  }
#define __ASSERT_MORE(expr, text, args...) \
  if (!(expr)) {                           \
    __ASSERT_PRINT(text, ##args);          \
  }
#else  // LOG_CFG_ASSERT_FAILED_BLOCK
#define __ASSERT_0(expr)   \
  if (!(expr)) {           \
    __ASSERT_COMMON(expr); \
    while (1) {            \
    }                      \
  }
#define __ASSERT_MORE(expr, text, args...) \
  if (!(expr)) {                           \
    __ASSERT_PRINT(text, ##args);          \
    while (1) {                            \
    }                                      \
  }
#endif  // LOG_CFG_ASSERT_FAILED_BLOCK
#else   // LOG_CFG_ENABLE_ASSERT
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

#if LOG_CFG_ENABLE_ASSERT
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
#else  // LOG_CFG_ENABLE_ASSERT
#define __ASSERT_CMD_0(expr, cmd) \
  if (!(expr)) {                  \
    cmd;                          \
  }
#define __ASSERT_CMD_MORE(expr, cmd, text, args...) \
  if (!(expr)) {                                    \
    cmd;                                            \
  }
#endif  // LOG_CFG_ENABLE_ASSERT

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
 * @param  text             断言失败时输出的文本(可选, 支持格式化)
 * @note 如果启用LOG_CFG_ASSERT_FAILED_BLOCK, 断言失败时将进入死循环
 * @note 变体: LOG_ASSERT_CMD - 断言失败时执行命令
 */
#define LOG_ASSERT(expr, ...)  \
  EVAL(__ASSERT_, __VA_ARGS__) \
  (expr, ##__VA_ARGS__)

/**
 * @brief 断言日志，断言失败时执行命令
 * @param  expr             断言表达式
 * @param  cmd              断言失败时执行的命令
 * @param  text             断言失败时输出的文本(可选, 支持格式化)
 */
#define LOG_ASSERT_CMD(expr, cmd, ...) \
  EVAL(__ASSERT_CMD_, __VA_ARGS__)     \
  (expr, cmd, ##__VA_ARGS__)

#if LOG_CFG_ENABLE_ALIAS  // 别名

#define LOG_T LOG_TRACE
#define LOG_D LOG_DEBUG
#define LOG_P LOG_PASS
#define LOG_I LOG_INFO
#define LOG_W LOG_WARN
#define LOG_E LOG_ERROR
#define LOG_F LOG_FATAL

#define LOG_TL LOG_TRACE_LIMIT
#define LOG_DL LOG_DEBUG_LIMIT
#define LOG_PL LOG_PASS_LIMIT
#define LOG_IL LOG_INFO_LIMIT
#define LOG_WL LOG_WARN_LIMIT
#define LOG_EL LOG_ERROR_LIMIT
#define LOG_FL LOG_FATAL_LIMIT

#define LOG_TR LOG_TRACE_REFRESH
#define LOG_DR LOG_DEBUG_REFRESH
#define LOG_PR LOG_PASS_REFRESH
#define LOG_IR LOG_INFO_REFRESH
#define LOG_WR LOG_WARN_REFRESH
#define LOG_ER LOG_ERROR_REFRESH
#define LOG_FR LOG_FATAL_REFRESH

#define LOG_A LOG_ASSERT
#define LOG_AC LOG_ASSERT_CMD

#endif  // LOG_CFG_ENABLE_ALIAS

#ifndef __cycleof__  // __cycleof__功能默认由perf_counter实现
#include "macro.h"
#pragma clang diagnostic ignored "-Wcompound-token-split-by-macro"
#define __cycleof__(__DUMMY, ...)                            \
  using(uint64_t _ = m_tick(), __cycle_count__ = _, _ = _, { \
    _ = m_tick() - _;                                        \
    __cycle_count__ = _;                                     \
    if (1) {                                                 \
      __VA_ARGS__                                            \
    }                                                        \
  })
#endif

#define __LOG_TIMEIT(fmt, args...)                      \
  __LOG(LOG_CFG_PREFIX, LOG_CFG_T_STR, LOG_CFG_T_COLOR, \
        LOG_CFG_SUFFIX LOG_CFG_NEWLINE, fmt, ##args)

/**
 * @brief 测量代码块执行时间
 * @param  NAME             测量名称
 */
#define timeit(NAME)                                              \
  __cycleof__("", {                                               \
    __LOG_TIMEIT(NAME ":%fus",                                    \
                 (double)__cycle_count__ / m_tick_clk * 1000000); \
  })

/**
 * @brief 测量代码块执行周期数
 * @param  NAME             测量名称
 */
#define cycleit(NAME) \
  __cycleof__("", { __LOG_TIMEIT(NAME ":%dcycles", __cycle_count__); })

/**
 * @brief 测量代码块执行时间, 并限制输出频率
 * @param  NAME             测量名称
 * @param  limit_ms         输出周期(ms)
 */
#define timeit_limit(NAME, limit_ms)                                \
  __cycleof__("", {                                                 \
    static m_time_t SAFE_NAME(limited_log_t) = 0;                   \
    if (m_time_ms() > SAFE_NAME(limited_log_t) + limit_ms) {        \
      SAFE_NAME(limited_log_t) = m_time_ms();                       \
      __LOG_TIMEIT(NAME ":%fus",                                    \
                   (double)__cycle_count__ * 1000000 / m_tick_clk); \
    }                                                               \
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
      __LOG_TIMEIT(NAME ":%dcycles", __cycle_count__);       \
    }                                                        \
  })

/**
 * @brief 测量代码块在指定周期内的最大单次执行时间
 * @param  NAME             测量名称
 * @param  duration_ms      测量周期(ms)
 */
#define timeit_max(NAME, duration_ms)                                     \
  __cycleof__("", {                                                       \
    static m_time_t SAFE_NAME(timeit_max) = 0;                            \
    static m_time_t SAFE_NAME(timeit_last) = 0;                           \
    if (__cycle_count__ > SAFE_NAME(timeit_max))                          \
      SAFE_NAME(timeit_max) = __cycle_count__;                            \
    if (m_time_ms() > SAFE_NAME(timeit_last) + duration_ms) {             \
      __LOG_TIMEIT(NAME "(max):%fus",                                     \
                   (double)SAFE_NAME(timeit_max) * 1000000 / m_tick_clk); \
      SAFE_NAME(timeit_max) = 0;                                          \
      SAFE_NAME(timeit_last) = m_time_ms();                               \
    }                                                                     \
  })

/**
 * @brief 测量代码块在指定周期内的平均执行时间
 * @param  NAME             测量名称
 * @param  duration_ms      测量周期(ms)
 */
#define timeit_avg(NAME, duration_ms)                             \
  __cycleof__("", {                                               \
    static m_time_t SAFE_NAME(timeit_sum) = 0;                    \
    static m_time_t SAFE_NAME(timeit_count) = 0;                  \
    static m_time_t SAFE_NAME(timeit_last) = 0;                   \
    SAFE_NAME(timeit_sum) += __cycle_count__;                     \
    SAFE_NAME(timeit_count)++;                                    \
    if (m_time_ms() > SAFE_NAME(timeit_last) + duration_ms) {     \
      __LOG_TIMEIT(NAME "(avg/%d):%fus", SAFE_NAME(timeit_count), \
                   (double)SAFE_NAME(timeit_sum) * 1000000 /      \
                       (m_tick_clk * SAFE_NAME(timeit_count)));   \
      SAFE_NAME(timeit_sum) = 0;                                  \
      SAFE_NAME(timeit_count) = 0;                                \
      SAFE_NAME(timeit_last) = m_time_ms();                       \
    }                                                             \
  })

/**
 * @brief 对单次测量时间求取平均值
 * @param  NAME             测量名称
 * @param  N                平均次数
 */
#define timeit_calc_avg(NAME, N)                                  \
  __cycleof__("", {                                               \
    double SAFE_NAME(timeit_avg) = N;                             \
    __LOG_TIMEIT(NAME "(avg/%d): %fus", N,                        \
                 (double)__cycle_count__ * 1000000 / m_tick_clk / \
                     SAFE_NAME(timeit_avg));                      \
  })

#ifdef __cplusplus
}
#endif

#endif  // __LOG_H

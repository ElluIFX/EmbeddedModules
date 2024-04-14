/**
 * @file log_color.h
 * @brief 终端输出格式宏定义
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-03-16
 *
 * THINK DIFFERENTLY
 */
#ifndef __LOG_COLOR_H__
#define __LOG_COLOR_H__

#include "macro.h"

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

#if LOG_CFG_ENABLE_COLOR
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
#endif  // LOG_CFG_ENABLE_COLOR

/**
 * @brief 设定终端输出格式, 可以混合使用
 * @note 颜色: T_BLACK/T_RED/T_GREEN/T_YELLOW/T_BLUE/T_MAGENTA/T_CYAN/T_WHITE
 * @note 背景: 在颜色后加 _BK
 * @note 格式: T_RESET/T_BOLD/T_UNDERLINE/T_BLINK/T_REVERSE/T_HIDE
 */
#define T_FMT(...)             \
    EVAL(__T_FMT, __VA_ARGS__) \
    (__VA_ARGS__)
#define T_RST T_FMT(T_RESET)

#endif  // __LOG_COLOR_H__

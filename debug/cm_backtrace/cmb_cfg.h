/*
 * This file is part of the CmBacktrace Library.
 *
 * Copyright (c) 2016, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the configure head file for this library.
 * Created on: 2016-12-15
 */

#ifndef __CMB_CFG_H__
#define __CMB_CFG_H__

#include "uni_io.h"

/* print line, must config by user */
/* e.g., printf(__VA_ARGS__);printf("\r\n")  or  SEGGER_RTT_printf(0,
 * __VA_ARGS__);SEGGER_RTT_WriteString(0, "\r\n")  */
#define cmb_println(...)       \
    printf_block(__VA_ARGS__); \
    printf_block("\r\n")

#if CMB_PRINT_USE_CHINESE
#define CMB_PRINT_LANGUAGE CMB_PRINT_LANGUAGE_CHINESE
#elif CMB_PRINT_USE_CHINESE_UTF8
#define CMB_PRINT_LANGUAGE CMB_PRINT_LANGUAGE_CHINESE_UTF8
#elif CMB_PRINT_USE_ENGLISH
#define CMB_PRINT_LANGUAGE CMB_PRINT_LANGUAGE_ENGLISH
#endif

#if CMB_OS_PLATFORM_USE_FREERTOS
#define CMB_OS_PLATFORM_TYPE CMB_OS_PLATFORM_FREERTOS
#elif CMB_OS_PLATFORM_USE_UCOSII
#define CMB_OS_PLATFORM_TYPE CMB_OS_PLATFORM_UCOSII
#elif CMB_OS_PLATFORM_USE_UCOSIII
#define CMB_OS_PLATFORM_TYPE CMB_OS_PLATFORM_UCOSIII
#elif CMB_OS_PLATFORM_USE_RTX5
#define CMB_OS_PLATFORM_TYPE CMB_OS_PLATFORM_RTX5
#elif CMB_OS_PLATFORM_USE_KLITE
#define CMB_OS_PLATFORM_TYPE CMB_OS_PLATFORM_KLITE
#elif CMB_OS_PLATFORM_USE_RTT
#define CMB_OS_PLATFORM_TYPE CMB_OS_PLATFORM_RTT
#endif

#if CMB_CPU_USE_ARM_CORTEX_M0
#define CMB_CPU_PLATFORM_TYPE CMB_CPU_ARM_CORTEX_M0
#elif CMB_CPU_USE_ARM_CORTEX_M3
#define CMB_CPU_PLATFORM_TYPE CMB_CPU_ARM_CORTEX_M3
#elif CMB_CPU_USE_ARM_CORTEX_M4
#define CMB_CPU_PLATFORM_TYPE CMB_CPU_ARM_CORTEX_M4
#elif CMB_CPU_USE_ARM_CORTEX_M7
#define CMB_CPU_PLATFORM_TYPE CMB_CPU_ARM_CORTEX_M7
#elif CMB_CPU_USE_ARM_CORTEX_M33
#define CMB_CPU_PLATFORM_TYPE CMB_CPU_ARM_CORTEX_M33
#endif

#endif /* __CMB_CFG_H__ */

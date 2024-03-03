/**
 * @file modules.h
 * @brief 模块公用头文件
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-04-29
 *
 * THINK DIFFERENTLY
 */

#ifndef __MODULES_H
#define __MODULES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#if __has_include("modules_config.h")
#include "modules_config.h"
#define KCONFIG_AVAILABLE 1
#else
#warning \
    "modules_config.h not found, run menuconfig (by Kconfig) to generate it"
#define KCONFIG_AVAILABLE 0
#endif

#if !KCONFIG_AVAILABLE  // 由Kconfig配置
// 动态内存分配方法(m_alloc/m_free/m_realloc):
#define MOD_CFG_HEAP_MATHOD_STDLIB 1
#define MOD_CFG_HEAP_MATHOD_LWMEM 0
#define MOD_CFG_HEAP_MATHOD_KLITE 0
#define MOD_CFG_HEAP_MATHOD_FREERTOS 0
#define MOD_CFG_HEAP_MATHOD_HEAP4 0
#define MOD_CFG_HEAP_MATHOD_RTT 0

// 时间获取方法(m_tick/m_time_*)
#define MOD_CFG_TIME_MATHOD_HAL 0
#define MOD_CFG_TIME_MATHOD_PERF_COUNTER 1

// 延时方法(m_delay_*)
#define MOD_CFG_DELAY_MATHOD_HAL 0
#define MOD_CFG_DELAY_MATHOD_PERF_COUNTER 1
#define MOD_CFG_DELAY_MATHOD_KLITE 0
#define MOD_CFG_DELAY_MATHOD_FREERTOS 0
#define MOD_CFG_DELAY_MATHOD_RTT 0

// 是否使用操作系统(MOD_MUTEX_*)
#define MOD_CFG_USE_OS_NONE 1
#define MOD_CFG_USE_OS_KLITE 0
#define MOD_CFG_USE_OS_FREERTOS 0
#define MOD_CFG_USE_OS_RTT 0

#endif  // KCONFIG_AVAILABLE

#if MOD_CFG_TIME_MATHOD_HAL
typedef uint32_t m_time_t;
#define m_time_t_max_value (UINT32_MAX)
#define Init_Module_Timebase() ((void)0)
#define m_time_ms() HAL_GetTick()
#define m_time_us() (HAL_GetTick() * 1000)
#define m_time_ns() (HAL_GetTick() * 1000000)
#define m_time_s() (HAL_GetTick() / 1000)
#define m_tick() HAL_GetTick()
#define m_tick_clk (1000)
#define m_tick_per_ms(type) ((type)1)
#define m_tick_per_us(type) ((type)0.001)
#elif MOD_CFG_TIME_MATHOD_PERF_COUNTER
#include "perf_counter.h"
typedef int64_t m_time_t;
#define m_time_t_max_value (INT64_MAX)
#define Init_Module_Timebase() init_cycle_counter(1);
#define m_time_ms() get_system_ms()
#define m_time_us() get_system_us()
#define m_time_ns() (get_system_us() * 1000)
#define m_time_s() (get_system_ms() / 1000)
#define m_tick() get_system_ticks()
#define m_tick_clk (SystemCoreClock)
#define m_tick_per_ms(type) ((type)SystemCoreClock / 1000)
#define m_tick_per_us(type) ((type)SystemCoreClock / 1000000)
#else
#error "MOD_TIME_MATHOD invalid"
#endif  // MOD_CFG_TIME_MATHOD

#if MOD_CFG_DELAY_MATHOD_HAL  // HAL
#define m_delay_us(x) HAL_Delay((x) / 1000)
#define m_delay_ms(x) HAL_Delay((x))
#define m_delay_s(x) HAL_Delay((x) * 1000)
#elif MOD_CFG_DELAY_MATHOD_PERF_COUNTER  // perf_counter
#define m_delay_us(x) delay_us((x))
#define m_delay_ms(x) delay_ms((x))
#define m_delay_s(x) delay_ms((x) * 1000)
#elif MOD_CFG_DELAY_MATHOD_KLITE  // klite
#include "kernel.h"
#define m_delay_us(x) thread_sleep((uint64_t)(x) / (1000000 / KERNEL_FREQ))
#if KERNEL_FREQ >= 1000
#define m_delay_ms(x) thread_sleep((uint64_t)(x) * (KERNEL_FREQ / 1000))
#else
#define m_delay_ms(x) thread_sleep((uint64_t)(x) / (1000 / KERNEL_FREQ))
#endif
#define m_delay_s(x) thread_sleep((uint64_t)(x) * KERNEL_FREQ)
#elif MOD_CFG_DELAY_MATHOD_FREERTOS  // freertos
#include "FreeRTOS.h"                // period = 1ms
#include "task.h"
#define m_delay_us(x) vTaskDelay((x) / 1000)
#define m_delay_ms(x) vTaskDelay((x))
#define m_delay_s(x) vTaskDelay((x) * 1000)
#elif MOD_CFG_DELAY_MATHOD_RTT  // rtthread
#include "rtthread.h"           // period = 1ms
#define m_delay_us(x) \
  rt_thread_delay((rt_tick_t)(x) / (1000000 / RT_TICK_PER_SECOND))
#if RT_TICK_PER_SECOND >= 1000
#define m_delay_ms(x) \
  rt_thread_delay((rt_tick_t)(x) * (RT_TICK_PER_SECOND / 1000))
#else
#define m_delay_ms(x) \
  rt_thread_delay((rt_tick_t)(x) / (1000 / RT_TICK_PER_SECOND))
#endif
#define m_delay_s(x) rt_thread_delay((rt_tick_t)(x) * RT_TICK_PER_SECOND)
#else
#error "MOD_DELAY_MATHOD invalid"
#endif  // MOD_CFG_DELAY_MATHOD

#if MOD_CFG_HEAP_MATHOD_STDLIB  // stdlib
#include "stdlib.h"
#define m_alloc(size) malloc(size)
#define m_free(ptr) free(ptr)
#define m_realloc(ptr, size) realloc(ptr, size)
#elif MOD_CFG_HEAP_MATHOD_LWMEM  // lwmem
#define _MOD_USE_DALLOC 1
#include "lwmem.h"
#define m_alloc(size) lwmem_malloc(size)
#define m_free(ptr) lwmem_free(ptr)
#define m_realloc(ptr, size) lwmem_realloc(ptr, size)
#elif MOD_CFG_HEAP_MATHOD_KLITE  // klite
#include "kernel.h"
#define m_alloc(size) heap_alloc(size)
#define m_free(ptr) heap_free((ptr))
#define m_realloc(ptr, size) heap_realloc((ptr), size)
#elif MOD_CFG_HEAP_MATHOD_FREERTOS  // freertos
#include "FreeRTOS.h"
#define m_alloc(size) vPortMalloc(size)
#define m_free(ptr) vPortFree(ptr)
#define m_realloc(ptr, size) pvPortRealloc((ptr), size)
#elif MOD_CFG_HEAP_MATHOD_HEAP4  // heap_4
#include "heap_4.h"
#define m_alloc(size) pvPortMalloc(size)
#define m_free(ptr) vPortFree(ptr)
#define m_realloc(ptr, size) pvPortRealloc((ptr), size)
#elif MOD_CFG_HEAP_MATHOD_RTT  // rtthread
#include "rtthread.h"
#define m_alloc(size) rt_malloc(size)
#define m_free(ptr) rt_free(ptr)
#define m_realloc(ptr, size) rt_realloc(ptr, size)
#else
#error "MOD_HEAP_MATHOD invalid"
#endif

#if MOD_CFG_USE_OS_NONE  // none
#define MOD_MUTEX_HANDLE void*
#define MOD_MUTEX_CREATE() (NULL)
#define MOD_MUTEX_ACQUIRE(mutex) ((void)0)
#define MOD_MUTEX_RELEASE(mutex) ((void)0)
#define MOD_MUTEX_FREE(mutex) ((void)0)
#elif MOD_CFG_USE_OS_KLITE  // klite
#include "kernel.h"
#define MOD_MUTEX_HANDLE mutex_t
#define MOD_MUTEX_CREATE() mutex_create()
#define MOD_MUTEX_ACQUIRE(mutex) mutex_lock(mutex)
#define MOD_MUTEX_RELEASE(mutex) mutex_unlock(mutex)
#define MOD_MUTEX_FREE(mutex) mutex_delete(mutex)
#elif MOD_CFG_USE_OS_FREERTOS  // freertos
#include "FreeRTOS.h"
#define MOD_MUTEX_HANDLE SemaphoreHandle_t
#define MOD_MUTEX_CREATE() xSemaphoreCreateMutex()
#define MOD_MUTEX_ACQUIRE(mutex) xSemaphoreTake(mutex, portMAX_DELAY)
#define MOD_MUTEX_RELEASE(mutex) xSemaphoreGive(mutex)
#define MOD_MUTEX_FREE(mutex) vSemaphoreDelete(mutex)
#elif MOD_CFG_USE_OS_RTT  // rtthread
#include "rtthread.h"
#define MOD_MUTEX_HANDLE rt_mutex_t
#define MOD_MUTEX_CREATE() rt_mutex_create("mod_mutex", RT_IPC_FLAG_PRIO)
#define MOD_MUTEX_ACQUIRE(mutex) rt_mutex_take(mutex, RT_WAITING_FOREVER)
#define MOD_MUTEX_RELEASE(mutex) rt_mutex_release(mutex)
#define MOD_MUTEX_FREE(mutex) rt_mutex_delete(mutex)
#else
#error "MOD_USE_OS invalid"
#endif

#ifndef __has_include
#define __has_include(x) 1  // Compatibility with non-clang compilers.
#endif

#ifdef __cplusplus
}
#endif
#endif  // __MODULES_H

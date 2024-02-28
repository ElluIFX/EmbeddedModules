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

#if __has_include("modules_conf.h")
#include "modules_conf.h"
#else
#error \
    "modules_conf.h not found, copy modules_conf.template.h to modules_conf.h and modify it"
#endif

#if _MOD_TIME_MATHOD == 0  // HAL
typedef uint32_t m_time_t;
#define m_time_t_max_value (UINT32_MAX)
#define Init_Module_Timebase() ((void)0)
#define m_time_ms() HAL_GetTick()
#define m_time_us() (HAL_GetTick() * 1000)
#define m_time_ns() (HAL_GetTick() * 1000000)
#define m_time_s() (HAL_GetTick() / 1000)
#define m_tick() HAL_GetTick()
#define m_tick_clk(type) ((type)1000)
#define m_tick_per_ms(type) ((type)1)
#define m_tick_per_us(type) ((type)0.001)
#elif _MOD_TIME_MATHOD == 1  // perf_counter
#include "perf_counter.h"
typedef int64_t m_time_t;
#define m_time_t_max_value (INT64_MAX)
#define Init_Module_Timebase() init_cycle_counter(1);
#define m_time_ms() get_system_ms()
#define m_time_us() get_system_us()
#define m_time_ns() (get_system_us() * 1000)
#define m_time_s() (get_system_ms() / 1000)
#define m_tick() get_system_ticks()
#define m_tick_clk(type) ((type)SystemCoreClock)
#define m_tick_per_ms(type) ((type)SystemCoreClock / 1000)
#define m_tick_per_us(type) ((type)SystemCoreClock / 1000000)
#else
#error "MOD_TIME_MATHOD invalid"
#endif  // _MOD_TIME_MATHOD

#if _MOD_DELAY_MATHOD == 0  // HAL
#define m_delay_us(x) HAL_Delay((x) / 1000)
#define m_delay_ms(x) HAL_Delay((x))
#define m_delay_s(x) HAL_Delay((x) * 1000)
#elif _MOD_DELAY_MATHOD == 1  // perf_counter
#define m_delay_us(x) delay_us((x))
#define m_delay_ms(x) delay_ms((x))
#define m_delay_s(x) delay_ms((x) * 1000)
#elif _MOD_DELAY_MATHOD == 2  // klite
#include "kernel.h"
#define m_delay_us(x) thread_sleep((uint64_t)(x) / (1000000 / KERNEL_FREQ))
#if KERNEL_FREQ >= 1000
#define m_delay_ms(x) thread_sleep((uint64_t)(x) * (KERNEL_FREQ / 1000))
#else
#define m_delay_ms(x) thread_sleep((uint64_t)(x) / (1000 / KERNEL_FREQ))
#endif
#define m_delay_s(x) thread_sleep((uint64_t)(x) * KERNEL_FREQ)
#elif _MOD_DELAY_MATHOD == 3  // freertos
#include "FreeRTOS.h"         // period = 1ms
#include "task.h"
#define m_delay_us(x) vTaskDelay((x) / 1000)
#define m_delay_ms(x) vTaskDelay((x))
#define m_delay_s(x) vTaskDelay((x) * 1000)
#elif _MOD_DELAY_MATHOD == 4  // rtthread
#include "rtthread.h"         // period = 1ms
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
#endif  // _MOD_DELAY_MATHOD

#if _MOD_HEAP_MATHOD == 0  // stdlib
#include "stdlib.h"
#define m_alloc(size) malloc(size)
#define m_free(ptr) free(ptr)
#define m_realloc(ptr, size) realloc(ptr, size)
#elif _MOD_HEAP_MATHOD == 1  // lwmem
#define _MOD_USE_DALLOC 1
#include "lwmem.h"
#define m_alloc(size) lwmem_malloc(size)
#define m_free(ptr) lwmem_free(ptr)
#define m_realloc(ptr, size) lwmem_realloc(ptr, size)
#elif _MOD_HEAP_MATHOD == 2  // klite
#include "kernel.h"
#define m_alloc(size) heap_alloc(size)
#define m_free(ptr) heap_free((ptr))
#define m_realloc(ptr, size) heap_realloc((ptr), size)
#elif _MOD_HEAP_MATHOD == 3  // freertos
#include "FreeRTOS.h"
#define m_alloc(size) vPortMalloc(size)
#define m_free(ptr) vPortFree(ptr)
#define m_realloc(ptr, size) pvPortRealloc((ptr), size)
#elif _MOD_HEAP_MATHOD == 4  // heap_4
#include "heap_4.h"
#define m_alloc(size) pvPortMalloc(size)
#define m_free(ptr) vPortFree(ptr)
#define m_realloc(ptr, size) pvPortRealloc((ptr), size)
#elif _MOD_HEAP_MATHOD == 5  // rtthread
#include "rtthread.h"
#define m_alloc(size) rt_malloc(size)
#define m_free(ptr) rt_free(ptr)
#define m_realloc(ptr, size) rt_realloc(ptr, size)
#else
#error "MOD_HEAP_MATHOD invalid"
#endif

#if _MOD_USE_OS == 0  // none
#define MOD_MUTEX_HANDLE void*
#define MOD_MUTEX_CREATE() (NULL)
#define MOD_MUTEX_ACQUIRE(mutex) ((void)0)
#define MOD_MUTEX_RELEASE(mutex) ((void)0)
#define MOD_MUTEX_FREE(mutex) ((void)0)
#elif _MOD_USE_OS == 1  // klite
#include "kernel.h"
#define MOD_MUTEX_HANDLE mutex_t
#define MOD_MUTEX_CREATE() mutex_create()
#define MOD_MUTEX_ACQUIRE(mutex) mutex_lock(mutex)
#define MOD_MUTEX_RELEASE(mutex) mutex_unlock(mutex)
#define MOD_MUTEX_FREE(mutex) mutex_delete(mutex)
#elif _MOD_USE_OS == 2  // freertos
#include "FreeRTOS.h"
#define MOD_MUTEX_HANDLE SemaphoreHandle_t
#define MOD_MUTEX_CREATE() xSemaphoreCreateMutex()
#define MOD_MUTEX_ACQUIRE(mutex) xSemaphoreTake(mutex, portMAX_DELAY)
#define MOD_MUTEX_RELEASE(mutex) xSemaphoreGive(mutex)
#define MOD_MUTEX_FREE(mutex) vSemaphoreDelete(mutex)
#elif _MOD_USE_OS == 3  // rtthread
#include "rtthread.h"
#define MOD_MUTEX_HANDLE rt_mutex_t
#define MOD_MUTEX_CREATE() rt_mutex_create("mod_mutex", RT_IPC_FLAG_PRIO)
#define MOD_MUTEX_ACQUIRE(mutex) rt_mutex_take(mutex, RT_WAITING_FOREVER)
#define MOD_MUTEX_RELEASE(mutex) rt_mutex_release(mutex)
#define MOD_MUTEX_FREE(mutex) rt_mutex_delete(mutex)
#else
#error "MOD_USE_OS invalid"
#endif

// 触发调试断点
#define MOD_TRIG_DEBUG_HALT() \
  if (CoreDebug->DHCSR & 1) { \
    __breakpoint(0);          \
  }

#ifndef __has_include
#define __has_include(x) 1  // Compatibility with non-clang compilers.
#endif

#ifdef __cplusplus
}
#endif
#endif  // __MODULES_H

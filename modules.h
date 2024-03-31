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
#define MOD_CFG_TIME_MATHOD_KLITE 0

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

// 是否在系统空闲时进入WFI
#define MOD_CFG_WFI_WHEN_SYSTEM_IDLE 0

#endif  // KCONFIG_AVAILABLE

#if MOD_CFG_TIME_MATHOD_HAL
typedef uint32_t m_time_t;
#define m_time_t_max (UINT32_MAX)
#define init_module_timebase() ((void)0)
#define m_time_ms() HAL_GetTick()
#define m_time_us() (HAL_GetTick() * 1000)
#define m_time_ns() (HAL_GetTick() * 1000000)
#define m_time_s() (HAL_GetTick() / 1000)
#define m_tick() HAL_GetTick()
#define m_tick_clk (1000)
#elif MOD_CFG_TIME_MATHOD_PERF_COUNTER
#include "perf_counter.h"
typedef int64_t m_time_t;
#define m_time_t_max (INT64_MAX)
#define init_module_timebase() init_cycle_counter(1);
#define m_time_ms() ((uint64_t)get_system_ms())
#define m_time_us() ((uint64_t)get_system_us())
#define m_time_ns() (((uint64_t)get_system_us()) * 1000)
#define m_time_s() (((uint64_t)get_system_ms()) / 1000)
#define m_tick() ((uint64_t)get_system_ticks())
#define m_tick_clk ((uint64_t)SystemCoreClock)
#elif MOD_CFG_TIME_MATHOD_KLITE
#include "klite.h"
typedef uint64_t m_time_t;
#define m_time_t_max (UINT64_MAX)
#define init_module_timebase() ((void)0)
#define m_time_ms() (kernel_ticks_to_ms(kernel_tick_count64()))
#define m_time_us() (kernel_ticks_to_us(kernel_tick_count64()))
#define m_time_ns() (m_time_us() * 1000)
#define m_time_s() (m_time_ms() / 1000)
#define m_tick() (kernel_tick_count64())
#define m_tick_clk (KLITE_CFG_FREQ)
#else
#error "MOD_CFG_TIME_MATHOD invalid"
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
#include "klite.h"
#define m_delay_us(x) thread_sleep((x) / (1000000U / KLITE_CFG_FREQ))
#if KLITE_CFG_FREQ >= 1000
#define m_delay_ms(x) thread_sleep((x) * (KLITE_CFG_FREQ / 1000U))
#else
#define m_delay_ms(x) thread_sleep((x) / (1000U / KLITE_CFG_FREQ))
#endif
#define m_delay_s(x) thread_sleep((x) * KLITE_CFG_FREQ)
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
#error "MOD_CFG_DELAY_MATHOD invalid"
#endif  // MOD_CFG_DELAY_MATHOD

#if MOD_CFG_HEAP_MATHOD_STDLIB  // stdlib
#include "stdlib.h"
// You should configure CubeMX instead of using this!
#define init_module_heap(ptr, size) (YOU_SHOULD_CONFIG_CUBEMX_INSTEAD)
#define m_alloc(size) malloc(size)
#define m_free(ptr) free(ptr)
#define m_realloc(ptr, size) realloc(ptr, size)
#elif MOD_CFG_HEAP_MATHOD_LWMEM  // lwmem
#define _MOD_USE_DALLOC 1
#include "lwmem.h"
#define init_module_heap(ptr, size)                              \
  do {                                                           \
    static lwmem_region_t _regions[] = {{ptr, size}, {NULL, 0}}; \
    lwmem_assignmem(_regions)                                    \
  } while (0)
#define m_alloc(size) lwmem_malloc(size)
#define m_free(ptr) lwmem_free(ptr)
#define m_realloc(ptr, size) lwmem_realloc(ptr, size)
#elif MOD_CFG_HEAP_MATHOD_KLITE  // klite
#include "klite.h"
// You should init klite instead of using this!
#define init_module_heap(ptr, size) (YOU_SHOULD_CONFIG_KLITE_INSTEAD)
#define m_alloc(size) heap_alloc(size)
#define m_free(ptr) heap_free((ptr))
#define m_realloc(ptr, size) heap_realloc((ptr), size)
#elif MOD_CFG_HEAP_MATHOD_FREERTOS  // freertos
#include "FreeRTOS.h"
// You should init freertos instead of using this!
#define init_module_heap(ptr, size) (YOU_SHOULD_CONFIG_FREERTOS_INSTEAD)
#define m_alloc(size) vPortMalloc(size)
#define m_free(ptr) vPortFree(ptr)
#define m_realloc(ptr, size) pvPortRealloc((ptr), size)
#elif MOD_CFG_HEAP_MATHOD_HEAP4  // heap4
#include "heap4.h"
#define init_module_heap(ptr, size) prvHeapInit(ptr, size)
#define m_alloc(size) pvPortMalloc(size)
#define m_free(ptr) vPortFree(ptr)
#define m_realloc(ptr, size) pvPortRealloc((ptr), size)
#elif MOD_CFG_HEAP_MATHOD_RTT  // rtthread
#include "rtthread.h"
// You should init rtthread instead of using this!
#define init_module_heap(ptr, size) (YOU_SHOULD_CONFIG_RTTHREAD_INSTEAD)
#define m_alloc(size) rt_malloc(size)
#define m_free(ptr) rt_free(ptr)
#define m_realloc(ptr, size) rt_realloc(ptr, size)
#else
#error "MODCFG__HEAP_MATHOD invalid"
#endif

#if MOD_CFG_USE_OS_NONE  // none
#define MOD_MUTEX_HANDLE volatile uint8_t
#define MOD_MUTEX_CREATE(name) (0)
#define MOD_MUTEX_ACQUIRE(mutex) \
  do {                           \
    while (mutex)                \
      ;                          \
    mutex = 1;                   \
  } while (0)
static inline bool _MOD_MUTEX_TRY_ACQUIRE(MOD_MUTEX_HANDLE *mutex,
                                          uint32_t ms) {
  m_time_t start = m_time_ms();
  while (*mutex) {
    if (m_time_ms() - start > ms) {
      return false;
    }
  }
  *mutex = 1;
  return true;
}
#define MOD_MUTEX_TRY_ACQUIRE(mutex, ms) _MOD_MUTEX_TRY_ACQUIRE(&mutex, ms)
#define MOD_MUTEX_RELEASE(mutex) mutex = 0
#define MOD_MUTEX_DELETE(mutex) ((void)0)

#define MOD_SEM_HANDLE volatile uint8_t
#define MOD_SEM_CREATE(name, init) (init)
#define MOD_SEM_TAKE(sem) \
  do {                    \
    while (!sem)          \
      ;                   \
    sem--;                \
  } while (0)
static inline bool _MOD_SEM_TRY_TAKE(MOD_SEM_HANDLE *sem, uint32_t ms) {
  m_time_t start = m_time_ms();
  while (!*sem) {
    if (m_time_ms() - start > ms) {
      return false;
    }
  }
  (*sem)--;
  return true;
}
#define MOD_SEM_TRY_TAKE(sem, ms) _MOD_SEM_TRY_TAKE(&sem, ms)
#define MOD_SEM_GIVE(sem) (sem++)
#define MOD_SEM_VALUE(sem) (sem)
#define MOD_SEM_DELETE(sem) ((void)0)
#elif MOD_CFG_USE_OS_KLITE  // klite
#include "klite.h"
#define MOD_MUTEX_HANDLE mutex_t
#define MOD_MUTEX_CREATE(name) mutex_create()
#define MOD_MUTEX_ACQUIRE(mutex) mutex_lock(mutex)
#define MOD_MUTEX_TRY_ACQUIRE(mutex, ms) \
  mutex_timed_lock(mutex, kernel_ms_to_ticks(ms))
#define MOD_MUTEX_RELEASE(mutex) mutex_unlock(mutex)
#define MOD_MUTEX_DELETE(mutex) mutex_delete(mutex)

#define MOD_SEM_HANDLE sem_t
#define MOD_SEM_CREATE(name, init) sem_create(init)
#define MOD_SEM_TAKE(sem) sem_wait(sem)
#define MOD_SEM_TRY_TAKE(sem, ms) sem_timed_wait(sem, kernel_ms_to_ticks(ms))
#define MOD_SEM_GIVE(sem) sem_post(sem)
#define MOD_SEM_VALUE(sem) sem_value(sem)
#define MOD_SEM_DELETE(sem) sem_delete(sem)
#elif MOD_CFG_USE_OS_FREERTOS  // freertos
#include "FreeRTOS.h"
#define MOD_MUTEX_HANDLE SemaphoreHandle_t
#define MOD_MUTEX_CREATE(name) xSemaphoreCreateMutex()
#define MOD_MUTEX_ACQUIRE(mutex) xSemaphoreTake(mutex, portMAX_DELAY)
#define MOD_MUTEX_TRY_ACQUIRE(mutex, ms) \
  xSemaphoreTake(mutex, pdMS_TO_TICKS(ms))
#define MOD_MUTEX_RELEASE(mutex) xSemaphoreGive(mutex)
#define MOD_MUTEX_DELETE(mutex) vSemaphoreDelete(mutex)

#define MOD_SEM_HANDLE SemaphoreHandle_t
#define MOD_SEM_CREATE(name, init) xSemaphoreCreateCounting(0xFFFF, init)
#define MOD_SEM_TAKE(sem) xSemaphoreTake(sem, portMAX_DELAY)
#define MOD_SEM_TRY_TAKE(sem, ms) xSemaphoreTake(sem, pdMS_TO_TICKS(ms))
#define MOD_SEM_GIVE(sem) xSemaphoreGive(sem)
#define MOD_SEM_VALUE(sem) uxSemaphoreGetCount(sem)
#define MOD_SEM_DELETE(sem) vSemaphoreDelete(sem)
#elif MOD_CFG_USE_OS_RTT  // rtthread
#include "rtthread.h"
#define MOD_MUTEX_HANDLE rt_mutex_t
#define MOD_MUTEX_CREATE(name) rt_mutex_create(name, RT_IPC_FLAG_PRIO)
#define MOD_MUTEX_ACQUIRE(mutex) rt_mutex_take(mutex, RT_WAITING_FOREVER)
#define MOD_MUTEX_TRY_ACQUIRE(mutex, ms) \
  rt_mutex_take(mutex, rt_tick_from_millisecond(ms))
#define MOD_MUTEX_RELEASE(mutex) rt_mutex_release(mutex)
#define MOD_MUTEX_DELETE(mutex) rt_mutex_delete(mutex)

#define MOD_SEM_HANDLE rt_sem_t
#define MOD_SEM_CREATE(name, init) rt_sem_create(name, init, RT_IPC_FLAG_PRIO)
#define MOD_SEM_TAKE(sem) rt_sem_take(sem, RT_WAITING_FOREVER)
#define MOD_SEM_TRY_TAKE(sem, ms) rt_sem_take(sem, rt_tick_from_millisecond(ms))
#define MOD_SEM_GIVE(sem) rt_sem_release(sem)
#define MOD_SEM_VALUE(sem) rt_sem_get(sem)
#define MOD_SEM_DELETE(sem) rt_sem_delete(sem)
#else
#error "MOD_CFG_USE_OS invalid"
#endif

#ifndef __has_include
#define __has_include(x) 1  // Compatibility with non-clang compilers.
#endif

#ifdef __cplusplus
}
#endif
#endif  // __MODULES_H

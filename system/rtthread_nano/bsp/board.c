/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-07-24     Tanek        the first version
 * 2018-11-12     Ernest Chen  modify copyright
 */

#include <rtconfig.h>
#include <rthw.h>
#include <rtthread.h>
#include <stdint.h>

#include "modules.h"
#include "uni_io.h"

// Updates the variable SystemCoreClock and must be called
// whenever the core clock is changed during program execution.
extern void SystemCoreClockUpdate(void);

// Holds the system core clock, which is the system clock
// frequency supplied to the SysTick timer and the processor
// core clock.
extern uint32_t SystemCoreClock;

static uint32_t rt_SysTick_Config(rt_uint32_t ticks) {
  // if ((ticks - 1) > 0xFFFFFF) {
  //   return 1;
  // }

  // SysTick->LOAD = (uint32_t)(ticks - 1UL);

  // // Maximum priority
  // NVIC_SetPriority(SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);

  // SysTick->VAL = 0UL;
  // SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
  //                 SysTick_CTRL_ENABLE_Msk;
  // return 0;

  SysTick_Config(ticks);
  NVIC_SetPriority(SysTick_IRQn, 255);
  return 0;
}

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)

#if !defined(RT_HEAP_SIZE)
#define RT_HEAP_SIZE 1024 * 64
#endif

static uint8_t ALIGN(RT_ALIGN_SIZE) rt_heap[RT_HEAP_SIZE];

#endif

/**
 * This function will initial your board.
 */
void rt_hw_board_init() {
  /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
  rt_components_board_init();
#endif

  /* System Clock Update */
  SystemCoreClockUpdate();

  /* System Tick Configuration */
  rt_SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
  rt_system_heap_init(rt_heap, rt_heap + sizeof(rt_heap));
#endif
}

void SysTick_Handler(void) {
  /* enter interrupt */
  rt_interrupt_enter();

  rt_tick_increase();

  /* leave interrupt */
  rt_interrupt_leave();
}

void rt_hw_console_output(const char *str) { puts(str); }

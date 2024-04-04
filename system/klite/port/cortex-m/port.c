/******************************************************************************
 * Copyright (c) 2015-2023 jiangxiaogang<kerndev@foxmail.com>
 *
 * This file is part of KLite distribution.
 *
 * KLite is free software, you can redistribute it and/or modify it under
 * the MIT Licence.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "klite_internal.h"
#include "main.h"  // or manual specify cmsis header

#define NVIC_INT_CTRL (*((volatile uint32_t *)0xE000ED04))
#define PEND_INT_SET (1 << 28)

void kl_port_context_switch(void) { NVIC_INT_CTRL = PEND_INT_SET; }

void *kl_port_stack_init(void *stack_base, void *stack_top, void *entry,
                         void *arg, void *exit) {
  uint32_t *sp;
  sp = (uint32_t *)(((uint32_t)stack_top) & 0xFFFFFFF8);  // 8-byte align
  *(--sp) = 0x01000000UL;     // xPSR set to thumb mode
  *(--sp) = (uint32_t)entry;  // PC
  *(--sp) = (uint32_t)exit;   // R14(LR)
  *(--sp) = 0;                // R12
  *(--sp) = 0;                // R3
  *(--sp) = 0;                // R2
  *(--sp) = 0;                // R1
  *(--sp) = (uint32_t)arg;    // R0

#if (__ARM_ARCH == 4) || (__ARM_ARCH == 7)
  *(--sp) = 0xFFFFFFF9UL;  // LR(EXC_RETURN) set to thread mode
#endif

  // cortex-m3/4/7: R11, R10, R9, R8, R7, R6, R5, R4
  // cortex-m0:     R7, R6, R5, R4, R11, R10, R9, R8
  *(--sp) = 0;
  *(--sp) = 0;
  *(--sp) = 0;
  *(--sp) = 0;
  *(--sp) = 0;
  *(--sp) = 0;
  *(--sp) = 0;
  *(--sp) = 0;
  return sp;
}

static uint32_t m_critical_nesting;

void kl_port_enter_critical(void) {
  __disable_irq();
  m_critical_nesting++;
}

void kl_port_leave_critical(void) {
  if (m_critical_nesting == 0) return;
  m_critical_nesting--;
  if (!m_critical_nesting) __enable_irq();
}

void kl_port_sys_init(void) {
  kl_port_enter_critical();
  NVIC_SetPriority(PendSV_IRQn, 255);
  NVIC_SetPriority(SysTick_IRQn, 255);
}

void kl_port_sys_start(void) {
  SystemCoreClockUpdate();
  SysTick_Config(SystemCoreClock / KLITE_CFG_FREQ);
  kl_port_leave_critical();
}

void kl_port_sys_idle(kl_tick_t time) {
#if MOD_CFG_WFI_WHEN_SYSTEM_IDLE
  __wfi();
#endif
}

extern __IO uint32_t uwTick;
void SysTick_Handler(void) {
  kl_kernel_tick_source(1);

#if KLITE_CFG_FREQ >= 1000
  static uint16_t tick_scaler = 0;
  if (++tick_scaler >= (KLITE_CFG_FREQ / 1000)) {  // us -> ms
    uwTick++;                                      // for HAL_Delay()
    tick_scaler = 0;
  }
#else
  uwTick += 1000 / KLITE_CFG_FREQ;
#endif
}

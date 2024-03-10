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
#include "kernel.h"
// #error "Include CMSIS based device header here!"
#include "main.h"

static uint32_t m_sys_nesting;

void cpu_enter_critical(void) {
  __disable_irq();
  m_sys_nesting++;
}

void cpu_leave_critical(void) {
  m_sys_nesting--;
  if (!m_sys_nesting) __enable_irq();
}

void cpu_sys_init(void) {
  cpu_enter_critical();
  NVIC_SetPriority(PendSV_IRQn, 255);
  NVIC_SetPriority(SysTick_IRQn, 255);
}

void cpu_sys_start(void) {
  SystemCoreClockUpdate();
  SysTick_Config(SystemCoreClock / KERNEL_CFG_FREQ);
  cpu_leave_critical();
}

void cpu_sys_sleep(uint32_t time) {
// Call wfi() can enter low power mode
// But SysTick may be stopped after call wfi() on some device.
#if MOD_CFG_WFI_WHEN_SYSTEM_IDLE
  __wfi();
#endif
}

extern __IO uint32_t uwTick;
void SysTick_Handler(void) {
  kernel_tick(1);

#if KERNEL_CFG_FREQ >= 1000
  static uint16_t tick_scaler = 0;
  if (++tick_scaler >= (KERNEL_CFG_FREQ / 1000)) {  // us -> ms
    uwTick++;                                       // for HAL_Delay()
    tick_scaler = 0;
  }
#else
  uwTick += 1000 / KERNEL_CFG_FREQ;
#endif
}

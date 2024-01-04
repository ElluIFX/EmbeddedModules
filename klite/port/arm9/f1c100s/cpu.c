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
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
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
#include "arm.h"
#include "intc.h"
#include "f1c100s.h"

#define PENDING_IRQ IRQ_WATCHDOG
#define SYSTICK_IRQ IRQ_TIMER0

static uint32_t m_lock_nesting;
static uint32_t m_cpsr_backup;

static void pending_handler(void)
{
	intc_clear_pending(PENDING_IRQ);
}

static void systick_handler(void)
{
	kernel_tick(1);
	TIMER->ISR |= 0x01;
}

static void systick_init(void)
{
	TIMER->TIM0_CTRL &= ~0x01;
	TIMER->ISR |= 0x01;
	TIMER->IER |= 0x01;
	TIMER->TIM0_INTV = 24000;
	TIMER->TIM0_CTRL = 0x4;
	TIMER->TIM0_CTRL |= 0x02;
	while(TIMER->TIM0_CTRL & 0x02);
	TIMER->TIM0_CTRL |= 0x01;
}

void cpu_sys_init(void)
{
	arm_disable_irq();
	intc_init();
	intc_set_priority(SYSTICK_IRQ, 0); //0 is lowest
	intc_set_priority(PENDING_IRQ, 0);
	intc_set_handler(SYSTICK_IRQ, systick_handler);
	intc_set_handler(PENDING_IRQ, pending_handler);
	intc_enable_irq(SYSTICK_IRQ);
	intc_enable_irq(PENDING_IRQ);
}

void cpu_sys_start(void)
{
	systick_init();
	arm_enable_irq();
}

void cpu_sys_sleep(uint32_t time)
{

}

void cpu_enter_critical(void)
{
	uint32_t cpsr;
	cpsr = arm_read_cpsr_c();
	arm_write_cpsr_c(cpsr | 0x80);
	if(m_lock_nesting++ == 0)
	{
		m_cpsr_backup = cpsr;
	}
}

void cpu_leave_critical(void)
{
	if(--m_lock_nesting == 0)
	{
		arm_write_cpsr_c(m_cpsr_backup);
	}
}

void *cpu_contex_init(void *stack_base, void *stack_top, void *entry, void *arg, void *exit)
{
	uint32_t *sp;
	sp = (uint32_t *)(((uint32_t)stack_top) & 0xFFFFFFF8);
	*(--sp) = (uint32_t)entry;          // PC
	*(--sp) = (uint32_t)exit;           // LR
	*(--sp) = 7;                        // R7
	*(--sp) = 6;                        // R6
	*(--sp) = 5;                        // R5
	*(--sp) = 4;                        // R4
	*(--sp) = 3;                        // R3
	*(--sp) = 2;                        // R2
	*(--sp) = 1;                        // R1
	*(--sp) = (uint32_t)arg;            // R0
	*(--sp) = 12;                       // R12
	*(--sp) = 11;                       // R11
	*(--sp) = 10;                       // R10
	*(--sp) = 9;                        // R9
	*(--sp) = 8;                        // R8
	*(--sp) = 0x00000053;               // xPSR = SVC,FIQ=disable,IRQ=enable
	return sp;
}

void cpu_contex_switch(void)
{
	intc_set_pending(PENDING_IRQ);
}

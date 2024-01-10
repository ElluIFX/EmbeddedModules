/*******************************************************************************
* @file   intc driver
* @brief  intc driver for f1c100s
* @author kerndev@foxmail.com
*******************************************************************************/
#include <string.h>
#include "f1c100s.h"
#include "intc.h"

static uint32_t m_vector[IRQ_MAX];

void intc_init(void)
{
	memset(m_vector, 0, sizeof(m_vector));
	INTC->BASE_ADDR = (uint32_t)m_vector;
	INTC->PEND[0] = 0xFFFFFFFF;
	INTC->PEND[1] = 0xFFFFFFFF;
	INTC->EN[0] = 0;
	INTC->EN[1] = 0;
	INTC->FF[0] = 0;
	INTC->FF[1] = 0;
}

void intc_set_handler(int irq, void (*isr)(void))
{
	m_vector[irq] = (uint32_t)isr;
}

void intc_set_priority(int irq, int prio)
{
	int n;
	uint32_t mask;
	uint32_t set;
	n = irq >> 4;
	mask = 0x3 << ((irq & 0xF) << 1);
	set = prio << ((irq & 0xF) << 1);
	INTC->PRIO[n] &= mask;
	INTC->PRIO[n] |= set;
}

void intc_set_pending(int irq)
{
	if(irq < 32)
	{
		INTC->FF[0] |= (1 << irq);
	}
	else
	{
		irq -= 32;
		INTC->FF[1] |= (1 << irq);
	}
}

void intc_clear_pending(int irq)
{
	if(irq < 32)
	{
		INTC->FF[0] &= ~(1 << irq);
	}
	else
	{
		irq -= 32;
		INTC->FF[1] &= ~(1 << irq);
	}
}

void intc_enable_irq(int irq)
{
	if(irq < 32)
	{
		INTC->EN[0] |= (1 << irq);
	}
	else
	{
		irq -= 32;
		INTC->EN[1] |= (1 << irq);
	}
}

void intc_disable_irq(int irq)
{
	if(irq < 32)
	{
		INTC->EN[0] &= ~(1 << irq);
	}
	else
	{
		irq -= 32;
		INTC->EN[1] &= ~(1 << irq);
	}
}

void intc_handler(void)
{
	typedef void (*func_t)(void);
	func_t func;
	uint32_t *addr;
	addr = (uint32_t *)INTC->VECTOR;
	func = (func_t)*addr;
	func();
}

#include <stdint.h>
#include "arm.h"

__asm uint32_t arm_read_p15_c1(void)
{
	MRC p15, 0, R0, c1, c0, 0
	BX  LR
}

__asm void arm_write_p15_c1(uint32_t value)
{
	MCR p15, 0, R0, c1, c0, 0
	BX  LR
}

__asm void arm_set_ttb(uint32_t base)
{
	MCR p15, 0, R0, c2, c0, 0
	BX  LR
}

__asm uint32_t arm_get_ttb(void)
{
	MRC p15, 0, R0, c2, c0, 0
	BX  LR
}

__asm void arm_set_domain(uint32_t domain)
{
	MCR p15, 0, R0, c3, c0, 0
	BX  LR
}

__asm uint32_t arm_get_domain(void)
{
	MCR p15, 0, R0, c3, c0, 0
	BX  LR
}

__asm void arm_invalidate_tlb(void)
{
	PUSH {R0}
	MOV R0, #0
	MCR p15, 0, R0, c7, c10, 4
	MCR p15, 0, R0, c8, c6, 0
	MCR p15, 0, R0, c8, c5, 0
	POP {R0}
	BX  LR
}

void arm_enable_dcache(void)
{
	uint32_t value = arm_read_p15_c1();
	arm_write_p15_c1(value | (1 << 2));
}

void arm_disable_dcache(void)
{
	uint32_t value = arm_read_p15_c1();
	arm_write_p15_c1(value & ~(1 << 2));
}

void arm_enable_icache(void)
{
	uint32_t value = arm_read_p15_c1();
	arm_write_p15_c1(value | (1 << 12));
}

void arm_disable_icache(void)
{
	uint32_t value = arm_read_p15_c1();
	arm_write_p15_c1(value & ~(1 << 12));
}

void arm_enable_mmu(void)
{
	uint32_t value = arm_read_p15_c1();
	arm_write_p15_c1(value | (1 << 0));
}

void arm_disable_mmu(void)
{
	uint32_t value = arm_read_p15_c1();
	arm_write_p15_c1(value & ~(1 << 0));
}

__asm uint32_t arm_read_cpsr_c(void)
{
	MRS    R0, CPSR
	BX     LR
}

__asm void arm_write_cpsr_c(uint32_t value)
{
	MSR    CPSR_c, R0
	BX     LR
}

__asm void arm_enable_irq(void)
{
	MRS    R0, CPSR
	AND    R0, R0, #0x7F
	MSR    CPSR_c, R0
	BX     LR
}

__asm void arm_disable_irq(void)
{
	MRS    R0, CPSR
	ORR    R0, R0, #0x80
	MSR    CPSR_c, R0
	BX     LR
}

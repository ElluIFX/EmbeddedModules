#ifndef __ARM_H__
#define __ARM_H__
#include <stddef.h>
#include <stdint.h>

uint32_t arm_read_p15_c1(void);
void arm_write_p15_c1(uint32_t value);
void arm_set_ttb(uint32_t base);
uint32_t arm_get_ttb(void);
void arm_set_domain(uint32_t domain);
uint32_t arm_get_domain(void);
void arm_invalidate_tlb(void);
void arm_enable_dcache(void);
void arm_disable_dcache(void);
void arm_enable_icache(void);
void arm_disable_icache(void);
void arm_enable_mmu(void);
void arm_disable_mmu(void);
uint32_t arm_read_cpsr_c(void);
void arm_write_cpsr_c(uint32_t value);
void arm_enable_irq(void);
void arm_disable_irq(void);

#endif /* __ARM_H__ */

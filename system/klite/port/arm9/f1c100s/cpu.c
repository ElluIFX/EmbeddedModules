#include "arm.h"
#include "f1c100s.h"
#include "intc.h"
#include "kl_priv.h"

#define PENDING_IRQ IRQ_WATCHDOG
#define SYSTICK_IRQ IRQ_TIMER0

static uint32_t m_lock_nesting;
static uint32_t m_cpsr_backup;

static void pending_handler(void) {
    intc_clear_pending(PENDING_IRQ);
}

static void systick_handler(void) {
    kernel_tick(1);
    TIMER->ISR |= 0x01;
}

static void systick_init(void) {
    TIMER->TIM0_CTRL &= ~0x01;
    TIMER->ISR |= 0x01;
    TIMER->IER |= 0x01;
    TIMER->TIM0_INTV = 24000;
    TIMER->TIM0_CTRL = 0x4;
    TIMER->TIM0_CTRL |= 0x02;
    while (TIMER->TIM0_CTRL & 0x02)
        ;
    TIMER->TIM0_CTRL |= 0x01;
}

void kl_port_sys_init(void) {
    arm_disable_irq();
    intc_init();
    intc_set_priority(SYSTICK_IRQ, 0);  //0 is lowest
    intc_set_priority(PENDING_IRQ, 0);
    intc_set_handler(SYSTICK_IRQ, systick_handler);
    intc_set_handler(PENDING_IRQ, pending_handler);
    intc_enable_irq(SYSTICK_IRQ);
    intc_enable_irq(PENDING_IRQ);
}

void kl_port_sys_start(void) {
    systick_init();
    arm_enable_irq();
}

void kl_port_sys_idle(kl_tick_t time) {}

void kl_port_enter_critical(void) {
    uint32_t cpsr;
    cpsr = arm_read_cpsr_c();
    arm_write_cpsr_c(cpsr | 0x80);
    if (m_lock_nesting++ == 0) {
        m_cpsr_backup = cpsr;
    }
}

void kl_port_leave_critical(void) {
    if (--m_lock_nesting == 0) {
        arm_write_cpsr_c(m_cpsr_backup);
    }
}

void* kl_port_stack_init(void* stack_base, void* stack_top, void* entry,
                         void* arg, void* exit) {
    uint32_t* sp;
    sp = (uint32_t*)(((uint32_t)stack_top) & 0xFFFFFFF8);
    *(--sp) = (uint32_t)entry;  // PC
    *(--sp) = (uint32_t)exit;   // LR
    *(--sp) = 7;                // R7
    *(--sp) = 6;                // R6
    *(--sp) = 5;                // R5
    *(--sp) = 4;                // R4
    *(--sp) = 3;                // R3
    *(--sp) = 2;                // R2
    *(--sp) = 1;                // R1
    *(--sp) = (uint32_t)arg;    // R0
    *(--sp) = 12;               // R12
    *(--sp) = 11;               // R11
    *(--sp) = 10;               // R10
    *(--sp) = 9;                // R9
    *(--sp) = 8;                // R8
    *(--sp) = 0x00000053;       // xPSR = SVC,FIQ=disable,IRQ=enable
    return sp;
}

void kl_port_context_switch(void) {
    intc_set_pending(PENDING_IRQ);
}

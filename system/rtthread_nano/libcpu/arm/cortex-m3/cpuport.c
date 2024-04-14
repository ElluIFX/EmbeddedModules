/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date         Author      Notes
 * 2009-01-05   Bernard     first version
 * 2011-02-14   onelife     Modify for EFM32
 * 2011-06-17   onelife     Merge all of the C source code into cpuport.c
 * 2012-12-23   aozima      stack addr align to 8byte.
 * 2012-12-29   Bernard     Add exception hook.
 * 2013-07-09   aozima      enhancement hard fault exception handler.
 * 2019-07-03   yangjie     add __rt_ffs() for armclang.
 */

#include <rtthread.h>

struct exception_stack_frame {
    rt_uint32_t r0;
    rt_uint32_t r1;
    rt_uint32_t r2;
    rt_uint32_t r3;
    rt_uint32_t r12;
    rt_uint32_t lr;
    rt_uint32_t pc;
    rt_uint32_t psr;
};

struct stack_frame {
    /* r4 ~ r11 register */
    rt_uint32_t r4;
    rt_uint32_t r5;
    rt_uint32_t r6;
    rt_uint32_t r7;
    rt_uint32_t r8;
    rt_uint32_t r9;
    rt_uint32_t r10;
    rt_uint32_t r11;

    struct exception_stack_frame exception_stack_frame;
};

/* flag in interrupt handling */
rt_uint32_t rt_interrupt_from_thread, rt_interrupt_to_thread;
rt_uint32_t rt_thread_switch_interrupt_flag;
/* exception hook */
static rt_err_t (*rt_exception_hook)(void* context) = RT_NULL;

/**
 * This function will initialize thread stack
 *
 * @param tentry the entry of thread
 * @param parameter the parameter of entry
 * @param stack_addr the beginning stack address
 * @param texit the function will be called when thread exit
 *
 * @return stack address
 */
rt_uint8_t* rt_hw_stack_init(void* tentry, void* parameter,
                             rt_uint8_t* stack_addr, void* texit) {
    struct stack_frame* stack_frame;
    rt_uint8_t* stk;
    unsigned long i;

    stk = stack_addr + sizeof(rt_uint32_t);
    stk = (rt_uint8_t*)RT_ALIGN_DOWN((rt_uint32_t)stk, 8);
    stk -= sizeof(struct stack_frame);

    stack_frame = (struct stack_frame*)stk;

    /* init all register */
    for (i = 0; i < sizeof(struct stack_frame) / sizeof(rt_uint32_t); i++) {
        ((rt_uint32_t*)stack_frame)[i] = 0xdeadbeef;
    }

    stack_frame->exception_stack_frame.r0 =
        (unsigned long)parameter;               /* r0 : argument */
    stack_frame->exception_stack_frame.r1 = 0;  /* r1 */
    stack_frame->exception_stack_frame.r2 = 0;  /* r2 */
    stack_frame->exception_stack_frame.r3 = 0;  /* r3 */
    stack_frame->exception_stack_frame.r12 = 0; /* r12 */
    stack_frame->exception_stack_frame.lr = (unsigned long)texit; /* lr */
    stack_frame->exception_stack_frame.pc =
        (unsigned long)tentry;                            /* entry point, pc */
    stack_frame->exception_stack_frame.psr = 0x01000000L; /* PSR */

    /* return task's current stack address */
    return stk;
}

/**
 * This function set the hook, which is invoked on fault exception handling.
 *
 * @param exception_handle the exception handling hook function.
 */
void rt_hw_exception_install(rt_err_t (*exception_handle)(void* context)) {
    rt_exception_hook = exception_handle;
}

#define SCB_CFSR \
    (*(volatile const unsigned*)0xE000ED28) /* Configurable Fault Status Register */
#define SCB_HFSR \
    (*(volatile const unsigned*)0xE000ED2C) /* HardFault Status Register */
#define SCB_MMAR \
    (*(volatile const unsigned*)0xE000ED34) /* MemManage Fault Address register */
#define SCB_BFAR \
    (*(volatile const unsigned*)0xE000ED38) /* Bus Fault Address Register */
#define SCB_AIRCR \
    (*(volatile unsigned long*)0xE000ED0C) /* Reset control Address Register */
#define SCB_RESET_VALUE \
    0x05FA0004 /* Reset value, write to SCB_AIRCR can reset cpu */

#define SCB_CFSR_MFSR \
    (*(volatile const unsigned char*)0xE000ED28) /* Memory-management Fault Status Register */
#define SCB_CFSR_BFSR \
    (*(volatile const unsigned char*)0xE000ED29) /* Bus Fault Status Register */
#define SCB_CFSR_UFSR \
    (*(volatile const unsigned short*)0xE000ED2A) /* Usage Fault Status Register */

struct exception_info {
    rt_uint32_t exc_return;
    struct stack_frame stack_frame;
};

/*
 * fault exception handler
 */
void rt_hw_hard_fault_exception(struct exception_info* exception_info) {
    extern long list_thread(void);
    struct stack_frame* context = &exception_info->stack_frame;

    if (rt_exception_hook != RT_NULL) {
        rt_err_t result;

        result = rt_exception_hook(exception_info);
        if (result == RT_EOK)
            return;
    }

    rt_kprintf("psr: 0x%08x\n", context->exception_stack_frame.psr);

    rt_kprintf("r00: 0x%08x\n", context->exception_stack_frame.r0);
    rt_kprintf("r01: 0x%08x\n", context->exception_stack_frame.r1);
    rt_kprintf("r02: 0x%08x\n", context->exception_stack_frame.r2);
    rt_kprintf("r03: 0x%08x\n", context->exception_stack_frame.r3);
    rt_kprintf("r04: 0x%08x\n", context->r4);
    rt_kprintf("r05: 0x%08x\n", context->r5);
    rt_kprintf("r06: 0x%08x\n", context->r6);
    rt_kprintf("r07: 0x%08x\n", context->r7);
    rt_kprintf("r08: 0x%08x\n", context->r8);
    rt_kprintf("r09: 0x%08x\n", context->r9);
    rt_kprintf("r10: 0x%08x\n", context->r10);
    rt_kprintf("r11: 0x%08x\n", context->r11);
    rt_kprintf("r12: 0x%08x\n", context->exception_stack_frame.r12);
    rt_kprintf(" lr: 0x%08x\n", context->exception_stack_frame.lr);
    rt_kprintf(" pc: 0x%08x\n", context->exception_stack_frame.pc);

    if (exception_info->exc_return & (1 << 2)) {
        rt_kprintf("hard fault on thread: %s\r\n\r\n", rt_thread_self()->name);
    } else {
        rt_kprintf("hard fault on handler\r\n\r\n");
    }

    while (1)
        ;
}

/**
 * shutdown CPU
 */
void rt_hw_cpu_shutdown(void) {
    rt_kprintf("shutdown...\n");

    RT_ASSERT(0);
}

/**
 * reset CPU
 */
RT_WEAK void rt_hw_cpu_reset(void) {
    SCB_AIRCR = SCB_RESET_VALUE;
}

#ifdef RT_USING_CPU_FFS
/**
 * This function finds the first bit set (beginning with the least significant bit)
 * in value and return the index of that bit.
 *
 * Bits are numbered starting at 1 (the least significant bit).  A return value of
 * zero from any of these functions means that the argument was zero.
 *
 * @return return the index of the first bit set. If value is 0, then this function
 * shall return 0.
 */
#if defined(__CC_ARM)
__asm int __rt_ffs(int value) {
    CMP r0,
# 0x00 BEQ exit

        RBIT r0, r0 CLZ r0, r0 ADDS r0, r0,
# 0x01

        exit BX lr
}
#elif defined(__CLANG_ARM)
int __rt_ffs(int value) {
    __asm volatile(
        "CMP     r0, #0x00            \n"
        "BEQ     1f                   \n"

        "RBIT    r0, r0               \n"
        "CLZ     r0, r0               \n"
        "ADDS    r0, r0, #0x01        \n"

        "1:                           \n"

        : "=r"(value)
        : "r"(value));
    return value;
}
#elif defined(__IAR_SYSTEMS_ICC__)
int __rt_ffs(int value) {
    if (value == 0)
        return value;

    asm("RBIT %0, %1" : "=r"(value) : "r"(value));
    asm("CLZ  %0, %1" : "=r"(value) : "r"(value));
    asm("ADDS %0, %1, #0x01" : "=r"(value) : "r"(value));

    return value;
}
#elif defined(__GNUC__)
int __rt_ffs(int value) {
    return __builtin_ffs(value);
}
#endif

#endif

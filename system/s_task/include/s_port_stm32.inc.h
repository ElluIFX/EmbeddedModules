#include "modules.h"

/* 3. Implement the initilization function for clock. Leave it blank if not
 * required. */
void my_clock_init() {}

/* 4. Implement the function of getting current clock ticks. */
my_clock_t my_clock() {
    return m_tick();
}

/* 5. Implement the idle delay function. */
void my_on_idle(uint64_t max_idle_ms) {
    // my_clock_t start = my_clock();
    // while (my_clock() - start < max_idle_ms * 1000) {
    //   __WFI();
    // }
}

#if defined __clang__ || defined __GNUC__

__attribute__((naked)) static void swapcontext(ucontext_t* old_tcb,
                                               const ucontext_t* new_tcb) {
    __asm__ __volatile__(
        "PUSH    {r4-r12,lr}\n"
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        "VPUSH   {s0-s31}\n"
        "VMRS    r2, fpscr\n"
        "PUSH    {r2}\n"
#endif
        "STR     sp, [r0]\n"
        //"LDR     r2, [r1]\n"
        //"MOV     sp, r2\n"
        "LDR     sp, [r1]\n"
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        "POP     {r2}\n"
        "VMSR    fpscr, r2\n"
        "VPOP    {s0-s31}\n"
#endif
        "POP     {r4-r12,lr}\n"

        "BX      lr\n");
}

#elif defined __ARMCC_VERSION
__asm static void swapcontext(ucontext_t* oucp, const ucontext_t* ucp) {
    PUSH{r4 - r12, lr}
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    VPUSH{S0 - S31} VMRS R2,
        FPSCR PUSH{R2}
#endif
    STR sp,
        [r0]

        // LDR     r2, [r1]
        // MOV     sp, r2
        LDR sp,
        [r1]
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        POP {
            R2
        } VMSR FPSCR,
        R2 VPOP {
            S0 - S31
        }
#endif
    POP{r4 - r12, lr}

    BX lr
}

#endif

static void create_context(ucontext_t* oucp, void* stack, size_t stack_size) {
    uint32_t* top_sp;
    uint32_t int_sp;

    int_sp = (int)((char*)stack + stack_size);
    int_sp = int_sp / 4 * 4;  // alignment
    top_sp = (uint32_t*)int_sp;

    top_sp[-1] = (int)&s_task_context_entry;
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)

    oucp->sp = (int)&top_sp[-43];
#else
    oucp->sp = (int)&top_sp[-10];
#endif
}

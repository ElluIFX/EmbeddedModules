#include "kl_priv.h"
#define LOG_MODULE "klite"
#include "log.h"

__weak void* kl_heap_alloc_fault_hook(kl_size_t size) {
    LOG_ERROR("failed to alloc %d on thread %d", size,
              kl_thread_id(kl_thread_self()));
    return NULL;
}

__weak void kl_stack_overflow_hook(kl_thread_t thread, bool is_bottom) {
    uart_fifo_tx_deinit(NULL);
    LOG_FATAL("stack overflow at %s of thread 0x%p (ID:%d Entry:%p)",
              is_bottom ? "bottom" : "top", thread, kl_thread_id(thread),
              thread->entry);
    while (1)
        ;
}

__weak void kl_kernel_idle_hook(void) {}

#include "kl_priv.h"
#include "log.h"

__weak void* kl_heap_alloc_fault_hook(kl_size_t size) {
  LOG_E("klite: failed to alloc %d", size);
  return NULL;
}

__weak void kl_stack_overflow_hook(kl_thread_t thread) {
  LOG_E("klite: stack overflow on thread-%d", kl_thread_id(thread));
}

__weak void kl_kernel_idle_hook(void) {}

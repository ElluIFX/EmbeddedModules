#include "kl_priv.h"

#define MAKE_VERSION_CODE(a, b, c) ((a << 24) | (b << 16) | (c))
#define KERNEL_VERSION_CODE MAKE_VERSION_CODE(5, 1, 0)

static uint64_t m_tick_count;
static kl_thread_t m_idle_thread;

void kl_kernel_init(void* heap_addr, kl_size_t heap_size) {
  m_tick_count = 0;
  m_idle_thread = NULL;
  kl_port_sys_init();
  kl_sched_init();
  kl_heap_init(heap_addr, heap_size);

  /* 创建idle线程 */
  m_idle_thread = kl_thread_create(kl_kernel_idle_entry, NULL,
                                   KLITE_CFG_IDLE_THREAD_STACK_SIZE, 0);
}

void kl_kernel_start(void) {
  kl_sched_switch();
  kl_port_sys_start();
}

void kl_kernel_tick_source(kl_tick_t time) {
  m_tick_count += time;
  kl_port_enter_critical();
  kl_sched_timing(time);
  kl_sched_preempt(true);
  kl_port_leave_critical();
}

void kl_kernel_idle_entry(void* args) {
  (void)args;

  while (1) {
    kl_kernel_idle_hook();
    kl_thread_idle_task();
    kl_port_enter_critical();
    kl_sched_idle();
    kl_port_leave_critical();
  }
}

kl_tick_t kl_kernel_idle_time(void) {
  return m_idle_thread ? kl_thread_time(m_idle_thread) : 0;
}

void kl_kernel_enter_critical(void) { kl_port_enter_critical(); }

void kl_kernel_exit_critical(void) { kl_port_leave_critical(); }

void kl_kernel_suspend_all(void) {
  kl_port_enter_critical();
  kl_sched_suspend();
  kl_port_leave_critical();
}

void kl_kernel_resume_all(void) {
  kl_port_enter_critical();
  kl_sched_resume();
  kl_port_leave_critical();
}

kl_tick_t kl_kernel_tick(void) { return (kl_tick_t)m_tick_count; }

uint64_t kl_kernel_tick64(void) { return m_tick_count; }

uint32_t kl_kernel_version(void) { return KERNEL_VERSION_CODE; }

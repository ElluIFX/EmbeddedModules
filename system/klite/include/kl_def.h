#ifndef __KLITE_DEF_H
#define __KLITE_DEF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kl_cfg.h"

#if KLITE_CFG_64BIT_TICK
typedef uint64_t kl_tick_t;
#define KL_WAIT_FOREVER UINT64_MAX
#else
typedef uint32_t kl_tick_t;
#define KL_WAIT_FOREVER UINT32_MAX
#endif

typedef uint32_t kl_size_t;

#define KL_THREAD_FLAGS_READY 0x01
#define KL_THREAD_FLAGS_SLEEP 0x02
#define KL_THREAD_FLAGS_WAIT 0x04
#define KL_THREAD_FLAGS_SUSPEND 0x08
#define KL_THREAD_FLAGS_EXITED 0x10

struct kl_thread_node {
  struct kl_thread_node *prev;
  struct kl_thread_node *next;
  struct kl_thread *tcb;
};
struct kl_thread_list {
  struct kl_thread_node *head;
  struct kl_thread_node *tail;
};
struct kl_thread {
  void *stack;                        // 栈基地址
  kl_size_t stack_size;               // 栈大小
  void (*entry)(void *);              // 线程入口
  uint32_t prio;                      // 线程优先级
  kl_tick_t time;                     // 线程运行时间
  kl_tick_t timeout;                  // 睡眠超时时间
  struct kl_thread_list *list_sched;  // 当前所处调度队列
  struct kl_thread_list *list_wait;   // 当前所处等待队列
  struct kl_thread_node node_sched;   // 调度队列节点
  struct kl_thread_node node_wait;    // 等待队列节点
  struct kl_thread_node node_manage;  // 管理队列节点
  uint32_t id_flags;                  // 高24位: ID, 低8位: FLAGS
};
typedef struct kl_thread *kl_thread_t;

struct kl_heap_stats {
  kl_size_t total_size;
  kl_size_t avail_size;
  kl_size_t largest_free;
  kl_size_t smallest_free;
  kl_size_t free_blocks;
  kl_size_t minimum_ever_avail;
  kl_size_t alloc_count;
  kl_size_t free_count;
};
typedef struct kl_heap_stats *kl_heap_stats_t;

#if KLITE_CFG_OPT_SEM
struct kl_sem {
  struct kl_thread_list list;
  uint32_t value;
};
typedef struct kl_sem *kl_sem_t;
#endif

#if KLITE_CFG_OPT_MUTEX
struct kl_event {
  struct kl_thread_list list;
  bool auto_reset;
  bool state;
};
typedef struct kl_event *kl_event_t;
#endif

#if KLITE_CFG_OPT_MUTEX
struct kl_mutex {
  struct kl_thread_list list;
  struct kl_thread *owner;
  uint32_t lock;
};
typedef struct kl_mutex *kl_mutex_t;
#endif

#if KLITE_CFG_OPT_COND
struct kl_cond {
  struct kl_thread_list list;
};
typedef struct kl_cond *kl_cond_t;
#endif

#if KLITE_CFG_OPT_RWLOCK
struct kl_rwlock {
  kl_mutex_t mutex;
  kl_cond_t read;
  kl_cond_t write;
  uint32_t read_wait_count;   // 等待读锁数量
  uint32_t write_wait_count;  // 等待写锁数量
  int32_t rw_count;           // -1:写锁 0:无锁 >0:读锁数量
};
typedef struct kl_rwlock *kl_rwlock_t;
#endif

#if KLITE_CFG_OPT_BARRIER
struct kl_barrier {
  struct kl_thread_list list;
  uint32_t value;
  uint32_t target;
};
typedef struct kl_barrier *kl_barrier_t;
#endif

#if KLITE_CFG_OPT_EVENT_FLAGS
#define KL_EVENT_FLAGS_WAIT_ANY 0x00
#define KL_EVENT_FLAGS_WAIT_ALL 0x01
#define KL_EVENT_FLAGS_AUTO_RESET 0x02
struct kl_event_flags {
  kl_mutex_t mutex;
  kl_cond_t cond;
  uint32_t bits;
};
typedef struct kl_event_flags *kl_event_flags_t;
#endif

#if KLITE_CFG_OPT_MAILBOX
struct kl_mailbox {
  struct {
    uint8_t *buf;
    kl_size_t size;
    kl_size_t wp;
    kl_size_t rp;
  } fifo;
  kl_mutex_t mutex;
  kl_cond_t write;  // 新可写空间
  kl_cond_t read;   // 新可读数据
};
typedef struct kl_mailbox *kl_mailbox_t;
#endif

#if KLITE_CFG_OPT_MPOOL
struct kl_mpool {
  kl_mutex_t mutex;
  kl_cond_t wait;
  uint8_t **block_list;
  kl_size_t block_count;
  kl_size_t free_count;
  kl_size_t free_head;
  kl_size_t free_tail;
};
typedef struct kl_mpool *kl_mpool_t;
#endif

#if KLITE_CFG_OPT_MSG_QUEUE
struct kl_mqueue_node {
  struct kl_mqueue_node *prev;
  struct kl_mqueue_node *next;
  uint8_t data[];
};
struct kl_mqueue {
  struct kl_mqueue_node *head;
  struct kl_mqueue_node *tail;
  kl_sem_t sem;
  kl_mutex_t mutex;
  kl_mpool_t mpool;
  kl_size_t size;
};
typedef struct kl_mqueue *kl_mqueue_t;
#endif

#if KLITE_CFG_OPT_SOFT_TIMER
struct kl_timer_task {
  struct kl_timer_task *prev;
  struct kl_timer_task *next;
  struct kl_timer *timer;
  void (*handler)(void *);
  void *arg;
  kl_tick_t reload;
  kl_tick_t timeout;
};
typedef struct kl_timer_task *kl_timer_task_t;
struct kl_timer {
  struct kl_timer_task *head;
  struct kl_timer_task *tail;
  kl_mutex_t mutex;
  kl_event_t event;
  kl_thread_t thread;
};
typedef struct kl_timer *kl_timer_t;
#endif

#if KLITE_CFG_OPT_THREAD_POOL
struct kl_thread_pool {
  kl_mqueue_t task_queue;
  kl_thread_t *thread_list;
  kl_sem_t idle_sem;
  kl_size_t worker_num;
};
typedef struct kl_thread_pool *kl_thread_pool_t;
#endif

#endif /* KLITE_DEF_H */

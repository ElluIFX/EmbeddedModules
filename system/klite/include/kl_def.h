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

#define KL_INVALID 0xFFFFFFFFU  // 无效返回值

typedef enum {
    KL_EOK,       // 没有错误
    KL_ERROR,     // 未知错误
    KL_EINVAL,    // 参数非法
    KL_ETIMEOUT,  // 超时错误
    KL_ENOMEM,    // 内存不足
    KL_ESIZE,     // 大小错误
    KL_EFULL,     // 资源满
    KL_EEMPTY,    // 资源空
    KL_EBUSY,     // 系统忙
    KL_EIO,       // IO错误
    KL_EPERM,     // 权限不足
    KL_ENOTSUP,   // 不支持
    KL_ENFOUND,   // 未找到
} kl_err_t;

typedef uint32_t kl_size_t;
typedef int32_t kl_ssize_t;

#define KL_THREAD_FLAGS_READY (1U << 0)
#define KL_THREAD_FLAGS_SLEEP (1U << 1)
#define KL_THREAD_FLAGS_WAIT (1U << 2)
#define KL_THREAD_FLAGS_SUSPEND (1U << 3)
#define KL_THREAD_FLAGS_EXITED (1U << 4)

struct kl_thread_node {
    struct kl_thread_node* prev;
    struct kl_thread_node* next;
    struct kl_thread* tcb;
};

struct kl_thread_list {
    struct kl_thread_node* head;
    struct kl_thread_node* tail;
};

struct kl_thread {
    void* stack;           // 栈基地址
    kl_size_t stack_size;  // 栈大小
    void (*entry)(void*);  // 线程入口
    uint32_t magic : 16;   // 线程魔数
    uint32_t prio : 16;    // 线程优先级
    uint32_t tid : 16;     // 线程ID
    uint32_t err : 8;      // 错误码
    uint32_t flags : 8;    // 线程状态
    kl_tick_t time;        // 线程运行时间
    kl_tick_t timeout;     // 睡眠超时时间
#if KLITE_CFG_ROUND_ROBIN_SLICE
    kl_tick_t slice;       // 抢占时间片
    kl_tick_t slice_tick;  // 时间片计数
#endif
#if KLITE_CFG_MLFQ
    kl_tick_t mlfq_tick;  // MLFQ配额计数
#endif
    struct kl_thread_list* list_sched;  // 当前所处调度队列
    struct kl_thread_list* list_wait;   // 当前所处等待队列
    struct kl_thread_node node_sched;   // 调度队列节点
    struct kl_thread_node node_wait;    // 等待队列节点
    struct kl_thread_node node_manage;  // 管理队列节点
    struct kl_thread_list list_join;    // 等待本线程结束的线程列表
};
typedef struct kl_thread* kl_thread_t;

struct kl_heap_stats {
    kl_size_t total_size;
    kl_size_t avail_size;
    kl_size_t largest_free;
    kl_size_t second_largest_free;
    kl_size_t smallest_free;
    kl_size_t free_blocks;
    kl_size_t minimum_ever_avail;
    kl_size_t alloc_count;
    kl_size_t free_count;
};
typedef struct kl_heap_stats* kl_heap_stats_t;

#if KLITE_CFG_IPC_SEM
struct kl_sem {
    struct kl_thread_list list;
    kl_size_t value;
};
typedef struct kl_sem* kl_sem_t;
#endif

#if KLITE_CFG_IPC_EVENT
struct kl_event {
    struct kl_thread_list list;
    bool auto_reset;
    bool state;
};
typedef struct kl_event* kl_event_t;
#endif

#if KLITE_CFG_IPC_MUTEX
struct kl_mutex {
#if KLITE_CFG_TRACE_MUTEX_OWNER
    struct kl_mutex* next;
#endif
    struct kl_thread_list list;
    struct kl_thread* owner;
    kl_size_t lock;
};
typedef struct kl_mutex* kl_mutex_t;
#endif

#if KLITE_CFG_IPC_COND
struct kl_cond {
    struct kl_thread_list list;
};
typedef struct kl_cond* kl_cond_t;
#endif

#if KLITE_CFG_IPC_RWLOCK
struct kl_rwlock {
    struct kl_mutex mutex;
    struct kl_cond read;
    struct kl_cond write;
    kl_thread_t writer;          // 写锁持有者
    kl_size_t read_wait_count;   // 等待读锁数量
    kl_size_t write_wait_count;  // 等待写锁数量
    kl_ssize_t rw_count;         // <0:写锁 0:无锁 >0:读锁
};
typedef struct kl_rwlock* kl_rwlock_t;
#endif

#if KLITE_CFG_IPC_BARRIER
struct kl_barrier {
    struct kl_thread_list list;
    kl_size_t value;
    kl_size_t target;
};
typedef struct kl_barrier* kl_barrier_t;
#endif

#if KLITE_CFG_IPC_EVENT_FLAGS
#define KL_EVENT_FLAGS_WAIT_ANY 0x00
#define KL_EVENT_FLAGS_WAIT_ALL 0x01
#define KL_EVENT_FLAGS_AUTO_RESET 0x02

struct kl_event_flags {
    struct kl_mutex mutex;
    struct kl_cond cond;
    kl_size_t bits;
};
typedef struct kl_event_flags* kl_event_flags_t;
#endif

#if KLITE_CFG_IPC_MAILBOX
struct kl_mailbox {
    struct {
        uint8_t* buf;
        kl_size_t size;
        kl_size_t wp;
        kl_size_t rp;
    } fifo;
    struct kl_mutex mutex;
    struct kl_cond write;  // 新可写空间
    struct kl_cond read;   // 新可读数据
};
typedef struct kl_mailbox* kl_mailbox_t;
#endif

#if KLITE_CFG_IPC_MPOOL
struct kl_mpool_node {
    struct kl_mpool_node* next;
    uint8_t data[];
};

struct kl_mpool {
    struct {
        struct kl_mpool_node* head;
    } free_list;
    struct kl_mutex mutex;
    struct kl_cond wait;
};
typedef struct kl_mpool* kl_mpool_t;
#endif

#if KLITE_CFG_IPC_MQUEUE
struct kl_mqueue_node {
    struct kl_mqueue_node* next;
    uint8_t data[];
};

struct kl_mqueue {
    struct {
        struct kl_mqueue_node* head;
    } msg_list;

    struct {
        struct kl_mqueue_node* head;
    } empty_list;
    struct kl_mutex mutex;
    struct kl_cond write;  // 新可写空间
    struct kl_cond read;   // 新可读数据
    struct kl_cond join;   // 任务完成
    kl_size_t size;        // 消息大小
    kl_size_t pending;     // 任务数量
};
typedef struct kl_mqueue* kl_mqueue_t;
#endif

#if KLITE_CFG_IPC_TIMER
struct kl_timer_task {
    struct kl_timer_task* next;
    struct kl_timer* timer;
    void (*handler)(void*);
    void* arg;
    kl_tick_t reload;
    kl_tick_t timeout;
    bool loop;
};
typedef struct kl_timer_task* kl_timer_task_t;

struct kl_timer {
    struct kl_timer_task* head;
    struct kl_mutex mutex;
    struct kl_cond cond;
    kl_thread_t thread;
};
typedef struct kl_timer* kl_timer_t;
#endif

#if KLITE_CFG_IPC_THREAD_POOL
struct kl_thread_pool {
    kl_mqueue_t task_queue;
    kl_thread_t* thread_list;
    kl_size_t worker_num;
};
typedef struct kl_thread_pool* kl_thread_pool_t;
#endif

#endif /* KLITE_DEF_H */

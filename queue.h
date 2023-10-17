/**
 * @file queue.h
 * @brief see queue.c for details.
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-01-07
 *
 * THINK DIFFERENTLY
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <macro.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define QUEUE_INT_SAFE 0  // Disable interrupt during variable access.

#define __DEQUEUE_0(__QUEUE)                     \
  ({                                             \
    uint8_t SAFE_NAME(temp);                     \
    queue_out_byte((__QUEUE), &SAFE_NAME(temp)); \
    SAFE_NAME(temp);                             \
  })

#define __DEQUEUE_1(__QUEUE, __ADDR) \
  queue_out((__QUEUE), __ADDR, (sizeof(typeof(*(__ADDR)))))

#define __DEQUEUE_2(__QUEUE, __ADDR, __ITEM_COUNT) \
  queue_out((__QUEUE), (__ADDR), __ITEM_COUNT * (sizeof(typeof((__ADDR[0])))))

#define __DEQUEUE_3(__QUEUE, __ADDR, __ITEM_COUNT, __TYPE) \
  queue_out((__QUEUE), (__ADDR), (__ITEM_COUNT * sizeof(__TYPE)))

#define __ENQUEUE_0__(__QUEUE, __VALUE)                          \
  do {                                                           \
    typeof((__VALUE)) SAFE_NAME(value) = __VALUE;                \
    queue_in((__QUEUE), &(SAFE_NAME(value)), (sizeof(__VALUE))); \
  } while (0)

#define __ENQUEUE_0(__QUEUE, __VALUE) __ENQUEUE_0__(__QUEUE, __VALUE)

#define __ENQUEUE_1(__QUEUE, __ADDR, __ITEM_COUNT) \
  queue_in((__QUEUE), (__ADDR), __ITEM_COUNT * (sizeof(typeof((__ADDR[0])))))

#define __ENQUEUE_2(__QUEUE, __ADDR, __ITEM_COUNT, __TYPE) \
  queue_in((__QUEUE), (__ADDR), (__ITEM_COUNT * sizeof(__TYPE)))

#define __QUEUE_PEEK_0(__QUEUE)                   \
  ({                                              \
    uint8_t SAFE_NAME(temp);                      \
    queue_peek_byte((__QUEUE), &SAFE_NAME(temp)); \
    SAFE_NAME(temp);                              \
  })
#define __QUEUE_PEEK_1(__QUEUE, __ADDR) \
  queue_peek((__QUEUE), (__ADDR), (sizeof(typeof(*(__ADDR)))))

#define __QUEUE_PEEK_2(__QUEUE, __ADDR, __ITEM_COUNT) \
  queue_peek((__QUEUE), (__ADDR), __ITEM_COUNT * (sizeof(typeof((__ADDR[0])))))

#define __QUEUE_PEEK_3(__QUEUE, __ADDR, __ITEM_COUNT, __TYPE) \
  queue_peek((__QUEUE), (__ADDR), (__ITEM_COUNT * sizeof(__TYPE)))

#define __QUEUE_INIT_0(__QUEUE, __BUFFER) \
  queue_init((__QUEUE), (__BUFFER), sizeof((__BUFFER)))

#define __QUEUE_INIT_1(__QUEUE, __BUFFER, __SIZE) \
  queue_init((__QUEUE), (__BUFFER), (__SIZE))

// 初始化队列 args: 队列指针 缓冲区指针 [缓冲区大小]
#define QUEUE_INIT(__QUEUE, __BUFFER, ...) \
  EVAL(__QUEUE_INIT_, ##__VA_ARGS__)       \
  (__QUEUE, __BUFFER, ##__VA_ARGS__)

// 创建队列 args: 队列缓冲区大小 return: 队列指针
#define QUEUE_CREATE(__SIZE)                                     \
  ({                                                             \
    static uint8_t SAFE_NAME(queue_buf)[__SIZE];                 \
    static queue_t SAFE_NAME(queue);                             \
    QUEUE_INIT(&SAFE_NAME(queue), SAFE_NAME(queue_buf), __SIZE); \
    &SAFE_NAME(queue);                                           \
  })

// 入队 args: 队列指针 数据值/(缓冲区指针 数据个数 [数据类型])
#define ENQUEUE(__QUEUE, __ADDR, ...) \
  EVAL(__ENQUEUE_, ##__VA_ARGS__)     \
  (__QUEUE, __ADDR, ##__VA_ARGS__)

// 出队 args: 队列指针 [缓冲区指针] [数据个数] [数据类型]
#define DEQUEUE(__QUEUE, ...)     \
  EVAL(__DEQUEUE_, ##__VA_ARGS__) \
  (__QUEUE, ##__VA_ARGS__)

// 查看队列 args: 队列指针 [缓冲区指针] [数据个数] [数据类型]
#define QUEUE_PEEK(__QUEUE, ...)     \
  EVAL(__QUEUE_PEEK_, ##__VA_ARGS__) \
  (__QUEUE, ##__VA_ARGS__)

// 队列是否为空
#define QUEUE_IS_EMPTY(__QUEUE) queue_check_empty(__QUEUE)

// 重置查看点
#define QUEUE_PEEK_RESET(__QUEUE) queue_reset_peek_pos(__QUEUE)

// 出队所有已查看的数据
#define DEQUEUE_PEEKED(__QUEUE) queue_pop_peeked(__QUEUE)

// 获取查看点
#define QUEUE_PEEK_GET(__QUEUE) queue_get_peek_pos(__QUEUE)

// 设置查看点
#define QUEUE_PEEK_SET(__QUEUE, __COUNT) queue_set_peek_pos(__QUEUE, __COUNT)

// 获取队列中数据个数
#define QUEUE_COUNT(__QUEUE) queue_get_count(__QUEUE)

// 获取队列中剩余空间个数
#define QUEUE_AVAIL(__QUEUE) queue_get_available(__QUEUE)

typedef struct queue_t {
  uint8_t *pchBuffer;
  uint16_t hwSize;
  uint16_t hwHead;
  uint16_t hwTail;
  uint16_t hwLength;
  uint16_t hwPeek;
  uint16_t hwPeekLength;
} queue_t;
typedef struct queue_t *queue;

extern queue_t *queue_init(queue_t *ptObj, void *pBuffer, uint16_t hwItemSize);
extern bool queue_in_byte(queue_t *ptQueue, uint8_t chByte);
extern int16_t queue_in(queue_t *ptObj, void *pchByte, uint16_t hwLength);
extern bool queue_out_byte(queue_t *ptQueue, uint8_t *pchByte);
extern int16_t queue_out(queue_t *ptObj, void *pchByte, uint16_t hwLength);

extern bool queue_reset_peek_pos(queue_t *ptQueue);
extern uint16_t queue_get_peek_pos(queue_t *ptQueue);
extern bool queue_set_peek_pos(queue_t *ptQueue, uint16_t hwCount);
extern bool queue_pop_peeked(queue_t *ptQueue);
extern bool queue_peek_byte(queue_t *ptQueue, uint8_t *pchByte);
extern int16_t queue_peek(queue_t *ptObj, void *pchByte, uint16_t hwLength);

extern bool queue_check_empty(queue_t *ptQueue);
extern int16_t queue_get_count(queue_t *ptObj);
extern int16_t queue_get_available(queue_t *ptObj);

extern void queue_debug(queue_t *ptObj);

#undef __BYTE_QUEUE_CLASS_INHERIT__
#undef __BYTE_QUEUE_CLASS_IMPLEMENT__

#endif  // __QUEUE_H__

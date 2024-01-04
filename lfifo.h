/**,.,
 * @file lfifo.h
 * @brief 原子化的环形FIFO缓冲区
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-07-20
 *
 * THINK DIFFERENTLY
 */

#ifndef __LFIFO_H__
#define __LFIFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>  // memcpy

#include "modules.h"

#ifndef FIFO_DISABLE_ATOMIC
#define FIFO_DISABLE_ATOMIC 0  // 禁用原子操作
#endif

#ifndef FIFO_MEMCPY
#define FIFO_MEMCPY memcpy  // 内存拷贝函数
#endif

#if FIFO_DISABLE_ATOMIC
#define FIFO_INIT(var, val) (var) = (val)
#define FIFO_LOAD(var, type) (var)
#define FIFO_STORE(var, val, type) (var) = (val)
typedef uint16_t fifo_atomic_size_t;
#else
#ifndef __cplusplus
#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
typedef atomic_uint_fast16_t fifo_atomic_size_t;
#else
#include <atomic>
typedef std::atomic_uint_fast16_t fifo_atomic_size_t;
#endif  // __cplusplus
#endif  // FIFO_DISABLE_ATOMIC

typedef uint16_t fifo_size_t;
typedef int16_t fifo_offset_t;

typedef struct {          // FIFO对象
  uint8_t *buf;           // 缓冲区指针
  fifo_size_t size;       // 缓冲区大小
  fifo_atomic_size_t wr;  // 写指针
  fifo_atomic_size_t rd;  // 读指针
} lfifo_t;

/**
 * @brief 初始化FIFO, 使用动态缓冲区
 * @param  fifo             FIFO对象
 * @param  size             请求的FIFO大小
 * @retval 0                成功
 * @note  实际申请的空间为size+1
 */
extern int LFifo_Init(lfifo_t *fifo, fifo_size_t size);

/**
 * @brief 销毁动态缓冲区FIFO
 * @param  fifo             FIFO对象
 * @warning 禁止对静态缓冲区FIFO使用此函数
 */
extern void LFifo_Destory(lfifo_t *fifo);

/**
 * @brief 使用静态缓冲区初始化FIFO
 * @param  fifo             FIFO对象
 * @param  buffer           静态缓冲区指针
 * @param  size             静态缓冲区大小
 * @note  实际可用空间为size-1
 */
extern void LFifo_AssignBuf(lfifo_t *fifo, uint8_t *buffer, fifo_size_t size);

/**
 * @brief 获取FIFO的大小
 * @param  fifo             FIFO对象
 * @retval fifo_size_t         FIFO的大小
 * @note  返回的实际可用大小为缓冲区大小-1
 */
extern fifo_size_t LFifo_GetSize(lfifo_t *fifo);

/**
 * @brief 获取FIFO的空闲空间
 * @param  fifo             FIFO对象
 * @retval fifo_size_t         FIFO的空闲空间
 */
extern fifo_size_t LFifo_GetFree(lfifo_t *fifo);

/**
 * @brief 获取FIFO的已用空间
 * @param  fifo             FIFO对象
 * @retval fifo_size_t         FIFO的已用空间
 */
extern fifo_size_t LFifo_GetUsed(lfifo_t *fifo);

/**
 * @brief 清空FIFO并填充数据
 * @param  fifo             FIFO对象
 * @param  fill_data        清空FIFO时填充的数据
 */
extern void LFifo_ClearFill(lfifo_t *fifo, const uint8_t fill_data);

/**
 * @brief 清空FIFO
 * @param  fifo             FIFO对象
 */
extern void LFifo_Clear(lfifo_t *fifo);

/**
 * @brief 向FIFO中写入数据
 * @param  fifo             FIFO对象
 * @param  data             写入数据缓冲区指针
 * @param  len              期望写入的数据长度
 * @retval fifo_size_t         实际写入的数据长度
 */
extern fifo_size_t LFifo_Put(lfifo_t *fifo, uint8_t *data, fifo_size_t len);

/**
 * @brief 从FIFO中读取数据
 * @param  fifo             FIFO对象
 * @param  data             存放数据的缓冲区指针
 * @param  len              期望读取的数据长度
 * @retval fifo_size_t         实际读取的数据长度
 * @note 传入NULL指针可丢弃数据
 */
extern fifo_size_t LFifo_Get(lfifo_t *fifo, uint8_t *data, fifo_size_t len);

/**
 * @brief 查看FIFO中的数据, 不改变FIFO的状态
 * @param  fifo             FIFO对象
 * @param  offset           期望查看的数据偏移
 * @param  data             存放数据的缓冲区指针
 * @param  len              期望查看的数据长度
 * @retval fifo_size_t         实际查看的数据长度
 */
extern fifo_size_t LFifo_Peek(lfifo_t *fifo, fifo_size_t offset, uint8_t *data,
                             fifo_size_t len);

/**
 * @brief 向FIFO中写入一字节数据
 * @param  fifo             FIFO对象
 * @param  data             写入数据
 * @retval 0                成功
 */
extern int LFifo_PutByte(lfifo_t *fifo, uint8_t data);

/**
 * @brief 从FIFO中读取一字节数据
 * @param  fifo             FIFO对象
 * @retval int              读取的数据, 读取失败返回-1
 */
extern int LFifo_GetByte(lfifo_t *fifo);

/**
 * @brief 查看FIFO中的一字节数据, 不改变FIFO的状态
 * @param  fifo             FIFO对象
 * @param  offset           期望查看的数据偏移
 * @retval int              查看的数据, 查看失败返回-1
 */
extern int LFifo_PeekByte(lfifo_t *fifo, fifo_size_t offset);

/**
 * @brief 获取FIFO当前的写数据指针
 * @param  fifo             FIFO对象
 * @param  offset           期望获取的数据偏移
 * @retval uint8_t*          当前的写指针
 * @note 0偏移指针指向的是下一个将要写入的数据
 */
extern uint8_t *LFifo_GetWrPtr(lfifo_t *fifo, fifo_offset_t offset);

/**
 * @brief 获取FIFO当前的读数据指针
 * @param  fifo             FIFO对象
 * @param  offset           期望获取的数据偏移
 * @retval uint8_t*          当前的读指针
 * @note 0偏移指针指向的是下一个将要读出的数据
 */
extern uint8_t *LFifo_GetRdPtr(lfifo_t *fifo, fifo_offset_t offset);

/**
 * @brief 向右查找FIFO中的数据
 * @param  fifo             FIFO对象
 * @param  data             查找的数据缓冲区指针
 * @param  len              查找的数据长度
 * @param  r_offset         起始查找偏移
 * @retval fifo_offset_t       查找到的数据偏移, 查找失败返回-1
 */
extern fifo_offset_t LFifo_Find(lfifo_t *fifo, uint8_t *data, fifo_size_t len,
                               fifo_size_t r_offset);

#ifdef __cplusplus
}
#endif
#endif  // __LFIFO_H__

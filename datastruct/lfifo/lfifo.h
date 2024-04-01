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

#include "modules.h"

typedef uint32_t mod_size_t;
typedef int32_t mod_offset_t;

typedef struct {         // FIFO对象
  mod_atomic_size_t wr;  // 写指针
  mod_atomic_size_t rd;  // 读指针
  mod_size_t size;       // 缓冲区大小
  uint8_t *buf;          // 缓冲区指针
} lfifo_t;

/**
 * @brief 初始化FIFO, 使用动态缓冲区
 * @param  fifo             FIFO对象
 * @param  size             请求的FIFO大小
 * @retval 0                成功
 * @note  实际申请的空间为size+1
 */
extern int LFifo_Init(lfifo_t *fifo, mod_size_t size);

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
extern void LFifo_AssignBuf(lfifo_t *fifo, uint8_t *buffer, mod_size_t size);

/**
 * @brief 获取FIFO的大小
 * @param  fifo             FIFO对象
 * @retval mod_size_t         FIFO的大小
 * @note  返回的实际可用大小为缓冲区大小-1
 */
extern mod_size_t LFifo_GetSize(lfifo_t *fifo);

/**
 * @brief 获取FIFO的空闲空间
 * @param  fifo             FIFO对象
 * @retval mod_size_t         FIFO的空闲空间
 */
extern mod_size_t LFifo_GetFree(lfifo_t *fifo);

/**
 * @brief 获取FIFO的已用空间
 * @param  fifo             FIFO对象
 * @retval mod_size_t         FIFO的已用空间
 */
extern mod_size_t LFifo_GetUsed(lfifo_t *fifo);

/**
 * @brief 判断FIFO是否为空
 * @param  fifo             FIFO对象
 */
extern bool LFifo_IsEmpty(lfifo_t *fifo);

/**
 * @brief 判断FIFO是否已满
 * @param  fifo             FIFO对象
 */
extern bool LFifo_IsFull(lfifo_t *fifo);

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
 * @retval mod_size_t         实际写入的数据长度
 * @note 传入NULL指针可只更新FIFO状态(配合LFifo_GetWritePtr自行管理写指针)
 */
extern mod_size_t LFifo_Write(lfifo_t *fifo, uint8_t *data, mod_size_t len);

/**
 * @brief 从FIFO中读取数据
 * @param  fifo             FIFO对象
 * @param  data             存放数据的缓冲区指针
 * @param  len              期望读取的数据长度
 * @retval mod_size_t         实际读取的数据长度
 * @note 传入NULL指针可丢弃数据
 */
extern mod_size_t LFifo_Read(lfifo_t *fifo, uint8_t *data, mod_size_t len);

/**
 * @brief 查看FIFO中的数据, 不改变FIFO的状态
 * @param  fifo             FIFO对象
 * @param  offset           期望查看的数据偏移
 * @param  data             存放数据的缓冲区指针
 * @param  len              期望查看的数据长度
 * @retval mod_size_t         实际查看的数据长度
 */
extern mod_size_t LFifo_Peek(lfifo_t *fifo, mod_size_t offset, uint8_t *data,
                             mod_size_t len);

/**
 * @brief 向FIFO中写入一字节数据
 * @param  fifo             FIFO对象
 * @param  data             写入数据
 * @retval 0                成功
 */
extern int LFifo_WriteByte(lfifo_t *fifo, uint8_t data);

/**
 * @brief 从FIFO中读取一字节数据
 * @param  fifo             FIFO对象
 * @retval int              读取的数据, 读取失败返回-1
 */
extern int LFifo_ReadByte(lfifo_t *fifo);

/**
 * @brief 查看FIFO中的一字节数据, 不改变FIFO的状态
 * @param  fifo             FIFO对象
 * @param  offset           期望查看的数据偏移
 * @retval int              查看的数据, 查看失败返回-1
 */
extern int LFifo_PeekByte(lfifo_t *fifo, mod_size_t offset);

/**
 * @brief 获取FIFO当前的写数据指针
 * @param  fifo             FIFO对象
 * @param  offset           期望获取的数据偏移
 * @retval uint8_t*         当前的写指针
 * @note 0偏移指针指向的是下一个将要写入的数据
 */
extern uint8_t *LFifo_GetWritePtr(lfifo_t *fifo, mod_offset_t offset);

/**
 * @brief 获取FIFO当前的读数据指针
 * @param  fifo             FIFO对象
 * @param  offset           期望获取的数据偏移
 * @retval uint8_t*         当前的读指针
 * @note 0偏移指针指向的是下一个将要读出的数据
 */
extern uint8_t *LFifo_GetReadPtr(lfifo_t *fifo, mod_offset_t offset);

/**
 * @brief 向右查找FIFO中的数据
 * @param  fifo             FIFO对象
 * @param  data             查找的数据缓冲区指针
 * @param  len              查找的数据长度
 * @param  r_offset         起始查找偏移
 * @retval mod_offset_t    查找到的数据偏移, 查找失败返回-1
 */
extern mod_offset_t LFifo_Find(lfifo_t *fifo, uint8_t *data, mod_size_t len,
                               mod_size_t r_offset);

/**
 * @brief 申请内存连续的写入空间, 不改变FIFO状态
 * @param  fifo             FIFO对象
 * @param  len              返回可用空间的长度
 * @retval uint8_t*         可用空间的指针
 * @note 需严格确保同时只有一个生产者
 */
extern uint8_t *LFifo_AcquireLinearWrite(lfifo_t *fifo, mod_size_t *len);

/**
 * @brief 释放连续写入空间, 并更新FIFO状态
 * @param  fifo             FIFO对象
 * @param  len              实际写入的数据长度
 * @note 需严格确保同时只有一个生产者
 */
extern void LFifo_ReleaseLinearWrite(lfifo_t *fifo, mod_size_t len);

/**
 * @brief 申请内存连续的读取空间, 不改变FIFO状态
 * @param  fifo             FIFO对象
 * @param  len              返回可用空间的长度
 * @retval uint8_t*         可用空间的指针
 * @note 需严格确保同时只有一个消费者
 */
extern uint8_t *LFifo_AcquireLinearRead(lfifo_t *fifo, mod_size_t *len);

/**
 * @brief 释放连续读取空间, 并更新FIFO状态
 * @param  fifo             FIFO对象
 * @param  len              实际读取的数据长度
 * @note 需严格确保同时只有一个消费者
 */
extern void LFifo_ReleaseLinearRead(lfifo_t *fifo, mod_size_t len);

#ifdef __cplusplus
}
#endif
#endif  // __LFIFO_H__

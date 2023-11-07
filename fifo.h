/**,.,
 * @file fifo.h
 * @brief 简单的FIFO缓冲区实现
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-07-20
 *
 * THINK DIFFERENTLY
 */

#ifndef __FIFO_H__
#define __FIFO_H__
#include "modules.h"

typedef struct {
  uint8_t *buffer;
  uint16_t size;
  uint16_t wr;
  uint16_t rd;
} fifo_t;

/**
 * @brief 初始化FIFO, 使用静态缓冲区
 * @param  fifo               FIFO对象
 * @param  static_buffer_p    静态缓冲区指针
 * @param  size               静态缓冲区大小
 * @note  实际可用空间为size-1
 */
extern void Fifo_Init_Static(fifo_t *fifo, uint8_t *static_buffer_p,
                             uint16_t size);

/**
 * @brief 初始化FIFO, 使用动态缓冲区
 * @param  fifo             FIFO对象
 * @param  size             请求的缓冲区大小
 * @retval 0                成功
 * @note  实际申请的空间为size+1
 */
extern uint8_t Fifo_Init_Dynamic(fifo_t *fifo, uint16_t size);

/**
 * @brief 销毁动态缓冲区FIFO
 * @param  fifo             FIFO对象
 * @warning 禁止对静态缓冲区FIFO使用此函数
 */
extern void Fifo_Destory_Dynamic(fifo_t *fifo);

/**
 * @brief 获取FIFO的大小
 * @param  fifo             FIFO对象
 * @retval uint16_t         FIFO的大小
 * @note  返回的实际可用大小为缓冲区大小-1
 */
extern uint16_t Fifo_GetSize(fifo_t *fifo);

/**
 * @brief 获取FIFO的空闲空间
 * @param  fifo             FIFO对象
 * @retval uint16_t         FIFO的空闲空间
 */
extern uint16_t Fifo_GetFree(fifo_t *fifo);

/**
 * @brief 获取FIFO的已用空间
 * @param  fifo             FIFO对象
 * @retval uint16_t         FIFO的已用空间
 */
extern uint16_t Fifo_GetUsed(fifo_t *fifo);

/**
 * @brief 清空FIFO并填充数据
 * @param  fifo             FIFO对象
 * @param  fill_data        清空FIFO时填充的数据
 */
extern void Fifo_Fill(fifo_t *fifo, const uint8_t fill_data);

/**
 * @brief 清空FIFO
 * @param  fifo             FIFO对象
 */
extern void Fifo_Clear(fifo_t *fifo);

/**
 * @brief 向FIFO中写入数据
 * @param  fifo             FIFO对象
 * @param  data             写入数据缓冲区指针
 * @param  len              期望写入的数据长度
 * @retval uint16_t         实际写入的数据长度
 */
extern uint16_t Fifo_Put(fifo_t *fifo, uint8_t *data, uint16_t len);

/**
 * @brief 从FIFO中读取数据
 * @param  fifo             FIFO对象
 * @param  data             存放数据的缓冲区指针
 * @param  len              期望读取的数据长度
 * @retval uint16_t         实际读取的数据长度
 * @note 传入NULL指针可丢弃数据
 */
extern uint16_t Fifo_Get(fifo_t *fifo, uint8_t *data, uint16_t len);

/**
 * @brief 查看FIFO中的数据, 不改变FIFO的状态
 * @param  fifo             FIFO对象
 * @param  data             存放数据的缓冲区指针
 * @param  len              期望查看的数据长度
 * @retval uint16_t         实际查看的数据长度
 */
extern uint16_t Fifo_Peek(fifo_t *fifo, uint8_t *data, uint16_t len);

#endif  // __FIFO_H__

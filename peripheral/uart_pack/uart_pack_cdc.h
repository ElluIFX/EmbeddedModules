/**
 * @file uart_pack_cdc.h
 * @author Ellu (ellu.grif@gmail.com)
 *
 * THINK DIFFERENTLY
 */

#ifndef __UART_PACK_CDC_H__
#define __UART_PACK_CDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "lfifo.h"
#include "modules.h"

/**
 * @brief  USB CDC FIFO发送/接收初始化
 * @param  txBuf           发送缓冲区（NULL则尝试动态分配）
 * @param  txBufSize       发送缓冲区大小
 * @param  rxBuf           接收缓冲区（NULL则尝试动态分配）
 * @param  rxBufSize       接收缓冲区大小
 * @param  rxCallback      接收完成回调函数
 * @param  cbkInIRQ        回调函数是否在中断中执行
 * @retval lfifo_t         接收LFIFO句柄, NULL:失败
 */
lfifo_t *CDC_FifoTxRxInit(uint8_t *txBuf, size_t txBufSize, uint8_t *rxBuf,
                          size_t rxBufSize, void (*rxCallback)(lfifo_t *fifo),
                          uint8_t cbkInIRQ);

/**
 * @brief USB CDC 发送格式化字符串
 */
extern int CDC_Printf(char *fmt, ...);

/**
 * @brief USB CDC 发送数据
 * @param  buf              数据指针
 * @param  len              数据长度
 */
extern void CDC_Send(uint8_t *buf, size_t len);

/**
 * @brief USB是否已连接
 * @retval uint8_t          1:已连接 0:未连接
 */
extern uint8_t CDC_Connected(void);

/**
 * @brief USB CDC 阻塞等待连接
 * @param  timeout_ms       超时时间(ms)
 */
extern void CDC_WaitConnect(int timeout_ms);

/**
 * @brief USB CDC 等待发送完成
 */
extern void CDC_WaitTxFinish(void);

#ifdef __cplusplus
}
#endif

#endif  // __UART_PACK_CDC_H__

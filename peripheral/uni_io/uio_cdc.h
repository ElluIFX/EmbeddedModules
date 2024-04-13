/**
 * @file uio_cdc.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#ifndef __UIO_CDC_H__
#define __UIO_CDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "lfifo.h"
#include "uio_cfg.h"

#if UIO_CFG_ENABLE_CDC
#if UIO_CFG_CDC_USE_CUBEMX
#if !__has_include("usbd_cdc_if.h")
#undef UIO_CFG_ENABLE_CDC
#warning \
    "usbd_cdc_if.h not found, CDC support disabled, enable it in CubeMX first"
#endif
#endif
#endif

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
lfifo_t *cdc_fifo_init(uint8_t *txBuf, size_t txBufSize, uint8_t *rxBuf,
                       size_t rxBufSize, void (*rxCallback)(lfifo_t *fifo),
                       uint8_t cbkInIRQ);

/**
 * @brief USB CDC 发送格式化字符串
 */
extern int cdc_printf(char *fmt, ...);

/**
 * @brief USB CDC 发送数据
 * @param  buf              数据指针
 * @param  len              数据长度
 */
extern void cdc_write(uint8_t *buf, size_t len);

/**
 * @brief USB是否已连接
 * @retval uint8_t          1:已连接 0:未连接
 */
extern uint8_t cdc_is_connected(void);

/**
 * @brief USB CDC 阻塞等待连接
 * @param  timeout_ms       超时时间(ms)
 */
extern void cdc_wait_for_connect(int timeout_ms);

/**
 * @brief USB CDC 等待发送完成
 */
extern void cdc_flush(void);

#ifdef __cplusplus
}
#endif

#endif  // __UIO_CDC_H__

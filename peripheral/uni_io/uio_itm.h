/**
 * @file uio_itm.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#ifndef __UIO_ITM_H__
#define __UIO_ITM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

#if UIO_CFG_ENABLE_ITM

/**
 * @brief 向ITM(SWO)指定通道发送字符
 * @param  port             通道号(0~31)
 * @param  ch               字符
 */
extern void itm_send_char(uint8_t port, uint8_t ch);

/**
 * @brief 从ITM(SWO)接收字符
 * @retval 接收的字符
 */
extern int32_t itm_recv_char(void);

/**
 * @brief 向ITM(SWO)指定通道发送格式化字符串
 * @param  port             通道号(0~31)
 * @param  fmt              类似printf的格式化字符串
 * @retval 发送的字节数
 */
extern int itm_printf(uint8_t port, const char* fmt, ...);

/**
 * @brief 向ITM(SWO)指定通道发送数据
 * @param  port             通道号(0~31)
 * @param  data             数据指针
 * @param  len              数据长度
 * @retval 发送的字节数(0:通道未激活)
 */
extern int itm_write(uint8_t port, uint8_t* data, size_t len);

/**
 * @brief 从ITM(SWO)读取数据
 * @param  data             缓冲区指针
 * @param  len              缓冲区大小
 * @param  timeout_ms       无数据超时时间(ms)
 * @retval 读取的字节数
 */
extern size_t itm_read(uint8_t* data, size_t len, m_time_t timeout_ms);

#endif  // UIO_CFG_ENABLE_ITM

#ifdef __cplusplus
}
#endif

#endif  // __UIO_ITM_H__

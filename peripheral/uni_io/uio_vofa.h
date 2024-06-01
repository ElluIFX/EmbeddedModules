/**
 * @file uio_vofa.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#ifndef __UIO_VOFA_H__
#define __UIO_VOFA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

#if UIO_CFG_ENABLE_VOFA
/**
 * @brief 向VOFA缓冲区添加浮点数据
 */
extern void vofa_add(float value);

/**
 * @brief 向VOFA缓冲区添加浮点数组
 */
extern void vofa_add_array(float* value, uint8_t len);

/**
 * @brief 清空VOFA缓冲区
 */
extern void vofa_clear(void);

/**
 * @brief 向指定串口发送VOFA数据
 */
extern void vofa_send_uart(UART_HandleTypeDef* huart);

/**
 * @brief 向指定串口发送VOFA数据(如串口未就绪则丢弃)
 */
extern void vofa_send_uart_fast(UART_HandleTypeDef* huart);

#if UIO_CFG_ENABLE_CDC
/**
 * @brief 向CDC发送VOFA数据
 */
extern void vofa_send_cdc(void);
#endif  // UIO_CFG_ENABLE_CDC

#endif  // UIO_CFG_ENABLE_VOFA

#ifdef __cplusplus
}
#endif

#endif  // __UIO_VOFA_H__

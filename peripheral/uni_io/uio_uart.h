/**
 * @file uio_uart.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#ifndef __UIO_UART_H__
#define __UIO_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "uio_cfg.h"

#if UIO_CFG_ENABLE_UART

#include "lfbb.h"
#include "lfifo.h"
#include "usart.h"

/**
 * @brief 串口错误状态
 * @note 1:奇偶校验错误 2:帧错误 3:噪声错误 4:接收溢出
 */
extern __IO uint8_t uart_error_state;

// public functions
/**
 * @brief 向指定串口发送格式化字符串
 * @param  huart            目标串口
 * @param  fmt              类似printf的格式化字符串
 * @retval 发送的字节数
 */
extern int uart_printf(UART_HandleTypeDef* huart, const char* fmt, ...);

/**
 * @brief 向指定串口发送格式化字符串(阻塞, 不使用FIFO/DMA/中断)
 * @param  huart            目标串口
 * @param  fmt              类似printf的格式化字符串
 * @retval 发送的字节数
 */
extern int uart_printf_block(UART_HandleTypeDef* huart, const char* fmt, ...);

/**
 * @brief 等待串口发送完成
 */
extern void uart_flush(UART_HandleTypeDef* huart);

/**
 * @brief 向指定串口发送数据
 * @param  huart            目标串口
 * @param  data             数据指针
 * @param  len              数据长度
 */
extern int uart_write(UART_HandleTypeDef* huart, const uint8_t* data,
                      size_t len);

/**
 * @brief 串口发送数据，阻塞时不等待
 * @note  不支持FIFO发送，直接将数据写入外设
 */
extern int uart_write_fast(UART_HandleTypeDef* huart, uint8_t* data,
                           size_t len);

/**
 * @brief 串口中断FIFO接收初始化
 * @param  huart            目标串口
 * @param  buf              接收缓冲区(NULL则尝试动态分配)
 * @param  buf_size          缓冲区大小
 * @param  rxCallback       接收完成回调函数
 * @retval lfifo_t          LFIFO句柄, NULL:失败
 */
extern lfifo_t* uart_fifo_rx_init(UART_HandleTypeDef* huart, uint8_t* buf,
                                  size_t buf_size,
                                  void (*rxCallback)(lfifo_t* fifo));
/**
 * @brief 轮询以在主循环中响应串口接收完成回调
 * @note FIFO接收需要该函数, DMA接收在cbkInIRQ=0时也需要该函数
 * @note 若轮询频率小于接收频率, DMA回调请求会粘包
 */
extern void uart_check_callback(void);

/**
 * @brief 串口发送完成中断处理，在函数HAL_UART_TxCpltCallback中调用
 * @note FIFO发送需要该函数
 */
extern void uart_tx_process(UART_HandleTypeDef* huart);

/**
 * @brief 串口接收中断处理，在函数HAL_UART_RxCpltCallback中调用
 * @note FIFO接收需要该函数
 */
extern void uart_rx_process(UART_HandleTypeDef* huart);

/**
 * @brief 串口错误中断处理，在函数HAL_UART_ErrorCallback中调用
 */
extern void uart_error_process(UART_HandleTypeDef* huart);

#if UIO_CFG_UART_ENABLE_FIFO_TX
/**
 * @brief 初始化FIFO串口发送
 * @param  huart         目标串口
 * @param  buf           发送缓冲区, 若为NULL则尝试动态分配
 * @param  buf_size       缓冲区大小
 * @retval int           0:成功 -1:失败
 */
extern int uart_fifo_tx_init(UART_HandleTypeDef* huart, uint8_t* buf,
                             size_t buf_size);
extern void uart_fifo_tx_deinit(UART_HandleTypeDef* huart);
#endif

#if UIO_CFG_UART_ENABLE_DMA_RX

/**
 * @brief 串口DMA接收初始化
 * @param  huart            目标串口
 * @param  buf              接收缓冲区(NULL则尝试动态分配)
 * @param  buf_size          缓冲区大小
 * @param  rxCallback       接收完成回调函数(NULL则需要手动使用LFBB句柄获取数据)
 * @param  cbkInIRQ         回调函数是否在中断中执行(极不推荐)
 * @retval LFBB_Inst_Type*  LFBB句柄, NULL:失败
 */
extern LFBB_Inst_Type* uart_dma_rx_init(
    UART_HandleTypeDef* huart, uint8_t* buf, size_t buf_size,
    void (*rxCallback)(uint8_t* data, size_t len), uint8_t cbkInIRQ);

/**
 * @brief 串口DMA接收处理，在函数HAL_UARTEx_RxEventCallback中调用
 * @note DMA接收需要该函数
 */
extern void uart_dma_rx_process(UART_HandleTypeDef* huart, size_t Size);
#endif  // UIO_CFG_UART_ENABLE_DMA_RX

#endif  // UIO_CFG_ENABLE_UART

#ifdef __cplusplus
}
#endif
#endif  // __UART_H

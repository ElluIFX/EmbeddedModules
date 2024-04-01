/**
 * @file uartPack.h
 * @author Ellu (ellu.grif@gmail.com)
 *
 * THINK DIFFERENTLY
 */

#ifndef __UART_PACK_H__
#define __UART_PACK_H__

#ifdef __cplusplus
extern "C" {
#endif
// #include <stdio.h>
#include <string.h>

#include "modules.h"

#if __has_include("usart.h")
#include "usart.h"
#endif
#include "lfbb.h"
#include "lfifo.h"
#include "uart_pack_cdc.h"

/****************************** 串口设置 ******************************/

#if !KCONFIG_AVAILABLE  // 由Kconfig配置
// 组件设置
#define UART_CFG_ENABLE_DMA_RX 1      // 是否开启串口DMA接收功能
#define UART_CFG_ENABLE_FIFO_TX 1     // 是否开启串口FIFO发送功能
#define UART_CFG_DCACHE_COMPATIBLE 0  // (H7/F7) DCache兼容模式
#define UART_CFG_REWRITE_HANLDER 1  // 是否重写HAL库中的串口中断处理函数
// USB CDC设置
#define UART_CFG_ENABLE_CDC 0      // 是否开启CDC虚拟串口支持
#define UART_CFG_ENABLE_ITM 0      // 是否开启ITM(SWO)支持
#define UART_CFG_CDC_USE_CUBEMX 1  // 是否使用CUBEMX生成的CDC代码
#define UART_CFG_CDC_USE_CHERRY 0  // 是否使用CherryUSB的CDC代码
// 串口收发设置
#define UART_CFG_TX_USE_DMA 1  // 对于支持DMA的串口是否使用DMA发送
#define UART_CFG_TX_USE_IT 1  // 对不支持DMA的串口是否使用中断发送
#define UART_CFG_TX_TIMEOUT 5  // 串口发送等待超时时间(ms)/0阻塞/<0放弃发送
#define UART_CFG_CDC_TIMEOUT 5  // CDC发送等待超时时间(ms)/<=0放弃发送
#define UART_CFG_FIFO_TIMEOUT 5  // FIFO发送等待超时时间(ms)/0阻塞/<0放弃发送
// printf重定向设置
#define UART_CFG_PRINTF_BLOCK 0           // 是否屏蔽所有printf
#define UART_CFG_PRINTF_REDIRECT 1        // 是否重定向printf
#define UART_CFG_PRINTF_REDIRECT_PUTX 1   // 是否重定向putchar/puts
#define UART_CFG_PRINTF_UART_PORT huart1  // printf重定向到串口
#define UART_CFG_PRINTF_USE_RTT 0         // printf重定向到RTT
#define UART_CFG_PRINTF_USE_CDC 0         // printf重定向到CDC
#define UART_CFG_PRINTF_USE_ITM 0         // printf重定向到ITM
// VOFA+
#define VOFA_CFG_ENABLE 1        // 是否开启VOFA相关函数
#define VOFA_CFG_BUFFER_SIZE 32  // VOFA缓冲区大小

#endif  // !KCONFIG_AVAILABLE

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
extern int uart_printf(UART_HandleTypeDef *huart, const char *fmt, ...);

/**
 * @brief 向指定串口发送格式化字符串(阻塞, 不使用FIFO/DMA/中断)
 * @param  huart            目标串口
 * @param  fmt              类似printf的格式化字符串
 * @retval 发送的字节数
 */
extern int uart_printf_block(UART_HandleTypeDef *huart, const char *fmt, ...);

extern uint8_t disable_uart_printf;  // 禁止printf输出

/**
 * @brief 等待串口发送完成
 */
extern void uart_flush(UART_HandleTypeDef *huart);

/**
 * @brief 向指定串口发送数据
 * @param  huart            目标串口
 * @param  data             数据指针
 * @param  len              数据长度
 */
extern int uart_write(UART_HandleTypeDef *huart, const uint8_t *data,
                      size_t len);

/**
 * @brief 串口发送数据，阻塞时不等待
 * @note  不支持FIFO发送，直接将数据写入外设
 */
extern int uart_write_fast(UART_HandleTypeDef *huart, uint8_t *data,
                           size_t len);

/**
 * @brief 串口中断FIFO接收初始化
 * @param  huart            目标串口
 * @param  buf              接收缓冲区(NULL则尝试动态分配)
 * @param  buf_size          缓冲区大小
 * @param  rxCallback       接收完成回调函数
 * @retval lfifo_t          LFIFO句柄, NULL:失败
 */
extern lfifo_t *uart_fifo_rx_init(UART_HandleTypeDef *huart, uint8_t *buf,
                                  size_t buf_size,
                                  void (*rxCallback)(lfifo_t *fifo));
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
extern void uart_tx_process(UART_HandleTypeDef *huart);

/**
 * @brief 串口接收中断处理，在函数HAL_UART_RxCpltCallback中调用
 * @note FIFO接收需要该函数
 */
extern void uart_rx_process(UART_HandleTypeDef *huart);

/**
 * @brief 串口错误中断处理，在函数HAL_UART_ErrorCallback中调用
 */
extern void uart_error_process(UART_HandleTypeDef *huart);

#if UART_CFG_ENABLE_ITM

/**
 * @brief 向ITM(SWO)通道0发送格式化字符串
 * @param  fmt              类似printf的格式化字符串
 * @retval 发送的字节数
 */
extern int ITM_printf(const char *fmt, ...);

/**
 * @brief 向ITM(SWO)指定通道发送数据
 * @param  port             通道号(0~31)
 * @param  data             数据指针
 * @param  len              数据长度
 * @retval 发送的字节数(0:通道未激活)
 */
extern int ITM_write(uint8_t port, uint8_t *data, size_t len);

/**
 * @brief 从ITM(SWO)读取数据
 * @param  data             缓冲区指针
 * @param  len              缓冲区大小
 * @param  timeout_ms       超时时间(ms)
 * @retval 读取的字节数
 */
extern size_t ITM_read(uint8_t *data, size_t len, m_time_t timeout_ms);

#endif  // UART_CFG_ENABLE_ITM

#if UART_CFG_ENABLE_FIFO_TX
/**
 * @brief 初始化FIFO串口发送
 * @param  huart         目标串口
 * @param  buf           发送缓冲区, 若为NULL则尝试动态分配
 * @param  buf_size       缓冲区大小
 * @retval int           0:成功 -1:失败
 */
extern int uart_fifo_tx_init(UART_HandleTypeDef *huart, uint8_t *buf,
                             size_t buf_size);
extern void uart_fifo_tx_deinit(UART_HandleTypeDef *huart);
#endif

#if UART_CFG_ENABLE_DMA_RX

/**
 * @brief 串口DMA接收初始化
 * @param  huart            目标串口
 * @param  buf              接收缓冲区(NULL则尝试动态分配)
 * @param  buf_size          缓冲区大小
 * @param  rxCallback       接收完成回调函数(NULL则需要手动使用LFBB句柄获取数据)
 * @param  cbkInIRQ         回调函数是否在中断中执行(极不推荐)
 * @retval LFBB_Inst_Type*  LFBB句柄, NULL:失败
 */
extern LFBB_Inst_Type *uart_dma_rx_init(
    UART_HandleTypeDef *huart, uint8_t *buf, size_t buf_size,
    void (*rxCallback)(uint8_t *data, size_t len), uint8_t cbkInIRQ);

/**
 * @brief 串口DMA接收处理，在函数HAL_UARTEx_RxEventCallback中调用
 * @note DMA接收需要该函数
 */
extern void uart_dma_rx_process(UART_HandleTypeDef *huart, size_t Size);
#endif  // UART_CFG_ENABLE_DMA_RX

#if VOFA_CFG_ENABLE
/**
 * @brief 向VOFA缓冲区添加浮点数据
 */
extern void vofa_add(float value);

/**
 * @brief 向VOFA缓冲区添加浮点数组
 */
extern void vofa_add_array(float *value, uint8_t len);

/**
 * @brief 清空VOFA缓冲区
 */
extern void vofa_clear(void);

/**
 * @brief 向指定串口发送VOFA数据
 */
extern void vofa_send(UART_HandleTypeDef *huart);

/**
 * @brief 向指定串口发送VOFA数据(如串口未就绪则丢弃)
 */
extern void vofa_send_fast(UART_HandleTypeDef *huart);

#if UART_CFG_ENABLE_CDC
/**
 * @brief 向CDC发送VOFA数据
 */
extern void vofa_send_cdc(void);
#endif  // UART_CFG_ENABLE_CDC

#endif  // VOFA_CFG_ENABLE

// 重定向printf
#if UART_CFG_PRINTF_BLOCK
#define printf(...) ((void)0)
#define printf_flush() ((void)0)
#else  // UART_CFG_PRINTF_BLOCK
#if UART_CFG_PRINTF_REDIRECT
#undef printf
#if UART_CFG_PRINTF_USE_RTT
#include "SEGGER_RTT.h"
#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define printf_flush() ((void)0)
#define printf_block(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define println(fmt, ...) SEGGER_RTT_printf(0, fmt "\r\n", ##__VA_ARGS__)
#elif UART_CFG_PRINTF_USE_CDC && UART_CFG_ENABLE_CDC
#define printf(...) cdc_printf(__VA_ARGS__)
#define printf_flush() cdc_printf_flush()
#define printf_block(...) cdc_printf(__VA_ARGS__)
#define println(fmt, ...) cdc_printf(fmt "\r\n", ##__VA_ARGS__)
#elif UART_CFG_PRINTF_USE_ITM
#define printf(...) ITM_printf(__VA_ARGS__)
#define printf_flush() ((void)0)
#define printf_block(...) ITM_printf(__VA_ARGS__)
#define println(fmt, ...) ITM_printf(fmt "\r\n", ##__VA_ARGS__)
#else
#define printf(...) uart_printf(&UART_CFG_PRINTF_UART_PORT, __VA_ARGS__)
#define printf_flush() uart_flush(&UART_CFG_PRINTF_UART_PORT)
#define printf_block(...) \
  uart_printf_block(&UART_CFG_PRINTF_UART_PORT, __VA_ARGS__)
#define println(fmt, ...) printf(fmt "\r\n", ##__VA_ARGS__)
#endif  // UART_CFG_PRINTF_USE_*
#endif  // UART_CFG_PRINTF_REDIRECT
#endif  // UART_CFG_PRINTF_BLOCK

#if UART_CFG_PRINTF_REDIRECT_PUTX
static inline int __putchar(int ch) {
#if UART_CFG_PRINTF_USE_RTT
  SEGGER_RTT_Write(0, &ch, 1);
#elif UART_CFG_PRINTF_USE_CDC && UART_CFG_ENABLE_CDC
  cdc_write(&ch, 1);
#elif UART_CFG_PRINTF_USE_ITM
  ITM_SendChar(ch);
#else
  uart_write(&UART_CFG_PRINTF_UART_PORT, (uint8_t *)&ch, 1);
#endif  // UART_CFG_PRINTF_USE_*
  return ch;
}
static inline int __puts(const char *s) {
#if UART_CFG_PRINTF_USE_RTT
  SEGGER_RTT_Write(0, s, strlen(s));
#elif UART_CFG_PRINTF_USE_CDC && UART_CFG_ENABLE_CDC
  cdc_write((uint8_t *)s, strlen(s));
#elif UART_CFG_PRINTF_USE_ITM
  while (*s) {
    ITM_SendChar(*s++);
  }
#else
  uart_write(&UART_CFG_PRINTF_UART_PORT, (uint8_t *)s, strlen(s));
#endif  // UART_CFG_PRINTF_USE_*
  return 0;
}
#undef putchar
#undef puts
#define putchar __putchar
#define puts __puts
#endif  // UART_CFG_PRINTF_REDIRECT_PUTX

#ifdef __cplusplus
}
#endif
#endif  // __UART_H

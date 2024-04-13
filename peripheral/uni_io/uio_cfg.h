/**
 * @file uio_cfg.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#ifndef __UIO_CFG_H__
#define __UIO_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

#if !KCONFIG_AVAILABLE  // 由Kconfig配置
// 串口收发设置
#define UIO_CFG_ENABLE_UART 1             // 是否开启串口支持
#define UIO_CFG_UART_ENABLE_DMA_RX 1      // 是否开启串口DMA接收功能
#define UIO_CFG_UART_ENABLE_FIFO_TX 1     // 是否开启串口FIFO发送功能
#define UIO_CFG_UART_DCACHE_COMPATIBLE 0  // (H7/F7) DCache兼容模式
#define UIO_CFG_UART_REWRITE_HANLDER 1  // 是否重写HAL库中的串口中断处理函数
#define UIO_CFG_UART_TX_USE_DMA 1  // 对于支持DMA的串口是否使用DMA发送
#define UIO_CFG_UART_TX_USE_IT 1  // 对不支持DMA的串口是否使用中断发送
#define UIO_CFG_UART_TX_TIMEOUT 5  // 串口发送等待超时时间(ms)/0阻塞/<0放弃
#define UIO_CFG_UART_FIFO_TIMEOUT 5  // FIFO发送等待超时时间(ms)/0阻塞/<0放弃
// USB CDC设置
#define UIO_CFG_ENABLE_CDC 0      // 是否开启CDC虚拟串口支持
#define UIO_CFG_CDC_USE_CUBEMX 1  // 是否使用CUBEMX生成的CDC代码
#define UIO_CFG_CDC_USE_CHERRY 0  // 是否使用CherryUSB的CDC代码
#define UIO_CFG_CDC_TIMEOUT 5  // CDC发送等待超时时间(ms)/<=0放弃发送
// ITM(SWO)设置
#define UIO_CFG_ENABLE_ITM 0  // 是否开启ITM(SWO)支持
// printf重定向设置
#define UIO_CFG_PRINTF_REDIRECT 1        // 是否重定向printf
#define UIO_CFG_PRINTF_REDIRECT_PUTX 1   // 是否重定向putchar/puts/getchar
#define UIO_CFG_PRINTF_DISABLE 0         // 是否屏蔽所有printf
#define UIO_CFG_PRINTF_UART_PORT huart1  // printf重定向到串口
#define UIO_CFG_PRINTF_USE_RTT 0         // printf重定向到RTT
#define UIO_CFG_PRINTF_USE_CDC 0         // printf重定向到CDC
#define UIO_CFG_PRINTF_USE_ITM 0         // printf重定向到ITM
#define UIO_CFG_PRINTF_ENDL "\r\n"       // printf换行符
// VOFA+
#define UIO_CFG_ENABLE_VOFA 1        // 是否开启VOFA相关函数
#define UIO_CFG_VOFA_BUFFER_SIZE 32  // VOFA缓冲区大小

#ifdef __cplusplus
}
#endif

#endif  // !KCONFIG_AVAILABLE

#endif  // __UIO_CFG_H__

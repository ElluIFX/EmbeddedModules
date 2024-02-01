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
#include <stdio.h>

#include "modules.h"

#if __has_include("usart.h")
#include "usart.h"
#endif

#include "lfbb.h"
#include "lfifo.h"

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
extern int printft(UART_HandleTypeDef *huart, const char *fmt, ...),
    printft_block(UART_HandleTypeDef *huart, const char *fmt, ...);
extern uint8_t disable_printft;  // 禁止printf输出

/**
 * @brief 等待串口发送完成
 */
extern void printft_flush(UART_HandleTypeDef *huart);

/**
 * @brief 向指定串口发送数据
 * @param  huart            目标串口
 * @param  data             数据指针
 * @param  len              数据长度
 */
extern int Uart_Send(UART_HandleTypeDef *huart, const uint8_t *data,
                     uint16_t len);

/**
 * @brief 串口发送数据，阻塞时不等待
 * @note  不支持FIFO发送，直接将数据写入外设
 */
extern int Uart_SendFast(UART_HandleTypeDef *huart, uint8_t *data,
                         uint16_t len);

// 适配标准C输入输出函数
extern void Uart_Putchar(UART_HandleTypeDef *huart, uint8_t data);
extern void Uart_Puts(UART_HandleTypeDef *huart, const char *str);
extern int Uart_Getchar(UART_HandleTypeDef *huart);
extern char *Uart_Gets(UART_HandleTypeDef *huart, char *str);

/**
 * @brief 串口中断FIFO接收初始化
 * @param  huart            目标串口
 * @param  buf              接收缓冲区(NULL则尝试动态分配)
 * @param  bufSize          缓冲区大小
 * @param  rxCallback       接收完成回调函数
 * @retval lfifo_t          LFIFO句柄, NULL:失败
 */
extern lfifo_t *Uart_FifoRxInit(UART_HandleTypeDef *huart, uint8_t *buf,
                                uint16_t bufSize,
                                void (*rxCallback)(lfifo_t *fifo));
/**
 * @brief 轮询以在主循环中响应串口接收完成回调
 * @note FIFO接收需要该函数, DMA接收在cbkInIRQ=0时也需要该函数
 * @note 若轮询频率小于接收频率, DMA回调请求会粘包
 */
extern void Uart_CallbackCheck(void);

/**
 * @brief 串口发送完成中断处理，在函数HAL_UART_TxCpltCallback中调用
 * @note FIFO发送需要该函数
 */
extern void Uart_TxProcess(UART_HandleTypeDef *huart);

/**
 * @brief 串口接收中断处理，在函数HAL_UART_RxCpltCallback中调用
 * @note FIFO接收需要该函数
 */
extern void Uart_RxProcess(UART_HandleTypeDef *huart);

/**
 * @brief 串口错误中断处理，在函数HAL_UART_ErrorCallback中调用
 */
extern void Uart_ErrorProcess(UART_HandleTypeDef *huart);

#if _UART_ENABLE_FIFO_TX
/**
 * @brief 初始化FIFO串口发送
 * @param  huart         目标串口
 * @param  buf           发送缓冲区, 若为NULL则尝试动态分配
 * @param  bufSize       缓冲区大小
 * @retval int           0:成功 -1:失败
 */
extern int Uart_FifoTxInit(UART_HandleTypeDef *huart, uint8_t *buf,
                           uint16_t bufSize);
extern void Uart_FifoTxDeInit(UART_HandleTypeDef *huart);
#endif

#if _UART_ENABLE_DMA_RX

/**
 * @brief 串口DMA接收初始化
 * @param  huart            目标串口
 * @param  buf              接收缓冲区(NULL则尝试动态分配)
 * @param  bufSize          缓冲区大小
 * @param  rxCallback       接收完成回调函数(NULL则需要手动使用LFBB句柄获取数据)
 * @param  cbkInIRQ         回调函数是否在中断中执行(极不推荐)
 * @retval LFBB_Inst_Type*  LFBB句柄, NULL:失败
 */
extern LFBB_Inst_Type *Uart_DmaRxInit(
    UART_HandleTypeDef *huart, uint8_t *buf, uint16_t bufSize,
    void (*rxCallback)(uint8_t *data, uint16_t len), uint8_t cbkInIRQ);

/**
 * @brief 串口DMA接收处理，在函数HAL_UARTEx_RxEventCallback中调用
 * @note DMA接收需要该函数
 */
extern void Uart_DmaRxProcess(UART_HandleTypeDef *huart, uint16_t Size);
#endif  // _UART_ENABLE_DMA_RX

#if _UART_ENABLE_CDC
#include "usbd_cdc_if.h"

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
lfifo_t *CDC_FifoTxRxInit(uint8_t *txBuf, uint16_t txBufSize, uint8_t *rxBuf,
                          uint16_t rxBufSize, void (*rxCallback)(lfifo_t *fifo),
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
extern void CDC_Send(uint8_t *buf, uint16_t len);

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

#endif  // _UART_ENABLE_CDC

#if _VOFA_ENABLE
/**
 * @brief 向VOFA缓冲区添加浮点数据
 */
extern void Vofa_Add(float value);

/**
 * @brief 向VOFA缓冲区添加浮点数组
 */
extern void Vofa_AddSeq(float *value, uint8_t len);

/**
 * @brief 清空VOFA缓冲区
 */
extern void Vofa_Clear(void);

/**
 * @brief 向指定串口发送VOFA数据
 */
extern void Vofa_Send(UART_HandleTypeDef *huart);

/**
 * @brief 向指定串口发送VOFA数据(如串口未就绪则丢弃)
 */
extern void Vofa_SendFast(UART_HandleTypeDef *huart);
#if _UART_ENABLE_CDC
/**
 * @brief 向CDC发送VOFA数据
 */
extern void Vofa_SendCDC(void);
#endif  // _UART_ENABLE_CDC
#endif  // _VOFA_ENABLE

// 重定向printf
#if _PRINTF_BLOCK
#define printf(...) ((void)0)
#define printf_flush() ((void)0)
#else  // _PRINTF_BLOCK
#if _PRINTF_REDIRECT
#if _PRINTF_USE_RTT
#include "SEGGER_RTT.h"
#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define printf_flush() ((void)0)
#else  // _PRINTF_USE_RTT
#if _PRINTF_USE_CDC && _UART_ENABLE_CDC
#define printf(...) printfcdc(__VA_ARGS__)
#define printf_flush() printfcdc_flush()
#undef putchar
#define putchar(c) CDC_Putchar(c)
#undef puts
#define puts(s) CDC_Puts(s)
#else
#undef printf
#define printf(...) printft(&_PRINTF_UART_PORT, __VA_ARGS__)
#define printf_flush() printft_flush(&_PRINTF_UART_PORT)
#define printf_block(...) printft_block(&_PRINTF_UART_PORT, __VA_ARGS__)
#define println(fmt, ...) printf(fmt "\r\n", ##__VA_ARGS__)
#if _PRINTF_REDIRECT_FUNC
#undef putchar
#undef getchar
#undef puts
#undef gets
#define putchar(c) Uart_Putchar(&_PRINTF_UART_PORT, c)
#define getchar() Uart_Getchar(&_PRINTF_UART_PORT)
#define puts(s) Uart_Puts(&_PRINTF_UART_PORT, s)
#define gets(s) Uart_Gets(&_PRINTF_UART_PORT, s)
#endif  // _PRINTF_REDIRECT_FUNC
#endif  // _PRINTF_USE_CDC && _UART_ENABLE_CDC
#endif  // _PRINTF_USE_RTT
#endif  // _PRINTF_REDIRECT
#endif  // _PRINTF_BLOCK

#ifdef __cplusplus
}
#endif
#endif  // __UART_H

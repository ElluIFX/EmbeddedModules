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
extern int printft(UART_HandleTypeDef *huart, const char *fmt, ...);
extern uint8_t disable_printft;

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

#include "lfifo.h"

/**
 * @brief 串口中断FIFO接收初始化
 * @param  huart            目标串口
 * @param  rxCallback       接收完成回调函数
 * @param  buf              接收缓冲区(NULL则尝试动态分配)
 * @param  bufSize          缓冲区大小
 * @retval lfifo_t          LFIFO句柄, NULL:失败
 */
extern lfifo_t *Uart_FifoRxInit(UART_HandleTypeDef *huart,
                                void (*rxCallback)(lfifo_t *fifo), uint8_t *buf,
                                uint16_t bufSize);
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
#include "lfbb.h"

/**
 * @brief 串口DMA接收初始化
 * @param  huart            目标串口
 * @param  rxCallback       接收完成回调函数
 * @param  cbkInIRQ         回调函数是否在中断中执行
 * @param  buf              接收缓冲区(NULL则尝试动态分配)
 * @param  bufSize          缓冲区大小
 * @retval LFBB_Inst_Type*  LFBB句柄, NULL:失败
 */
extern LFBB_Inst_Type *Uart_DmaRxInit(
    UART_HandleTypeDef *huart, void (*rxCallback)(uint8_t *data, uint16_t len),
    uint8_t cbkInIRQ, uint8_t *buf, uint16_t bufSize);

/**
 * @brief 串口DMA接收处理，在函数HAL_UARTEx_RxEventCallback中调用
 * @note DMA接收需要该函数
 */
extern void Uart_DmaRxProcess(UART_HandleTypeDef *huart, uint16_t Size);
#endif  // _UART_ENABLE_DMA_RX

#if _UART_ENABLE_CDC
#include "usbd_cdc_if.h"
typedef struct {                         // CDC型UART控制结构体
  uint8_t buffer[APP_RX_DATA_SIZE];      // 接收保存缓冲区
  __IO uint8_t finished;                 // 接收完成标志位
  __IO uint16_t len;                     // 接收保存区计数器
  void (*rxCallback)(char *, uint16_t);  // 接收完成回调函数
  uint8_t cbkInIRQ;                      // 回调函数是否在中断中执行
} usb_cdc_ctrl_t;

// USB CDC 串口接收结构体
extern usb_cdc_ctrl_t usb_cdc;

/**
 * @brief USB CDC 发送格式化字符串
 */
extern int printfcdc(char *fmt, ...);

/**
 * @brief USB CDC 等待发送完成
 */
extern void printfcdc_flush(void);

/**
 * @brief USB CDC 发送数据
 * @param  buf              数据指针
 * @param  len              数据长度
 */
extern int CDC_Send(uint8_t *buf, uint16_t len);

/**
 * @brief USB是否已连接
 * @retval uint8_t          1:已连接 0:未连接
 */
extern uint8_t USB_Connected(void);

/**
 * @brief USB CDC 阻塞等待连接
 * @param  timeout_ms       超时时间(ms)
 */
extern void CDC_WaitConnect(int timeout_ms);

/**
 * @brief 注册USB CDC接收回调函数
 * @param callback 回调函数
 * @param cbkInIRQ 回调函数是否在中断中执行
 */
extern void CDC_RegisterCallback(void (*callback)(char *buf, uint16_t len),
                                 uint8_t cbkInIRQ);

// 适配标准C输入输出函数
extern void CDC_Putchar(uint8_t data);
extern void CDC_Puts(char *data);

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
#undef println
#define println(fmt, ...) printf(fmt "\r\n", ##__VA_ARGS__)
#if _PRINTF_REDIRECT_FUNC
#undef putchar
#define putchar(c) Uart_Putchar(&_PRINTF_UART_PORT, c)
#undef getchar
#define getchar() Uart_Getchar(&_PRINTF_UART_PORT)
#undef puts
#define puts(s) Uart_Puts(&_PRINTF_UART_PORT, s)
#undef gets
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

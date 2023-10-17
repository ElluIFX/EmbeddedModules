/**
 * @file uartPack.h
 * @author Ellu (ellu.grif@gmail.com)
 *
 * THINK DIFFERENTLY
 */

#ifndef __UART_PACK_H__
#define __UART_PACK_H__

#include "macro.h"
#include "modules.h"
#include "stdio.h"
#include "usart.h"

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
// typedef
typedef struct {                           // 中断串口接收控制结构体
  uint8_t rxBuf[_UART_RECV_BUFFER_SIZE];   // 接收缓冲区
  uint8_t buffer[_UART_RECV_BUFFER_SIZE];  // 接收保存缓冲区
  __IO uint8_t finished;                   // 接收完成标志位
  __IO uint16_t len;                       // 接收保存区长度
  uint16_t rxIdx;                          // 接收缓冲区索引
  m_time_t rxTime;                         // 接收超时计时器
  m_time_t rxTimeout;                      // 接收超时时间
  UART_HandleTypeDef *huart;               // 串口句柄
  void (*rxCallback)(char *, uint16_t);    // 接收完成回调函数
  uint8_t cbkInIRQ;  // 回调函数是否在中断中执行
} uart_it_rx_t;

#if _UART_ENABLE_DMA
typedef struct {  // DMA串口接收控制结构体
  uint8_t rxBuf[_UART_RECV_BUFFER_SIZE] __ALIGNED(32);   // 接收缓冲区
  uint8_t buffer[_UART_RECV_BUFFER_SIZE] __ALIGNED(32);  // 接收保存缓冲区
  __IO uint8_t finished;                 // 接收完成标志位
  __IO uint16_t len;                     // 接收保存区长度
  UART_HandleTypeDef *huart;             // 串口句柄
  void (*rxCallback)(char *, uint16_t);  // 接收完成回调函数
  uint8_t cbkInIRQ;                      // 回调函数是否在中断中执行
} uart_dma_rx_t;
#endif

#if _UART_ENABLE_TX_FIFO
typedef struct {                         // FIFO串口发送控制结构体
  uint8_t buffer[_UART_TX_FIFO_SIZE];  // 发送缓冲区
  uint16_t wr;                      // 写索引
  uint16_t rd;                      // 读索引
  uint16_t rd_temp;                 // 读索引临时变量
  UART_HandleTypeDef *huart;             // 串口句柄
} uart_fifo_tx_t;
#endif

#if _UART_ENABLE_CDC
#include "usbd_cdc_if.h"
typedef struct {                         // CDC型UART控制结构体
  uint8_t buffer[APP_RX_DATA_SIZE];      // 接收保存缓冲区
  __IO uint8_t finished;                 // 接收完成标志位
  __IO uint16_t len;                     // 接收保存区计数器
  void (*rxCallback)(char *, uint16_t);  // 接收完成回调函数
  uint8_t cbkInIRQ;                      // 回调函数是否在中断中执行
} usb_cdc_ctrl_t;

// USB CDC 串口接收
extern usb_cdc_ctrl_t usb_cdc;
#endif

#define RX_DONE(uart_t) uart_t.finished
#define RX_DATA(uart_t) ((char *)uart_t.buffer)
#define RX_LEN(uart_t) uart_t.len
#define RX_CLEAR(uart_t) (uart_t.finished = 0)

/**
 * @brief 串口错误状态
 * @note 1:奇偶校验错误 2:帧错误 3:噪声错误 4:接收溢出
 */
extern uint8_t uart_error_state;

// public functions
extern int printft(UART_HandleTypeDef *huart, char *fmt, ...);
extern void printft_flush(UART_HandleTypeDef *huart);
extern void print_hex(const char *text, uint8_t *data, uint16_t len);
extern int Uart_Send(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len);
extern int Uart_Send_Buffered(UART_HandleTypeDef *huart, uint8_t *data,
                              uint16_t len);
extern void Uart_Putchar(UART_HandleTypeDef *huart, uint8_t data);
extern void Uart_Puts(UART_HandleTypeDef *huart, char *str);
extern int Uart_Getchar(UART_HandleTypeDef *huart);
extern char *Uart_Gets(UART_HandleTypeDef *huart, char *str);
extern int Uart_SendFast(UART_HandleTypeDef *huart, uint8_t *data,
                         uint16_t len);
extern void Uart_IT_Rx_Init(uart_it_rx_t *ctrl, UART_HandleTypeDef *huart,
                      m_time_t rxTimeout,
                      void (*rxCallback)(char *data, uint16_t len),
                      uint8_t cbkInIRQ);
extern void Uart_IT_Timeout_Check(void);
extern void Uart_Callback_Check(void);

extern void Uart_Tx_Process(UART_HandleTypeDef *huart);
extern void Uart_Rx_Process(UART_HandleTypeDef *huart);
extern void Uart_Error_Process(UART_HandleTypeDef *huart);

#if _UART_ENABLE_TX_FIFO
extern void Uart_FIFO_Tx_Init(uart_fifo_tx_t *ctrl, UART_HandleTypeDef *huart);
#endif

#if _UART_ENABLE_DMA
extern void Uart_DMA_Rx_Init(uart_dma_rx_t *ctrl, UART_HandleTypeDef *huart,
                          void (*rxCallback)(char *data, uint16_t len),
                          uint8_t cbkInIRQ);
extern void Uart_DMA_Rx_Process(UART_HandleTypeDef *huart, uint16_t Size);
#endif  // _UART_ENABLE_DMA

#if _UART_ENABLE_CDC
extern int printfcdc(char *fmt, ...);
extern void printfcdc_flush(void);
extern int CDC_Send(uint8_t *buf, uint16_t len);
extern void CDC_Wait_Connect(int timeout_ms);
extern void CDC_Register_Callback(void (*callback)(char *buf, uint16_t len),
                                  uint8_t cbkInIRQ);
extern void CDC_Putchar(uint8_t data);
extern void CDC_Puts(char *data);
extern uint8_t USB_Connected(void);
#endif  // _UART_ENABLE_CDC

#if _VOFA_ENABLE
extern void Vofa_Add(float value);
extern void Vofa_AddSeq(float *value, uint8_t len);
extern void Vofa_Clear(void);
extern void Vofa_Send(UART_HandleTypeDef *huart);
extern void Vofa_SendFast(UART_HandleTypeDef *huart);
extern void Vofa_SendCDC(void);
#endif  // _VOFA_ENABLE

#endif  // __UART_H

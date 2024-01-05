#include "uart_pack.h"

#include "lwprintf.h"
#include "macro.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#if 1
static void uart_memcpy(void *dst, const void *src, uint16_t len) {
  uint8_t *dst8 = (uint8_t *)dst;
  uint8_t *src8 = (uint8_t *)src;
  while (len--) {
    *dst8++ = *src8++;
  }
}
#else
#define uart_memcpy memcpy
#endif

#if _UART_TX_USE_DMA
#define _UART_NOT_READY                     \
  (huart->gState != HAL_UART_STATE_READY || \
   (huart->hdmatx && huart->hdmatx->State != HAL_DMA_STATE_READY))
#else
#define _UART_NOT_READY huart->gState != HAL_UART_STATE_READY
#endif

static inline void _send_func(UART_HandleTypeDef *huart, uint8_t *data,
                              uint16_t len) {
#if _UART_TX_USE_DMA & _UART_ENABLE_DMA
  if (huart->hdmatx) {
#if _UART_DCACHE_COMPATIBLE
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)data, len);
#endif
    HAL_UART_Transmit_DMA(huart, data, len);
  } else
#endif
#if _UART_TX_USE_IT
    HAL_UART_Transmit_IT(huart, data, len);
#else
  HAL_UART_Transmit(huart, data, len, 0xFFFF);
#endif
}

#if _UART_ENABLE_TX_FIFO
typedef struct {              // FIFO串口发送控制结构体
  uint8_t *buffer;            // 发送缓冲区
  uint16_t size;              // 缓冲区大小
  uint16_t wr;                // 写索引
  uint16_t rd;                // 占用读索引
  uint16_t rd_send;           // 发送读索引
  UART_HandleTypeDef *huart;  // 串口句柄
  void *next;                 // 下一个控制结构体
} uart_fifo_tx_t;

static uart_fifo_tx_t *fifo_tx_entry = NULL;

static int fifo_lwprintf_fn(int ch, lwprintf_t *lwobj);
void Uart_FifoTxInit(UART_HandleTypeDef *huart, uint8_t *buffer,
                     uint16_t bufSize) {
  if (!bufSize) return;
  uart_fifo_tx_t *fifo = NULL;
  if (!fifo_tx_entry) {
    fifo_tx_entry = m_alloc(sizeof(uart_fifo_tx_t));
    fifo = fifo_tx_entry;
  } else {
    fifo = fifo_tx_entry;
    while (fifo->next) fifo = fifo->next;
    fifo->next = m_alloc(sizeof(uart_fifo_tx_t));
    fifo = fifo->next;
  }
  if (!fifo) return;
  if (buffer)
    fifo->buffer = buffer;
  else
    fifo->buffer = m_alloc(bufSize);
  if (!fifo->buffer) return;
  fifo->huart = huart;
  fifo->size = bufSize;
  fifo->wr = 0;
  fifo->rd = 0;
  fifo->rd_send = 0;
  fifo->next = NULL;
}

static uart_fifo_tx_t *is_fifo_tx(UART_HandleTypeDef *huart) {
  uart_fifo_tx_t *fifo = fifo_tx_entry;
  while (fifo) {
    if (fifo->huart == huart) return fifo;
    fifo = fifo->next;
  }
  return NULL;
}

#define FIFO_TX_DATA_LENGTH(FIFO)             \
  (FIFO->wr >= FIFO->rd ? FIFO->wr - FIFO->rd \
                        : FIFO->size + FIFO->wr - FIFO->rd)
#define FIFO_TX_FREE_SPACE(FIFO) (FIFO->size - FIFO_TX_DATA_LENGTH(FIFO) - 1)

static void fifo_exchange(uart_fifo_tx_t *fifo, uint8_t force) {
  UART_HandleTypeDef *huart = fifo->huart;
  if (!force && _UART_NOT_READY) return;  // 串口正在发送
  if (fifo->rd != fifo->rd_send) fifo->rd = fifo->rd_send;
  if (fifo->wr == fifo->rd) return;  // FIFO为空
  if (fifo->wr > fifo->rd) {         // 无需循环
    _send_func(huart, fifo->buffer + fifo->rd, fifo->wr - fifo->rd);
    fifo->rd_send = fifo->wr;
  } else {  // 先发尾部
    _send_func(huart, fifo->buffer + fifo->rd, fifo->size - fifo->rd);
    fifo->rd_send = 0;
  }
}

static inline int wait_fifo(uart_fifo_tx_t *fifo, uint16_t len) {
#if _UART_FIFO_TIMEOUT < 0
  return -1;
#else
#if _UART_FIFO_TIMEOUT > 0
  m_time_t _start_time = m_time_ms();
#endif  // _UART_FIFO_TIMEOUT
  while (FIFO_TX_FREE_SPACE(fifo) < len) {
    fifo_exchange(fifo, 0);
#if _UART_FIFO_TIMEOUT > 0
    if (m_time_ms() - _start_time > _UART_FIFO_TIMEOUT) return -1;
#endif  // _UART_FIFO_TIMEOUT
  }
  return 0;
#endif  // _UART_FIFO_TIMEOUT
}

static int fifo_send(uart_fifo_tx_t *fifo, uint8_t *data, uint16_t len) {
  if (len > fifo->size) return -1;
  if (FIFO_TX_FREE_SPACE(fifo) < len) {  // FIFO空间不足
    if (wait_fifo(fifo, len) < 0) return -1;
  }
  if (fifo->wr + len > fifo->size) {  // 需要循环
    uart_memcpy(fifo->buffer + fifo->wr, data, fifo->size - fifo->wr);
    data += fifo->size - fifo->wr;
    len -= fifo->size - fifo->wr;
    fifo->wr = 0;
  }
  uart_memcpy(fifo->buffer + fifo->wr, data, len);
  fifo->wr = (fifo->wr + len) % fifo->size;
  fifo_exchange(fifo, 0);
  return 0;
}

static int fifo_lwprintf_fn(int ch, lwprintf_t *lwobj) {
  uart_fifo_tx_t *fifo = (uart_fifo_tx_t *)lwobj->out_fn_arg;
  if (ch == '\0') {
    fifo_exchange(fifo, 0);
    return 0;
  }
  if (FIFO_TX_FREE_SPACE(fifo) < 1) {  // FIFO空间不足
    if (wait_fifo(fifo, 1) < 0) return -1;
  }
  fifo->buffer[fifo->wr] = ch;
  fifo->wr = (fifo->wr + 1) % fifo->size;
  return ch;
}
#endif  // _UART_ENABLE_TX_FIFO

static int pub_lwprintf_fn(int ch, lwprintf_t *lwobj) {
  UART_HandleTypeDef *huart = (UART_HandleTypeDef *)lwobj->out_fn_arg;
  if (ch == '\0') return 0;
  if (huart) {
    while (_UART_NOT_READY) {
      __NOP();
    }
    _send_func(huart, (uint8_t *)&ch, 1);
    return ch;
  }
  return -1;
}

uint8_t disable_printft = 0;  // 关闭printf输出

int printft(UART_HandleTypeDef *huart, char *fmt, ...) {
  if (unlikely(disable_printft)) return 0;
  int sendLen;
  va_list ap;
  lwprintf_t lwp_pub;
#if _UART_ENABLE_TX_FIFO
  uart_fifo_tx_t *fifo = is_fifo_tx(huart);
  if (fifo) {
    lwp_pub.out_fn = fifo_lwprintf_fn;
    lwp_pub.out_fn_arg = (void *)fifo;
    va_start(ap, fmt);
    sendLen = lwprintf_vprintf_ex(&lwp_pub, fmt, ap);
    va_end(ap);
    return sendLen;
  }
#endif
  lwp_pub.out_fn = pub_lwprintf_fn;
  lwp_pub.out_fn_arg = (void *)huart;
  va_start(ap, fmt);
  sendLen = lwprintf_vprintf_ex(&lwp_pub, fmt, ap);
  va_end(ap);
  return sendLen;
}

void printft_flush(UART_HandleTypeDef *huart) {
#if _UART_ENABLE_TX_FIFO
  uart_fifo_tx_t *fifo = is_fifo_tx(huart);
  if (fifo) {
    while (FIFO_TX_DATA_LENGTH(fifo)) {
      fifo_exchange(fifo, 0);
    }
  }
#endif
  while (_UART_NOT_READY) {
    __NOP();
  }
}

void print_hex(const char *text, uint8_t *data, uint16_t len) {
  uint16_t i;
  printf("%s [ ", text);
  for (i = 0; i < len; i++) {
    printf("%02X ", data[i]);
  }
  printf("]\r\n");
}

int Uart_Send(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len) {
  if (!len) return 0;
#if _UART_ENABLE_TX_FIFO
  uart_fifo_tx_t *fifo = is_fifo_tx(huart);
  if (fifo) return fifo_send(fifo, data, len);
#endif
#if _UART_TX_TIMEOUT > 0
  m_time_t _start_time = m_time_ms();
  while (_UART_NOT_READY) {
    if (m_time_ms() - _start_time > _UART_TX_TIMEOUT) return -1;
  }
#elif _UART_TX_TIMEOUT < 0
  if (_UART_NOT_READY) return -1;
#else
  while (_UART_NOT_READY) {
    __NOP();
  }
#endif
  _send_func(huart, data, len);
  return 0;
}

void Uart_Putchar(UART_HandleTypeDef *huart, uint8_t data) {
  Uart_Send(huart, &data, 1);
}

void Uart_Puts(UART_HandleTypeDef *huart, char *str) {
  Uart_Send(huart, (uint8_t *)str, strlen(str));
}

int Uart_Getchar(UART_HandleTypeDef *huart) {
  uint8_t data;
  if (HAL_UART_Receive(huart, &data, 1, 0xFFFF) != HAL_OK) {
    return EOF;
  }
  return data;
}

char *Uart_Gets(UART_HandleTypeDef *huart, char *str) {
  char *p = str;
  while (1) {
    int c = Uart_Getchar(huart);
    if (c == EOF) {
      return NULL;
    }
    *p++ = c;
    if (c == '\n') {
      break;
    }
  }
  *p = '\0';
  return str;
}

/**
 * @brief 串口发送数据，阻塞时不等待
 */
int Uart_SendFast(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len) {
  if (!len) return 0;
  if (_UART_NOT_READY) return -1;
  _send_func(huart, data, len);
  return 0;
}

static uart_fifo_rx_t *fifo_rx_entry = NULL;

void Uart_FifoRxInit(uart_fifo_rx_t *ctrl, UART_HandleTypeDef *huart,
                     void (*rxCallback)(uint8_t data)) {
  ctrl->huart = huart;
  ctrl->full = 0;
  ctrl->wr = 0;
  ctrl->rd = 0;
  ctrl->rxCallback = rxCallback;
  ctrl->next = NULL;
  HAL_UART_Receive_IT(huart, ctrl->fifo, 1);
  if (!fifo_rx_entry) {
    fifo_rx_entry = ctrl;
  } else {
    uart_fifo_rx_t *item = fifo_rx_entry;
    while (item->next) item = item->next;
    item->next = ctrl;
  }
}

inline void Uart_RxProcess(UART_HandleTypeDef *huart) {
  uart_fifo_rx_t *ctrl = fifo_rx_entry;
  while (ctrl) {
    if (ctrl->huart != huart) {
      ctrl = ctrl->next;
      continue;
    }
    if (!ctrl->full) ctrl->wr = (ctrl->wr + 1) % _UART_RX_BUF_SIZE;
    if (ctrl->wr == (ctrl->rd > 0 ? ctrl->rd - 1 : _UART_RX_BUF_SIZE - 1))
      ctrl->full = 1;
    HAL_UART_Receive_IT(ctrl->huart, ctrl->fifo + ctrl->wr, 1);
    return;
  }
}

void (*swRxCallback)(char *data, uint16_t len) = NULL;
char *swRxData = NULL;
uint16_t swRxLen = 0;

void Uart_CallbackCheck(void) {
  if (swRxCallback != NULL) {
    swRxCallback(swRxData, swRxLen);
    swRxCallback = NULL;
  }
  uart_fifo_rx_t *ctrl = fifo_rx_entry;
  while (ctrl) {
    while (ctrl->rd != ctrl->wr) {
      ctrl->rxCallback(ctrl->fifo[ctrl->rd]);
      ctrl->rd = (ctrl->rd + 1) % _UART_RX_BUF_SIZE;
      ctrl->full = 0;
    }
    ctrl = ctrl->next;
  }
}

inline void Uart_TxProcess(UART_HandleTypeDef *huart) {
#if _UART_ENABLE_TX_FIFO
  uart_fifo_tx_t *fifo = is_fifo_tx(huart);
  if (fifo) fifo_exchange(fifo, 1);
#endif
}

#if _UART_ENABLE_DMA

static uart_dma_rx_t *dma_rx_entry = NULL;
void Uart_DmaRxInit(uart_dma_rx_t *ctrl, UART_HandleTypeDef *huart,
                    void (*rxCallback)(char *data, uint16_t len),
                    uint8_t cbkInIRQ) {
  ctrl->huart = huart;
  ctrl->finished = 0;
  ctrl->len = 0;
  ctrl->buffer[0] = 0;
  ctrl->rxCallback = rxCallback;
  ctrl->cbkInIRQ = cbkInIRQ;
  ctrl->next = NULL;
  HAL_UARTEx_ReceiveToIdle_DMA(huart, ctrl->rxBuf, _UART_RX_BUF_SIZE - 1);
  if (!dma_rx_entry) {
    dma_rx_entry = ctrl;
  } else {
    uart_dma_rx_t *dma_rx = dma_rx_entry;
    while (dma_rx->next) dma_rx = dma_rx->next;
    dma_rx->next = ctrl;
  }
}

inline void Uart_DmaRxProcess(UART_HandleTypeDef *huart, uint16_t Size) {
  uart_dma_rx_t *dma_rx = dma_rx_entry;
  while (dma_rx) {
    if (dma_rx->huart != huart) {
      dma_rx = dma_rx->next;
      continue;
    }
#if _UART_DCACHE_COMPATIBLE
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)dma_rx->rxBuf,
                                      ((Size + 31) / 32) * 32);
#endif
    uart_memcpy(dma_rx->buffer, dma_rx->rxBuf, Size);
    dma_rx->len = Size;
    dma_rx->buffer[Size] = 0;
    dma_rx->finished = 1;
    HAL_UARTEx_ReceiveToIdle_DMA(dma_rx->huart, dma_rx->rxBuf,
                                 _UART_RX_BUF_SIZE - 1);
    if (dma_rx->rxCallback) {
      if (dma_rx->cbkInIRQ) {
        dma_rx->rxCallback((char *)dma_rx->buffer, dma_rx->len);
      } else {
        swRxCallback = dma_rx->rxCallback;
        swRxData = (char *)dma_rx->buffer;
        swRxLen = dma_rx->len;
      }
    }
    return;
  }
}
#endif  // _UART_ENABLE_DMA

__IO uint8_t uart_error_state = 0;

inline void Uart_ErrorProcess(UART_HandleTypeDef *huart) {
  if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_PE)) != RESET) {  // 奇偶校验错误
    __HAL_UNLOCK(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);
    uart_error_state = 1;
  }
  if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_FE)) != RESET) {  // 帧错误
    __HAL_UNLOCK(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    uart_error_state = 2;
  }
  if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_NE)) != RESET) {  // 噪声错误
    __HAL_UNLOCK(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    uart_error_state = 3;
  }
  if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE)) != RESET) {  // 接收溢出
    __HAL_UNLOCK(huart);
    __HAL_UART_CLEAR_OREFLAG(huart);
    uart_error_state = 4;
  }
  // 自动重启 DMA
#if _UART_ENABLE_DMA
  uart_dma_rx_t *dma_rx = dma_rx_entry;
  while (dma_rx) {
    if (dma_rx->huart->Instance == huart->Instance) {
      HAL_UART_DMAStop(dma_rx->huart);
      HAL_UARTEx_ReceiveToIdle_DMA(dma_rx->huart, dma_rx->rxBuf,
                                   _UART_RX_BUF_SIZE - 1);
      // break;
    }
    dma_rx = dma_rx->next;
  }
#endif
  // 自动重启中断
  uart_fifo_rx_t *fifo_rx = fifo_rx_entry;
  while (fifo_rx) {
    if (fifo_rx->huart->Instance == huart->Instance) {
      HAL_UART_AbortReceive_IT(fifo_rx->huart);
      HAL_UART_Receive_IT(fifo_rx->huart, fifo_rx->fifo + fifo_rx->wr, 1);
    }
    fifo_rx = fifo_rx->next;
  }
}

#if _UART_REWRITE_HANLDER  // 重写HAL中断处理函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  Uart_RxProcess(huart);
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  Uart_TxProcess(huart);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  Uart_ErrorProcess(huart);
}
#if _UART_ENABLE_DMA
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  Uart_DmaRxProcess(huart, Size);
}
void HAL_UARTEx_TxEventCallback(UART_HandleTypeDef *huart) {
  Uart_TxProcess(huart);
}
#endif  // _UART_ENABLE_DMA
#endif  // _UART_REWRITE_HANLDER

__weak void Assert_Failed_Handler(char *file, uint32_t line) {}

#if _UART_ENABLE_CDC
usb_cdc_ctrl_t usb_cdc;  // USB CDC 接收控制器
extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];
__IO static USBD_CDC_HandleTypeDef *hcdc = NULL;

static int8_t Hook_CDC_Init_FS(void) {
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
  return (USBD_OK);
}

static int8_t Hook_CDC_DeInit_FS(void) { return (USBD_OK); }

static int8_t Hook_CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length) {
#if 0
  // detect baudrate change
#define MAGIC_BAUDRATE 123456
  extern void reset_to_dfu(void);
  if (cmd == CDC_SET_LINE_CODING) {
    uint32_t baudrate = (uint32_t)(pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16) |
                                   (pbuf[3] << 24));
    if (baudrate == MAGIC_BAUDRATE) {
      reset_to_dfu();
    }
  }
#endif
  return (USBD_OK);
}

static int8_t Hook_CDC_Receive_FS(uint8_t *Buf, uint32_t *Len) {
  uart_memcpy(usb_cdc.buffer, Buf, *Len);
  usb_cdc.len = *Len;
  usb_cdc.buffer[*Len] = 0;
  usb_cdc.finished = 1;
  if (usb_cdc.cbkInIRQ) {
    usb_cdc.rxCallback((char *)usb_cdc.buffer, usb_cdc.len);
  } else {
    swRxCallback = usb_cdc.rxCallback;
    swRxData = (char *)usb_cdc.buffer;
    swRxLen = usb_cdc.len;
  }
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  return (USBD_OK);
}

static int8_t Hook_CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len,
                                       uint8_t epnum) {
  return USBD_OK;
}

__attribute__((constructor(255)))  // 自动Hook
  void
  Hook_CDC_Init(void) {
  // hook USBD_Interface_fops_FS
  USBD_Interface_fops_FS.Init = Hook_CDC_Init_FS;
  USBD_Interface_fops_FS.DeInit = Hook_CDC_DeInit_FS;
  USBD_Interface_fops_FS.Control = Hook_CDC_Control_FS;
  USBD_Interface_fops_FS.Receive = Hook_CDC_Receive_FS;
#if 1  // TransmitCplt 只在部分MCU上支持
  USBD_Interface_fops_FS.TransmitCplt = Hook_CDC_TransmitCplt_FS;
#endif
}

int32_t _cdc_start_time = 0;
int printfcdc(char *fmt, ...) {
  if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -3;
  _cdc_start_time = m_time_ms();
  while (hcdc->TxState != 0) {  // 检查上一次发送是否完成
#if _UART_CDC_TIMEOUT > 0
    if (m_time_ms() - _cdc_start_time > _UART_CDC_TIMEOUT) return -1;
#else
    return -1;
#endif
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -2;
  }
  va_list ap;
  va_start(ap, fmt);
  int sendLen =
      lwprintf_vsnprintf((char *)UserTxBufferFS, APP_TX_DATA_SIZE, fmt, ap);
  va_end(ap);
  if (sendLen > 0) {
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, sendLen);
    USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  }
  return sendLen;
}

void printfcdc_flush(void) {
  _cdc_start_time = m_time_ms();
  while (hcdc->TxState != 0) {
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return;
    if (m_time_ms() - _cdc_start_time > _UART_CDC_TIMEOUT) return;
  }
}

int CDC_Send(uint8_t *buf, uint16_t len) {
  if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -3;
  _cdc_start_time = m_time_ms();
  while (hcdc->TxState != 0) {  // 检查上一次发送是否完成
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -2;
    if (m_time_ms() - _cdc_start_time > _UART_CDC_TIMEOUT) return -1;
  }
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, buf, len);
  USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  return 0;
}

static const char _cbuf[] = "\r\n";

void CDC_WaitConnect(int timeout_ms) {
  _cdc_start_time = m_time_ms();
  while (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
    if (timeout_ms > 0 && m_time_ms() - _cdc_start_time > timeout_ms) return;
  }
  CDC_Send((uint8_t *)_cbuf, 2);
  while (hcdc->TxState != 0 ||
         hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
    if (timeout_ms > 0 && m_time_ms() - _cdc_start_time > timeout_ms) return;
  }
}

void CDC_RegisterCallback(void (*callback)(char *buf, uint16_t len),
                          uint8_t cbkInIRQ) {
  usb_cdc.rxCallback = callback;
  usb_cdc.cbkInIRQ = cbkInIRQ;
}

uint8_t USB_Connected(void) {
  return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
}

void CDC_Putchar(uint8_t data) { CDC_Send(&data, 1); }

void CDC_Puts(char *data) { CDC_Send((uint8_t *)data, strlen(data)); }

#endif  // _UART_ENABLE_CDC

#if _VOFA_ENABLE

static float VOFA_Buffer[_VOFA_BUFFER_SIZE + 1];
static uint8_t vofa_index = 0;
static const uint32_t vofa_endbit = 0x7F800000;

__attribute__((always_inline, flatten)) void Vofa_Add(float value) {
  if (vofa_index < _VOFA_BUFFER_SIZE) VOFA_Buffer[vofa_index++] = value;
}

void Vofa_AddSeq(float *value, uint8_t len) {
  if (vofa_index + len >= _VOFA_BUFFER_SIZE) return;
  uart_memcpy(&VOFA_Buffer[vofa_index], value, len * sizeof(float));
  vofa_index += len;
}

void Vofa_Clear(void) { vofa_index = 0; }

void Vofa_Send(UART_HandleTypeDef *huart) {
  if (vofa_index == 0) return;
  Vofa_AddSeq((float *)&vofa_endbit, 1);
  Uart_Send(huart, (uint8_t *)VOFA_Buffer, vofa_index * sizeof(float));
  vofa_index = 0;
}

void Vofa_SendFast(UART_HandleTypeDef *huart) {
  if (vofa_index == 0) return;
  uart_memcpy(&VOFA_Buffer[vofa_index], &vofa_endbit, sizeof(float));
  Uart_SendFast(huart, (uint8_t *)VOFA_Buffer, ++vofa_index * sizeof(float));
  vofa_index = 0;
}

#if _UART_ENABLE_CDC
void Vofa_SendCDC(void) {
  if (vofa_index == 0) return;
  Vofa_AddSeq((float *)&vofa_endbit, 1);
  CDC_Send((uint8_t *)VOFA_Buffer, vofa_index * sizeof(float));
  vofa_index = 0;
}
#endif  // _UART_ENABLE_CDC
#endif  // _VOFA_ENABLE

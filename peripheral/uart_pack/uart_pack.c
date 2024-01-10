#include "uart_pack.h"

#include <stdarg.h>
#include <string.h>

#include "lfbb.h"
#include "lwprintf.h"
#include "ulist.h"

#if 0  // memcpy 实现
static inline void uart_memcpy(void *dst, const void *src, uint16_t len) {
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

static inline void _send_func(UART_HandleTypeDef *huart, const uint8_t *data,
                              uint16_t len) {
#if _UART_TX_USE_DMA
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

#if _UART_ENABLE_FIFO_TX
typedef struct {              // FIFO串口发送控制结构体
  LFBB_Inst_Type lfbb;        // 发送缓冲区
  size_t sending;             // 正在发送的数据长度
  UART_HandleTypeDef *huart;  // 串口句柄
#if _MOD_USE_OS
  MOD_MUTEX_HANDLE mutex;  // 互斥锁
#endif
} uart_fifo_tx_t;

static ulist_t fifo_tx_list = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .isize = sizeof(uart_fifo_tx_t),
    .cfg = ULIST_CFG_NO_ALLOC_EXTEND | ULIST_CFG_CLEAR_DIRTY_REGION,
};

int Uart_FifoTxInit(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t bufSize) {
  if (!bufSize) return -1;
  uart_fifo_tx_t *ctrl = (uart_fifo_tx_t *)ulist_append(&fifo_tx_list);
  if (!ctrl) return -1;
  if (!buf) {
    ctrl->lfbb.data = m_alloc(bufSize);
    if (!ctrl->lfbb.data) {
      ulist_remove(&fifo_tx_list, ctrl);
      return NULL;
    }
    buf = ctrl->lfbb.data;
  }
  LFBB_Init(&ctrl->lfbb, buf, bufSize);
  ctrl->huart = huart;
  ctrl->sending = 0;
#if _MOD_USE_OS
  ctrl->mutex = MOD_MUTEX_CREATE();
#endif
  return 0;
}

static uart_fifo_tx_t *is_fifo_tx(UART_HandleTypeDef *huart) {
  if (!fifo_tx_list.num) return NULL;
  ulist_foreach(&fifo_tx_list, uart_fifo_tx_t, ctrl) {
    if (ctrl->huart == huart) return ctrl;
  }
  return NULL;
}

static void fifo_exchange(uart_fifo_tx_t *ctrl, uint8_t force) {
  UART_HandleTypeDef *huart = ctrl->huart;
  if (!force && _UART_NOT_READY) return;  // 串口正在发送
  if (ctrl->sending) {
    LFBB_ReadRelease(&ctrl->lfbb, ctrl->sending);
    ctrl->sending = 0;
  }
  uint8_t *data = LFBB_ReadAcquire(&ctrl->lfbb, &ctrl->sending);
  if (data) {
    _send_func(huart, data, ctrl->sending);
  } else {
    ctrl->sending = 0;
  }
}

static inline uint8_t *wait_fifo(uart_fifo_tx_t *ctrl, const bool is2,
                                 uint16_t len, size_t *len2) {
#if _UART_FIFO_TIMEOUT < 0
  if (is2) {
    return LFBB_WriteAcquire2(&ctrl->lfbb, len2);
  } else {
    return LFBB_WriteAcquire(&ctrl->lfbb, len);
  }
#else
#if _UART_FIFO_TIMEOUT > 0
  m_time_t _start_time = m_time_ms();
#endif  // _UART_FIFO_TIMEOUT
  uint8_t *data;
  while (1) {
    if (is2) {
      data = LFBB_WriteAcquire2(&ctrl->lfbb, len2);
    } else {
      data = LFBB_WriteAcquire(&ctrl->lfbb, len);
    }
    if (data) return data;
    fifo_exchange(ctrl, 0);
#if _UART_FIFO_TIMEOUT > 0
    m_delay_ms(1);
    if (m_time_ms() - _start_time > _UART_FIFO_TIMEOUT) return NULL;
#endif  // _UART_FIFO_TIMEOUT
  }
  return NULL;
#endif  // _UART_FIFO_TIMEOUT
}

static inline int fifo_send(uart_fifo_tx_t *ctrl, const uint8_t *data, uint16_t len) {
  if (len > ctrl->lfbb.size - 1) return -1;
  MOD_MUTEX_ACQUIRE(ctrl->mutex);
  uint8_t *buf = wait_fifo(ctrl, false, len, NULL);
  if (!buf) {
    MOD_MUTEX_RELEASE(ctrl->mutex);
    return -1;
  }
  uart_memcpy(buf, data, len);
  LFBB_WriteRelease(&ctrl->lfbb, len);
  MOD_MUTEX_RELEASE(ctrl->mutex);
  fifo_exchange(ctrl, 0);
  return 0;
}

static inline size_t fifo_send_va(uart_fifo_tx_t *ctrl, const char *fmt, va_list ap) {
  size_t sendLen;
  MOD_MUTEX_ACQUIRE(ctrl->mutex);
  uint8_t *buf = wait_fifo(ctrl, true, 0, &sendLen);
  if (!buf) {
    MOD_MUTEX_RELEASE(ctrl->mutex);
    return 0;
  }
  sendLen = lwprintf_vsnprintf((char *)buf, sendLen, fmt, ap);
  LFBB_WriteRelease(&ctrl->lfbb, sendLen);
  fifo_exchange(ctrl, 0);
  MOD_MUTEX_RELEASE(ctrl->mutex);
  return sendLen;
}

#endif  // _UART_ENABLE_FIFO_TX

static int pub_lwprintf_fn(int ch, lwprintf_t *lwobj) {
  if (ch == '\0') return 0;
  UART_HandleTypeDef *huart = (UART_HandleTypeDef *)lwobj->arg;
  if (huart) {
    HAL_UART_Transmit(huart, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
  }
  return -1;
}

uint8_t disable_printft = 0;  // 关闭printf输出

int printft(UART_HandleTypeDef *huart, const char *fmt, ...) {
  if (unlikely(disable_printft)) return 0;
  int sendLen;
  va_list ap;
#if _UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *ctrl = is_fifo_tx(huart);
  if (ctrl) {
    va_start(ap, fmt);
    sendLen = fifo_send_va(ctrl, fmt, ap);
    va_end(ap);
    return sendLen;
  }
#endif
  lwprintf_t lwp_pub;
  lwp_pub.out_fn = pub_lwprintf_fn;
  lwp_pub.arg = (void *)huart;
  va_start(ap, fmt);
  sendLen = lwprintf_vprintf_ex(&lwp_pub, fmt, ap);
  va_end(ap);
  return sendLen;
}

void printft_flush(UART_HandleTypeDef *huart) {
#if _UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *ctrl = is_fifo_tx(huart);
  if (ctrl) {
    while (!LFBB_IsEmpty(&ctrl->lfbb)) {
      fifo_exchange(ctrl, 0);
    }
  }
#endif
  while (_UART_NOT_READY) {
    __NOP();
  }
}

int Uart_Send(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len) {
  if (!len) return 0;
#if _UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *ctrl = is_fifo_tx(huart);
  if (ctrl) return fifo_send(ctrl, data, len);
#endif
#if _UART_TX_TIMEOUT > 0
  m_time_t _start_time = m_time_ms();
  while (_UART_NOT_READY) {
    m_delay_ms(1);
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

void Uart_Puts(UART_HandleTypeDef *huart, const char *str) {
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

int Uart_SendFast(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len) {
  if (!len) return 0;
  if (_UART_NOT_READY) return -1;
  _send_func(huart, data, len);
  return 0;
}

typedef struct {                      // 中断FIFO串口接收控制结构体
  lfifo_t fifo;                       // 接收保存区
  uint8_t tmp;                        // 临时变量
  UART_HandleTypeDef *huart;          // 串口句柄
  void (*rxCallback)(lfifo_t *fifo);  // 接收完成回调函数
} uart_fifo_rx_t;

static ulist_t fifo_rx_list = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .isize = sizeof(uart_fifo_rx_t),
    .cfg = ULIST_CFG_NO_ALLOC_EXTEND | ULIST_CFG_CLEAR_DIRTY_REGION,
};

lfifo_t *Uart_FifoRxInit(UART_HandleTypeDef *huart,
                         void (*rxCallback)(lfifo_t *fifo), uint8_t *buf,
                         uint16_t bufSize) {
  if (!bufSize) return NULL;
  uart_fifo_rx_t *ctrl = (uart_fifo_rx_t *)ulist_append(&fifo_rx_list);
  if (!ctrl) return NULL;
  if (!buf) {
    if (LFifo_Init(&ctrl->fifo, bufSize) != 0) {
      ulist_remove(&fifo_rx_list, ctrl);
      return NULL;
    }
  } else {
    LFifo_AssignBuf(&ctrl->fifo, buf, bufSize);
  }
  ctrl->huart = huart;
  ctrl->rxCallback = rxCallback;
  HAL_UART_Receive_IT(huart, &ctrl->tmp, 1);
  return &ctrl->fifo;
}

inline void Uart_RxProcess(UART_HandleTypeDef *huart) {
  ulist_foreach(&fifo_rx_list, uart_fifo_rx_t, ctrl) {
    if (ctrl->huart != huart) continue;
    LFifo_WriteByte(&ctrl->fifo, ctrl->tmp);
    HAL_UART_Receive_IT(ctrl->huart, &ctrl->tmp, 1);
    return;
  }
}

inline void Uart_TxProcess(UART_HandleTypeDef *huart) {
#if _UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *fifo = is_fifo_tx(huart);
  if (fifo) fifo_exchange(fifo, 1);
#endif
}

#if _UART_ENABLE_DMA_RX
static uint8_t dma_tmp = 0;

typedef struct {                            // DMA串口接收控制结构体
  LFBB_Inst_Type lfbb;                      // 接收缓冲区
  uint8_t *pBuffer;                         // 接收缓冲区
  size_t pSize;                             // 接收缓冲区大小
  UART_HandleTypeDef *huart;                // 串口句柄
  void (*rxCallback)(uint8_t *, uint16_t);  // 接收完成回调函数
  uint8_t cbkInIRQ;  // 回调函数是否在中断中执行
} uart_dma_rx_t;

static ulist_t dma_rx_list = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .isize = sizeof(uart_dma_rx_t),
    .cfg = ULIST_CFG_NO_ALLOC_EXTEND | ULIST_CFG_CLEAR_DIRTY_REGION,
};

LFBB_Inst_Type *Uart_DmaRxInit(UART_HandleTypeDef *huart,
                               void (*rxCallback)(uint8_t *data, uint16_t len),
                               uint8_t cbkInIRQ, uint8_t *buf,
                               uint16_t bufSize) {
  if (!bufSize) return NULL;
  uart_dma_rx_t *ctrl = (uart_dma_rx_t *)ulist_append(&dma_rx_list);
  if (!ctrl) return NULL;
  ctrl->huart = huart;
  ctrl->rxCallback = rxCallback;
  ctrl->cbkInIRQ = cbkInIRQ;
  if (!buf) {
    ctrl->lfbb.data = m_alloc(bufSize);
    if (!ctrl->lfbb.data) {
      ulist_remove(&dma_rx_list, ctrl);
      return NULL;
    }
    buf = ctrl->lfbb.data;
  }
  LFBB_Init(&ctrl->lfbb, buf, bufSize);
  ctrl->pBuffer = LFBB_WriteAcquire2(&ctrl->lfbb, &ctrl->pSize);
  if (!ctrl->pBuffer) {
    m_free(ctrl->lfbb.data);
    ulist_remove(&dma_rx_list, ctrl);
    return NULL;
  }
  HAL_UARTEx_ReceiveToIdle_DMA(huart, ctrl->pBuffer, ctrl->pSize);
  return &ctrl->lfbb;
}

inline void Uart_DmaRxProcess(UART_HandleTypeDef *huart, uint16_t Size) {
  size_t len;
  uint8_t *data;
  ulist_foreach(&dma_rx_list, uart_dma_rx_t, ctrl) {
    if (ctrl->huart != huart) continue;
    if (ctrl->pBuffer != &dma_tmp) {
      LFBB_WriteRelease(&ctrl->lfbb, Size);
#if _UART_DCACHE_COMPATIBLE
      SCB_CleanInvalidateDCache_by_Addr((uint32_t *)ctrl->pBuffer,
                                        ((ctrl->pSize + 31) / 32) * 32);
#endif
    }
    ctrl->pBuffer = LFBB_WriteAcquire2(&ctrl->lfbb, &ctrl->pSize);
    if (!ctrl->pBuffer) {
      ctrl->pBuffer = &dma_tmp;
      ctrl->pSize = 1;
    }
    HAL_UARTEx_ReceiveToIdle_DMA(huart, ctrl->pBuffer, ctrl->pSize);
    if (ctrl->rxCallback && ctrl->cbkInIRQ) {
      data = LFBB_ReadAcquire(&ctrl->lfbb, &len);
      if (!data) return;
      ctrl->rxCallback(data, len);
      LFBB_ReadRelease(&ctrl->lfbb, len);
    }
    return;
  }
}
#endif  // _UART_ENABLE_DMA_RX

#if _UART_ENABLE_CDC
static void (*swRxCallback)(char *data, uint16_t len) = NULL;
static char *swRxData = NULL;
static uint16_t swRxLen = 0;
#endif  // _UART_ENABLE_CDC

void Uart_CallbackCheck(void) {
  ulist_foreach(&fifo_rx_list, uart_fifo_rx_t, ctrl) {
    if (ctrl->rxCallback == NULL) continue;
    while (!LFifo_IsEmpty(&ctrl->fifo)) {
      ctrl->rxCallback(&ctrl->fifo);
    }
  }
#if _UART_ENABLE_DMA_RX
  size_t len;
  uint8_t *data;
  ulist_foreach(&dma_rx_list, uart_dma_rx_t, ctrl) {
    if (ctrl->rxCallback == NULL || ctrl->cbkInIRQ) continue;
    data = LFBB_ReadAcquire(&ctrl->lfbb, &len);
    if (!data) continue;
    ctrl->rxCallback(data, len);
    LFBB_ReadRelease(&ctrl->lfbb, len);
  }
#endif  // _UART_ENABLE_DMA_RX
#if _UART_ENABLE_CDC
  if (swRxCallback != NULL) {
    swRxCallback(swRxData, swRxLen);
    swRxCallback = NULL;
  }
#endif  // _UART_ENABLE_CDC
}

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
  ulist_foreach(&fifo_rx_list, uart_fifo_rx_t, ctrl) {
    if (ctrl->huart->Instance == huart->Instance) {
      HAL_UART_AbortReceive_IT(ctrl->huart);
      HAL_UART_Receive_IT(ctrl->huart, &ctrl->tmp, 1);
    }
  }
#if _UART_ENABLE_DMA_RX
  ulist_foreach(&dma_rx_list, uart_dma_rx_t, ctrl) {
    if (ctrl->huart->Instance == huart->Instance) {
      HAL_UART_DMAStop(ctrl->huart);
      HAL_UARTEx_ReceiveToIdle_DMA(huart, ctrl->pBuffer, ctrl->pSize);
    }
  }
#endif
#if _UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *fifo = is_fifo_tx(huart);
  if (fifo) {
    HAL_UART_DMAStop(fifo->huart);
    HAL_UART_AbortTransmit_IT(fifo->huart);
    fifo->sending = 0;
    fifo_exchange(fifo, 1);
  }
#endif
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
#if _UART_ENABLE_DMA_RX
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  Uart_DmaRxProcess(huart, Size);
}
void HAL_UARTEx_TxEventCallback(UART_HandleTypeDef *huart) {
  Uart_TxProcess(huart);
}
#endif  // _UART_ENABLE_DMA_RX
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
// TransmitCplt 只在部分MCU上支持
#if __has_keyword(USBD_Interface_fops_FS, TransmitCplt)
  USBD_Interface_fops_FS.TransmitCplt = Hook_CDC_TransmitCplt_FS;
#endif
}

int printfcdc(char *fmt, ...) {
  if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -3;
  m_time_t _cdc_start_time = m_time_ms();
  while (hcdc->TxState != 0) {  // 检查上一次发送是否完成
#if _UART_CDC_TIMEOUT > 0
    m_delay_ms(1);
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
  m_time_t _cdc_start_time = m_time_ms();
  while (hcdc->TxState != 0) {
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return;
    m_delay_ms(1);
    if (m_time_ms() - _cdc_start_time > _UART_CDC_TIMEOUT) return;
  }
}

int CDC_Send(uint8_t *buf, uint16_t len) {
  if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -3;
  m_time_t _cdc_start_time = m_time_ms();
  while (hcdc->TxState != 0) {  // 检查上一次发送是否完成
#if _UART_CDC_TIMEOUT > 0
    m_delay_ms(1);
    if (m_time_ms() - _cdc_start_time > _UART_CDC_TIMEOUT) return -1;
#else
    return -1;
#endif
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -2;
  }
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, buf, len);
  USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  return 0;
}

static const char _cbuf[] = "\r\n";

void CDC_WaitConnect(int timeout_ms) {
  m_time_t _cdc_start_time = m_time_ms();
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

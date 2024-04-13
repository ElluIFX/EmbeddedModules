/**
 * @file uio_uart.c
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#include "uio_uart.h"

#if UIO_CFG_ENABLE_UART

#include <stdarg.h>
#include <string.h>

#include "lwprintf.h"
#include "ulist.h"

#if UIO_CFG_UART_TX_USE_DMA
#define HUART_BUSY                          \
  (huart->gState != HAL_UART_STATE_READY || \
   (huart->hdmatx && huart->hdmatx->State != HAL_DMA_STATE_READY))
#else
#define HUART_BUSY huart->gState != HAL_UART_STATE_READY
#endif

static inline HAL_StatusTypeDef uart_send_raw(UART_HandleTypeDef *huart,
                                              const uint8_t *data, size_t len) {
#if UIO_CFG_UART_TX_USE_DMA
  if (huart->hdmatx) {  // dma available
#if UIO_CFG_UART_DCACHE_COMPATIBLE
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)data, len);
#endif
    return HAL_UART_Transmit_DMA(huart, data, len);
  } else
#endif
#if UIO_CFG_UART_TX_USE_IT
    return HAL_UART_Transmit_IT(huart, data, len);
#else
  return HAL_UART_Transmit(huart, data, len, UIO_CFG_UART_TX_TIMEOUT);
#endif
}

#if UIO_CFG_UART_ENABLE_FIFO_TX
typedef struct {              // FIFO串口发送控制结构体
  LFBB_Inst_Type lfbb;        // 发送缓冲区
  size_t sending;             // 正在发送的数据长度
  UART_HandleTypeDef *huart;  // 串口句柄
  MOD_MUTEX_HANDLE mutex;     // 互斥锁
} uart_fifo_tx_t;

static ulist_t fifo_tx_list = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .isize = sizeof(uart_fifo_tx_t),
    .cfg = ULIST_CFG_NO_ALLOC_EXTEND | ULIST_CFG_CLEAR_DIRTY_REGION,
};

int uart_fifo_tx_init(UART_HandleTypeDef *huart, uint8_t *buf,
                      size_t buf_size) {
  if (!buf_size) return -1;
  uart_fifo_tx_t *ctrl = (uart_fifo_tx_t *)ulist_append(&fifo_tx_list);
  if (!ctrl) return -1;
  if (!buf) {
    ctrl->lfbb.data = m_alloc(buf_size);
    if (!ctrl->lfbb.data) {
      ulist_remove(&fifo_tx_list, ctrl);
      return NULL;
    }
    buf = ctrl->lfbb.data;
  }
  LFBB_Init(&ctrl->lfbb, buf, buf_size);
  ctrl->huart = huart;
  ctrl->sending = 0;
  ctrl->mutex = MOD_MUTEX_CREATE("uartTx");
  return 0;
}

void uart_fifo_tx_deinit(UART_HandleTypeDef *huart) {
  ulist_foreach(&fifo_tx_list, uart_fifo_tx_t, ctrl) {
    if (ctrl->huart == huart) {
      HAL_UART_DMAStop(ctrl->huart);
      HAL_UART_AbortTransmit_IT(ctrl->huart);
      MOD_MUTEX_DELETE(ctrl->mutex);
      m_free(ctrl->lfbb.data);
      ulist_remove(&fifo_tx_list, ctrl);
      return;
    }
  }
}

static uart_fifo_tx_t *uart_fifo_tx_get_handle(UART_HandleTypeDef *huart) {
  if (!fifo_tx_list.num) return NULL;
  ulist_foreach(&fifo_tx_list, uart_fifo_tx_t, ctrl) {
    if (ctrl->huart == huart) return ctrl;
  }
  return NULL;
}

static void uart_fifo_tx_exchange(uart_fifo_tx_t *ctrl, uint8_t from_irq) {
  UART_HandleTypeDef *huart = ctrl->huart;
  if (!from_irq && HUART_BUSY) return;  // 串口正在发送
  if (ctrl->sending) {
    LFBB_ReadRelease(&ctrl->lfbb, ctrl->sending);
    ctrl->sending = 0;
  }
  uint8_t *data = LFBB_ReadAcquire(&ctrl->lfbb, &ctrl->sending);
  if (data) {
    uart_send_raw(huart, data, ctrl->sending);
  } else {
    ctrl->sending = 0;
  }
}

static inline uint8_t *uart_fifo_tx_wait_free(uart_fifo_tx_t *ctrl,
                                              size_t len) {
  if (len > ctrl->lfbb.size - 1) return NULL;
#if UIO_CFG_UART_FIFO_TIMEOUT < 0
  return LFBB_WriteAcquire(&ctrl->lfbb, len);
#else
#if UIO_CFG_UART_FIFO_TIMEOUT > 0
  m_time_t _start_time = m_time_ms();
#endif  // UIO_CFG_UART_FIFO_TIMEOUT
  uint8_t *data;
  while (1) {
    data = LFBB_WriteAcquire(&ctrl->lfbb, len);
    if (data) return data;
    uart_fifo_tx_exchange(ctrl, 0);
#if UIO_CFG_UART_FIFO_TIMEOUT > 0
    m_delay_ms(1);
    if (m_time_ms() - _start_time > UIO_CFG_UART_FIFO_TIMEOUT) return NULL;
#endif  // UIO_CFG_UART_FIFO_TIMEOUT
  }
  return NULL;
#endif  // UIO_CFG_UART_FIFO_TIMEOUT
}

static inline int uart_fifo_send(uart_fifo_tx_t *ctrl, const uint8_t *data,
                                 size_t len) {
  MOD_MUTEX_ACQUIRE(ctrl->mutex);
  uint8_t *buf = uart_fifo_tx_wait_free(ctrl, len);
  if (!buf) {
    MOD_MUTEX_RELEASE(ctrl->mutex);
    return -1;
  }
  memcpy(buf, data, len);
  LFBB_WriteRelease(&ctrl->lfbb, len);
  uart_fifo_tx_exchange(ctrl, 0);
  MOD_MUTEX_RELEASE(ctrl->mutex);
  return 0;
}

static inline size_t uart_fifo_send_va(uart_fifo_tx_t *ctrl, const char *fmt,
                                       va_list ap) {
  size_t sendLen, writeLen;
  MOD_MUTEX_ACQUIRE(ctrl->mutex);
  // 第一次尝试直接获取缓冲区最大可用长度
  uint8_t *buf = LFBB_WriteAcquireAlt(&ctrl->lfbb, &sendLen);
  if (!buf) {
    MOD_MUTEX_RELEASE(ctrl->mutex);
    return 0;
  }
  do {  // 尝试写入数据
    writeLen = lwprintf_vsnprintf((char *)buf, sendLen, fmt, ap);
    if (writeLen >= ctrl->lfbb.size - 1) {  // 需求长度超过缓冲区最大长度
      MOD_MUTEX_RELEASE(ctrl->mutex);
      return 0;
    }
    if (writeLen + 1 >= sendLen) {  // 数据长度超过缓冲区最大长度
      buf = uart_fifo_tx_wait_free(ctrl, writeLen + 2);  // 等待缓冲区可用
      if (!buf) {
        MOD_MUTEX_RELEASE(ctrl->mutex);
        return 0;
      }
      sendLen = writeLen + 2;
      continue;  // 重新写入数据
    }
    break;
  } while (1);
  LFBB_WriteRelease(&ctrl->lfbb, writeLen + 1);
  uart_fifo_tx_exchange(ctrl, 0);
  MOD_MUTEX_RELEASE(ctrl->mutex);
  return sendLen;
}

#endif  // UIO_CFG_UART_ENABLE_FIFO_TX

static int uart_block_tx_fn(int ch, lwprintf_t *lwobj) {
  if (ch == '\0') return 0;
  UART_HandleTypeDef *huart = (UART_HandleTypeDef *)lwobj->arg;
  while (HUART_BUSY) {
    __NOP();
  }
  if (huart) {
    HAL_UART_Transmit(huart, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
  }
  return -1;
}

static int uart_printf_block_ap(UART_HandleTypeDef *huart, const char *fmt,
                                va_list ap) {
  lwprintf_t lwp_pub;
  lwp_pub.out_fn = uart_block_tx_fn;
  lwp_pub.arg = (void *)huart;
  return lwprintf_vprintf_ex(&lwp_pub, fmt, ap);
}

int uart_printf_block(UART_HandleTypeDef *huart, const char *fmt, ...) {
  if (HUART_BUSY) {
    HAL_UART_DMAStop(huart);
    HAL_UART_AbortTransmit_IT(huart);
  }
  va_list ap;
  va_start(ap, fmt);
  int sendLen = uart_printf_block_ap(huart, fmt, ap);
  va_end(ap);
  return sendLen;
}

int uart_printf(UART_HandleTypeDef *huart, const char *fmt, ...) {
  int sendLen;
  va_list ap;
#if UIO_CFG_UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *ctrl = uart_fifo_tx_get_handle(huart);
  if (ctrl) {
    va_start(ap, fmt);
    sendLen = uart_fifo_send_va(ctrl, fmt, ap);
    va_end(ap);
    return sendLen;
  }
#endif
  va_start(ap, fmt);
  sendLen = uart_printf_block_ap(huart, fmt, ap);
  va_end(ap);
  return sendLen;
}

void uart_flush(UART_HandleTypeDef *huart) {
#if UIO_CFG_UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *ctrl = uart_fifo_tx_get_handle(huart);
  if (ctrl) {
    MOD_MUTEX_ACQUIRE(ctrl->mutex);
    while (!LFBB_IsEmpty(&ctrl->lfbb)) {
      uart_fifo_tx_exchange(ctrl, 0);
    }
    MOD_MUTEX_RELEASE(ctrl->mutex);
    return;
  }
#endif
  while (HUART_BUSY) {
    __NOP();
  }
}

int uart_write(UART_HandleTypeDef *huart, const uint8_t *data, size_t len) {
  if (!len) return 0;
#if UIO_CFG_UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *ctrl = uart_fifo_tx_get_handle(huart);
  if (ctrl) return uart_fifo_send(ctrl, data, len);
#endif
#if UIO_CFG_UART_TX_TIMEOUT > 0
  m_time_t _start_time = m_time_ms();
  while (HUART_BUSY) {
    // m_delay_ms(1);
    if (m_time_ms() - _start_time > UIO_CFG_UART_TX_TIMEOUT) return -1;
  }
#elif UIO_CFG_UART_TX_TIMEOUT < 0
  if (HUART_BUSY) return -1;
#else
  while (HUART_BUSY) {
    __NOP();
  }
#endif
  uart_send_raw(huart, data, len);
  return 0;
}

int uart_write_fast(UART_HandleTypeDef *huart, uint8_t *data, size_t len) {
  if (!len) return 0;
  if (HUART_BUSY) return -1;
  uart_send_raw(huart, data, len);
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

lfifo_t *uart_fifo_rx_init(UART_HandleTypeDef *huart, uint8_t *buf,
                           size_t buf_size, void (*rxCallback)(lfifo_t *fifo)) {
  if (!buf_size) return NULL;
  uart_fifo_rx_t *ctrl = (uart_fifo_rx_t *)ulist_append(&fifo_rx_list);
  if (!ctrl) return NULL;
  if (!buf) {
    if (LFifo_Init(&ctrl->fifo, buf_size) != 0) {
      ulist_remove(&fifo_rx_list, ctrl);
      return NULL;
    }
  } else {
    LFifo_AssignBuf(&ctrl->fifo, buf, buf_size);
  }
  ctrl->huart = huart;
  ctrl->rxCallback = rxCallback;
  HAL_UART_Receive_IT(huart, &ctrl->tmp, 1);
  return &ctrl->fifo;
}

inline void uart_rx_process(UART_HandleTypeDef *huart) {
  ulist_foreach(&fifo_rx_list, uart_fifo_rx_t, ctrl) {
    if (ctrl->huart != huart) continue;
    LFifo_WriteByte(&ctrl->fifo, ctrl->tmp);
    HAL_UART_Receive_IT(ctrl->huart, &ctrl->tmp, 1);
    return;
  }
}

inline void uart_tx_process(UART_HandleTypeDef *huart) {
#if UIO_CFG_UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *fifo = uart_fifo_tx_get_handle(huart);
  if (fifo) uart_fifo_tx_exchange(fifo, 1);
#endif
}

#if UIO_CFG_UART_ENABLE_DMA_RX
static uint8_t dma_tmp;  // DMA垃圾箱

typedef struct {                          // DMA串口接收控制结构体
  LFBB_Inst_Type lfbb;                    // 环形数据缓冲区
  uint8_t *pBuffer;                       // 接收缓冲区指针
  size_t pSize;                           // 接收缓冲区大小
  UART_HandleTypeDef *huart;              // 串口句柄
  void (*rxCallback)(uint8_t *, size_t);  // 接收完成回调函数
  uint8_t cbkInIRQ;  // 回调函数是否在中断中执行
} uart_dma_rx_t;

static ulist_t dma_rx_list = {
    .data = NULL,
    .cap = 0,
    .num = 0,
    .isize = sizeof(uart_dma_rx_t),
    .cfg = ULIST_CFG_NO_ALLOC_EXTEND | ULIST_CFG_CLEAR_DIRTY_REGION,
};

LFBB_Inst_Type *uart_dma_rx_init(UART_HandleTypeDef *huart, uint8_t *buf,
                                 size_t buf_size,
                                 void (*rxCallback)(uint8_t *data, size_t len),
                                 uint8_t cbkInIRQ) {
  if (!buf_size) return NULL;
  uart_dma_rx_t *ctrl = (uart_dma_rx_t *)ulist_append(&dma_rx_list);
  if (!ctrl) return NULL;
  ctrl->huart = huart;
  ctrl->rxCallback = rxCallback;
  ctrl->cbkInIRQ = cbkInIRQ;
  if (!buf) {
    ctrl->lfbb.data = m_alloc(buf_size);
    if (!ctrl->lfbb.data) {
      ulist_remove(&dma_rx_list, ctrl);
      return NULL;
    }
    buf = ctrl->lfbb.data;
  }
  LFBB_Init(&ctrl->lfbb, buf, buf_size);
  ctrl->pBuffer = LFBB_WriteAcquireAlt(&ctrl->lfbb, &ctrl->pSize);
  if (!ctrl->pBuffer) {
    m_free(ctrl->lfbb.data);
    ulist_remove(&dma_rx_list, ctrl);
    return NULL;
  }
  HAL_UARTEx_ReceiveToIdle_DMA(huart, ctrl->pBuffer, ctrl->pSize);
  return &ctrl->lfbb;
}

inline void uart_dma_rx_process(UART_HandleTypeDef *huart, size_t Size) {
  size_t len;
  uint8_t *data;
  ulist_foreach(&dma_rx_list, uart_dma_rx_t, ctrl) {
    if (ctrl->huart != huart) continue;
    if (ctrl->pBuffer != &dma_tmp) {  // 收到有效数据
      LFBB_WriteRelease(&ctrl->lfbb, Size);
#if UIO_CFG_UART_DCACHE_COMPATIBLE
      SCB_CleanInvalidateDCache_by_Addr(
          (uint32_t *)ctrl->pBuffer,
          ((ctrl->pSize + 31) / 32) * 32);  // 对齐
#endif
    }
    ctrl->pBuffer = LFBB_WriteAcquireAlt(&ctrl->lfbb, &ctrl->pSize);
    if (!ctrl->pBuffer) {  // 缓冲区已满，丢弃数据
      ctrl->pBuffer = &dma_tmp;
      ctrl->pSize = 1;
    }
    HAL_UARTEx_ReceiveToIdle_DMA(huart, ctrl->pBuffer, ctrl->pSize);
    if (ctrl->rxCallback && ctrl->cbkInIRQ) {
      data = LFBB_ReadAcquire(&ctrl->lfbb, &len);
      if (!data) return;  // 不应该发生
      ctrl->rxCallback(data, len);
      LFBB_ReadRelease(&ctrl->lfbb, len);
    }
    return;
  }
}
#endif  // UIO_CFG_UART_ENABLE_DMA_RX

void uart_check_callback(void) {
  ulist_foreach(&fifo_rx_list, uart_fifo_rx_t, ctrl) {
    if (ctrl->rxCallback == NULL) continue;
    if (!LFifo_IsEmpty(&ctrl->fifo)) {
      ctrl->rxCallback(&ctrl->fifo);
    }
  }
#if UIO_CFG_UART_ENABLE_DMA_RX
  size_t len;
  uint8_t *data;
  ulist_foreach(&dma_rx_list, uart_dma_rx_t, ctrl) {
    if (ctrl->rxCallback == NULL || ctrl->cbkInIRQ) continue;
    data = LFBB_ReadAcquire(&ctrl->lfbb, &len);
    if (!data) continue;
    ctrl->rxCallback(data, len);
    LFBB_ReadRelease(&ctrl->lfbb, len);
  }
#endif  // UIO_CFG_UART_ENABLE_DMA_RX
#if UIO_CFG_ENABLE_CDC
  extern void cdc_check_callback(void);
  cdc_check_callback();
#endif  // UIO_CFG_ENABLE_CDC
}

__IO uint8_t uart_error_state = 0;

inline void uart_error_process(UART_HandleTypeDef *huart) {
  if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_PE)) != 0) {  // 奇偶校验错误
    __HAL_UNLOCK(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);
    uart_error_state = 1;
  }
  if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_FE)) != 0) {  // 帧错误
    __HAL_UNLOCK(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    uart_error_state = 2;
  }
  if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_NE)) != 0) {  // 噪声错误
    __HAL_UNLOCK(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    uart_error_state = 3;
  }
  if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE)) != 0) {  // 接收溢出
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
#if UIO_CFG_UART_ENABLE_DMA_RX
  ulist_foreach(&dma_rx_list, uart_dma_rx_t, ctrl) {
    if (ctrl->huart->Instance == huart->Instance) {
      HAL_UART_DMAStop(ctrl->huart);
      HAL_UARTEx_ReceiveToIdle_DMA(huart, ctrl->pBuffer, ctrl->pSize);
    }
  }
#endif
#if UIO_CFG_UART_ENABLE_FIFO_TX
  uart_fifo_tx_t *fifo = uart_fifo_tx_get_handle(huart);
  if (fifo) {
    HAL_UART_DMAStop(fifo->huart);
    HAL_UART_AbortTransmit_IT(fifo->huart);
    fifo->sending = 0;
    uart_fifo_tx_exchange(fifo, 1);
  }
#endif
}

#if UIO_CFG_UART_REWRITE_HANLDER  // 重写HAL中断处理函数

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  uart_rx_process(huart);
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  uart_tx_process(huart);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  uart_error_process(huart);
}

#if UIO_CFG_UART_ENABLE_DMA_RX
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  uart_dma_rx_process(huart, Size);
}
void HAL_UARTEx_TxEventCallback(UART_HandleTypeDef *huart) {
  uart_tx_process(huart);
}
#endif  // UIO_CFG_UART_ENABLE_DMA_RX

#endif  // UIO_CFG_UART_REWRITE_HANLDER

#endif  // UIO_CFG_ENABLE_UART

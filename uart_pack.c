#include "uart_pack.h"

#include "stdarg.h"
#include "stdlib.h"
#include "string.h"

#define MAX_UART_NUM 8  // 最大串口数量

#if _PRINTF_USE_DMA
#define _UART_NOT_READY                     \
  (huart->gState != HAL_UART_STATE_READY || \
   huart->hdmatx->State != HAL_DMA_STATE_READY)
#else
#define _UART_NOT_READY huart->gState != HAL_UART_STATE_READY
#endif

#if _UART_TIMEOUT == 0
#define _WAIT_UART_READY(x) \
  while (_UART_NOT_READY) { \
    __NOP();                \
  }
#else
#define _WAIT_UART_READY(x)                          \
  m_time_t _start_time = m_time_ms();                \
  while (_UART_NOT_READY) {                          \
    if (m_time_ms() - _start_time > _UART_TIMEOUT) { \
      return x;                                      \
    }                                                \
    __NOP();                                         \
  }
#endif

#if !_UART_USE_ALLOC
static char *get_buffer(UART_HandleTypeDef *huart_get) {
  static char send_buff[_UART_SEND_BUFFER_SIZE];  // 发送缓冲区
  static UART_HandleTypeDef *huart = NULL;
  if (huart) {
    _WAIT_UART_READY(NULL);
  }
  huart = huart_get;
  return send_buff;
}
#else   // !_UART_USE_ALLOC
static char *get_buffer(UART_HandleTypeDef *huart_get) {
  static char *send_ptrs[_UART_MAX_ALLOC_NUM] = {NULL};
  static UART_HandleTypeDef *send_huarts[_UART_MAX_ALLOC_NUM] = {NULL};
  UART_HandleTypeDef *huart;
  char *buf_avail = NULL;
  do {
    for (uint8_t i = 0; i < _UART_MAX_ALLOC_NUM; i++) {
      huart = send_huarts[i];
      if (huart) {
        if ((!_UART_NOT_READY) ||
            (huart->pTxBuffPtr && huart->pTxBuffPtr - (uint8_t *)send_ptrs[i] >
                                      _UART_SEND_BUFFER_SIZE)) {
          if (huart->Instance == huart_get->Instance && !buf_avail) {
            buf_avail = send_ptrs[i];
            continue;
          };
          _dfree(send_ptrs[i]);
          send_ptrs[i] = NULL;
          send_huarts[i] = NULL;
        }
      }
    }
    if (buf_avail) return buf_avail;
    for (uint8_t i = 0; i < _UART_MAX_ALLOC_NUM; i++) {
      if (!send_ptrs[i]) {
        m_alloc(send_ptrs[i], _UART_SEND_BUFFER_SIZE);
        send_huarts[i] = huart_get;
        return (char *)send_ptrs[i];
      }
    }
  } while (_UART_ALLOC_BLOCK);
  return NULL;
}
#endif  // !_UART_USE_ALLOC

#if 0
void uart_memcpy(void *dst, const void *src, uint16_t len) {
  uint8_t *dst8 = (uint8_t *)dst;
  uint8_t *src8 = (uint8_t *)src;
  while (len--) {
    *dst8++ = *src8++;
  }
}
#else
#define uart_memcpy memcpy
#endif

/**
 * @brief 向指定串口发送格式化字符串
 * @param  huart            目标串口
 * @param  fmt              类似printf的格式化字符串
 * @retval 发送的字节数
 */
int printft(UART_HandleTypeDef *huart, char *fmt, ...) {
  char *sendBuffP = get_buffer(huart);
  if (sendBuffP == NULL) return -1;
  va_list ap;         // typedef char *va_list
  va_start(ap, fmt);  // 找到第一个可变形参的地址赋给ap
  int sendLen = vsnprintf(sendBuffP, _UART_SEND_BUFFER_SIZE, fmt, ap);
  va_end(ap);
  if (Uart_Send(huart, (uint8_t *)sendBuffP, sendLen) < 0) return -1;
  return sendLen;
}

/**
 * @brief 等待串口发送完成
 */
void printft_flush(UART_HandleTypeDef *huart) {
  while (_UART_NOT_READY) {
    __NOP();
  }
}

/**
 * @brief 打印十六进制数组
 */
void print_hex(const char *text, uint8_t *data, uint16_t len) {
  uint16_t i;
  printf("%s [ ", text);
  for (i = 0; i < len; i++) {
    printf("%02X ", data[i]);
  }
  printf("]\r\n");
}

/**
 * @brief 串口发送数据
 * @param  huart            目标串口
 * @param  data             数据指针
 * @param  len              数据长度
 */
int Uart_Send(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len) {
  if (len > 0) {
    _WAIT_UART_READY(-1);
#if _UART_DCACHE_COMPATIBLE
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)data, len);
#endif
#if _PRINTF_USE_DMA & _UART_ENABLE_DMA
    HAL_UART_Transmit_DMA(huart, data, len);
#elif _PRINTF_USE_IT
    HAL_UART_Transmit_IT(huart, data, len);
#else
    HAL_UART_Transmit(huart, data, len, 0xFFFF);
#endif
  }
  return 0;
}

int Uart_Send_Buffered(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len) {
  if (len > 0) {
    uint8_t *sendBuffP = (uint8_t *)get_buffer(huart);
    if (sendBuffP == NULL) return -1;
    uart_memcpy(sendBuffP, data, len);
    _WAIT_UART_READY(-1);
#if _UART_DCACHE_COMPATIBLE
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)sendBuffP, len);
#endif
#if _PRINTF_USE_DMA & _UART_ENABLE_DMA
    HAL_UART_Transmit_DMA(huart, sendBuffP, len);
#elif _PRINTF_USE_IT
    HAL_UART_Transmit_IT(huart, sendBuffP, len);
#else
    HAL_UART_Transmit(huart, sendBuffP, len, 0xFFFF);
#endif
  }
  return 0;
}

void Uart_Putchar(UART_HandleTypeDef *huart, uint8_t data) {
  while (_UART_NOT_READY) {  // 检查串口是否打开
    __NOP();
  }
  HAL_UART_Transmit(huart, &data, 1, 0xFFFF);
}

void Uart_Puts(UART_HandleTypeDef *huart, char *str) {
  while (_UART_NOT_READY) {  // 检查串口是否打开
    __NOP();
  }
  HAL_UART_Transmit(huart, (uint8_t *)str, strlen(str), 0xFFFF);
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
  if (len > 0) {
    if (_UART_NOT_READY) return -1;
#if _PRINTF_USE_DMA & _UART_ENABLE_DMA
    HAL_UART_Transmit_DMA(huart, data, len);
#elif _PRINTF_USE_IT
    HAL_UART_Transmit_IT(huart, data, len);
#else
    HAL_UART_Transmit(huart, data, len, 0xFFFF);
#endif
  }
  return 0;
}

#if !_UART_CALLBACK_IN_IRQ
void (*swRxCallback)(char *data, uint16_t len) = NULL;
char *swRxData = NULL;
uint16_t swRxLen = 0;

/**
 * @brief 轮询以在主循环中响应串口接收完成回调
 * @note 若轮询频率小于接收频率, 回调请求会被覆盖
 */
void Uart_Callback_Check(void) {
  if (swRxCallback != NULL) {
    swRxCallback(swRxData, swRxLen);
    swRxCallback = NULL;
  }
}
#endif

static uart_ctrl_t *uart_ctrl_list[MAX_UART_NUM] = {NULL};

/**
 * @brief 串口中断接收初始化
 * @param  ctrl             结构体指针
 * @param  huart            目标串口
 * @param  rxTimeout        接收超时时间
 * @param  rxCallback       接收完成回调函数
 * @param  cbkInIRQ         回调函数是否在中断中执行
 */
void Uart_Init(uart_ctrl_t *ctrl, UART_HandleTypeDef *huart, m_time_t rxTimeout,
               void (*rxCallback)(char *data, uint16_t len), uint8_t cbkInIRQ) {
  ctrl->rxTimeout = rxTimeout;
  ctrl->finished = 0;
  ctrl->rxIdx = 0;
  ctrl->len = 0;
  ctrl->huart = huart;
  ctrl->rxCallback = rxCallback;
  ctrl->cbkInIRQ = cbkInIRQ;
  HAL_UART_Receive_IT(huart, ctrl->rxBuf, 1);
  // add to list
  for (int i = 0; i < MAX_UART_NUM; i++) {
    if (uart_ctrl_list[i] == NULL) {
      uart_ctrl_list[i] = ctrl;
      break;
    }
  }
}

/**
 * @brief 串口接收中断处理，在函数HAL_UART_RxCpltCallback中调用
 */
inline void Uart_Rx_Process(UART_HandleTypeDef *huart) {
  uart_ctrl_t *item = uart_ctrl_list[0];
  while (item) {
    if (item->huart == huart) {
      item->rxTime = m_time_ms();
      if (++item->rxIdx >= _UART_RECV_BUFFER_SIZE - 1) {
        uart_memcpy(item->buffer, item->rxBuf, item->rxIdx);
        item->len = item->rxIdx;
        item->buffer[item->rxIdx] = 0;
        item->finished = 1;
        item->rxIdx = 0;
        if (item->rxCallback) {
          if (item->cbkInIRQ) {
            item->rxCallback((char *)item->buffer, item->len);
          } else {
            swRxCallback = item->rxCallback;
            swRxData = (char *)item->buffer;
            swRxLen = item->len;
          }
        }
      }
      HAL_UART_Receive_IT(item->huart, item->rxBuf + item->rxIdx, 1);
      return;
    }
    item++;
  }
}

/**
 * @brief 串口中断接收超时判断，在调度器中调用
 */
void Uart_Timeout_Check(void) {
  uart_ctrl_t *item = uart_ctrl_list[0];
  while (item) {
    if (item->rxIdx && m_time_ms() - item->rxTime > item->rxTimeout) {
      HAL_UART_AbortReceive_IT(item->huart);
      uart_memcpy(item->buffer, item->rxBuf, item->rxIdx);
      item->len = item->rxIdx;
      item->buffer[item->rxIdx] = 0;
      item->finished = 1;
      item->rxIdx = 0;
      HAL_UART_Receive_IT(item->huart, item->rxBuf, 1);
      if (item->rxCallback) {
        item->rxCallback((char *)item->buffer, item->len);
      }
    }
    item++;
  }
}

#if _UART_ENABLE_DMA

uart_dma_ctrl_t *dma_ctrl_list[MAX_UART_NUM] = {NULL};

/**
 * @brief 串口DMA接收初始化
 * @param  ctrl             结构体指针
 * @param  huart            目标串口
 * @param  rxCallback       接收完成回调函数
 * @param  cbkInIRQ         回调函数是否在中断中执行
 */
void Uart_DMA_Init(uart_dma_ctrl_t *ctrl, UART_HandleTypeDef *huart,
                   void (*rxCallback)(char *data, uint16_t len),
                   uint8_t cbkInIRQ) {
  ctrl->huart = huart;
  ctrl->finished = 0;
  ctrl->len = 0;
  ctrl->buffer[0] = 0;
  ctrl->rxCallback = rxCallback;
  ctrl->cbkInIRQ = cbkInIRQ;
  HAL_UARTEx_ReceiveToIdle_DMA(huart, ctrl->rxBuf, _UART_RECV_BUFFER_SIZE - 1);
  for (int i = 0; i < MAX_UART_NUM; i++) {
    if (dma_ctrl_list[i] == NULL) {
      dma_ctrl_list[i] = ctrl;
      break;
    }
  }
}

/**
 * @brief 串口DMA接收处理，在函数HAL_UARTEx_RxEventCallback中调用
 */
inline void Uart_DMA_Process(UART_HandleTypeDef *huart, uint16_t Size) {
  uart_dma_ctrl_t *item = dma_ctrl_list[0];
  while (item) {
    if (item->huart == huart) {
#if _UART_DCACHE_COMPATIBLE
      SCB_CleanInvalidateDCache_by_Addr((uint32_t *)item->rxBuf,
                                        ((Size + 31) / 32) * 32);
#endif
      uart_memcpy(item->buffer, item->rxBuf, Size);
      item->len = Size;
      item->buffer[Size] = 0;
      item->finished = 1;
      HAL_UARTEx_ReceiveToIdle_DMA(item->huart, item->rxBuf,
                                   _UART_RECV_BUFFER_SIZE - 1);
      if (item->rxCallback) {
        if (item->cbkInIRQ) {
          item->rxCallback((char *)item->buffer, item->len);
        } else {
          swRxCallback = item->rxCallback;
          swRxData = (char *)item->buffer;
          swRxLen = item->len;
        }
      }
      return;
    }
    item++;
  }
}
#endif  // _UART_ENABLE_DMA

uint8_t uart_error_state = 0;  // 串口错误状态

/**
 * @brief 串口错误中断处理，在函数HAL_UART_ErrorCallback中调用
 */
inline void Uart_Error_Process(UART_HandleTypeDef *huart) {
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
  uart_dma_ctrl_t *item = dma_ctrl_list[0];
  while (item) {
    if (item->huart->Instance == huart->Instance) {
      HAL_UART_DMAStop(item->huart);
      HAL_UARTEx_ReceiveToIdle_DMA(item->huart, item->rxBuf,
                                   _UART_RECV_BUFFER_SIZE - 1);
      // break;
    }
    item++;
  }
#endif
  // 自动重启中断
  uart_ctrl_t *item2 = uart_ctrl_list[0];
  while (item2) {
    if (item2->huart->Instance == huart->Instance) {
      HAL_UART_AbortReceive_IT(item2->huart);
      HAL_UART_Receive_IT(item2->huart, item2->rxBuf, 1);
      // break;
    }
    item2++;
  }
}

#if _UART_REWRITE_HANLDER  // 重写HAL中断处理函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  Uart_Rx_Process(huart);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  Uart_Error_Process(huart);
}
#if _UART_ENABLE_DMA
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  Uart_DMA_Process(huart, Size);
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

__attribute__((constructor(255))) // 自动Hook
void Hook_CDC_Init(void) {
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
/**
 * @brief USB CDC 发送格式化字符串
 */
int printfcdc(char *fmt, ...) {
  if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -3;
  _cdc_start_time = m_time_ms();
  while (hcdc->TxState != 0) {  // 检查上一次发送是否完成
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return -2;
    if (m_time_ms() - _cdc_start_time > _UART_CDC_TIMEOUT) return -1;
  }
  va_list ap;
  va_start(ap, fmt);
  int sendLen = vsnprintf((char *)UserTxBufferFS, APP_TX_DATA_SIZE, fmt, ap);
  va_end(ap);
  if (sendLen > 0) {
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, sendLen);
    USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  }
  return sendLen;
}

/**
 * @brief USB CDC 等待发送完成
 */
void printfcdc_flush(void) {
  _cdc_start_time = m_time_ms();
  while (hcdc->TxState != 0) {
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return;
    if (m_time_ms() - _cdc_start_time > _UART_CDC_TIMEOUT) return;
  }
}

/**
 * @brief USB CDC 发送数据
 */
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
/**
 * @brief USB CDC 阻塞等待连接
 */
void CDC_Wait_Connect(int timeout_ms) {
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

/**
 * @brief 注册USB CDC接收回调函数
 * @param callback 回调函数
 */
void CDC_Register_Callback(void (*callback)(char *buf, uint16_t len),
                           uint8_t cbkInIRQ) {
  usb_cdc.rxCallback = callback;
  usb_cdc.cbkInIRQ = cbkInIRQ;
}

/**
 * @brief USB是否已连接
 */
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

void Vofa_SendCDC(void) {
  if (vofa_index == 0) return;
  Vofa_AddSeq((float *)&vofa_endbit, 1);
#if _UART_ENABLE_CDC
  CDC_Send((uint8_t *)VOFA_Buffer, vofa_index * sizeof(float));
#endif
  vofa_index = 0;
}

#endif  // _VOFA_ENABLE

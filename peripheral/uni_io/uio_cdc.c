/**
 * @file uio_cdc.c
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#include "uio_cdc.h"

#if UIO_CFG_ENABLE_CDC

#include <string.h>

#include "lwprintf.h"

static struct {                         // CDC型UART控制结构体
    lfifo_t txFifo;                     // 发送缓冲区
    lfifo_t rxFifo;                     // 接收缓冲区
    void (*rxCallback)(lfifo_t* fifo);  // 接收完成回调函数
    uint8_t cbkInIRQ;                   // 回调函数是否在中断中执行
} usb_cdc;

#if UIO_CFG_CDC_USE_CUBEMX
#include "usbd_cdc_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];
__IO static USBD_CDC_HandleTypeDef* hcdc = NULL;

static int8_t Hook_CDC_Init_FS(void) {
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
    hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
    return (USBD_OK);
}

static int8_t Hook_CDC_DeInit_FS(void) {
    return (USBD_OK);
}

static int8_t Hook_CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length) {
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

static int8_t Hook_CDC_Receive_FS(uint8_t* Buf, uint32_t* Len) {
    LFifo_Write(&usb_cdc.rxFifo, Buf, *Len);
    if (usb_cdc.cbkInIRQ && usb_cdc.rxCallback != NULL) {
        usb_cdc.rxCallback(&usb_cdc.rxFifo);
    }
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return (USBD_OK);
}

static void start_next_cdc_transfer(uint8_t force) {
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)
        return;
    if (!force && hcdc->TxState != 0)
        return;
    if (LFifo_IsEmpty(&usb_cdc.txFifo))
        return;
    size_t rd = LFifo_Read(&usb_cdc.txFifo, UserTxBufferFS, APP_TX_DATA_SIZE);
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, rd);
    USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}

static int8_t Hook_CDC_TransmitCplt_FS(uint8_t* Buf, uint32_t* Len,
                                       uint8_t epnum) {
    // start_next_cdc_transfer(1);
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
    USBD_Interface_fops_FS.TransmitCplt = Hook_CDC_TransmitCplt_FS;
}

static inline void cdc_start_transfers(void) {
    start_next_cdc_transfer(0);
}

static inline uint8_t cdc_connected(void) {
    return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
}

static inline uint8_t cdc_idle(void) {
    return hcdc->TxState == 0;
}

#elif UIO_CFG_CDC_USE_CHERRY
#include "cdc_acm_app.h"

void cdc_acm_data_recv_callback(uint8_t* buf, uint32_t len) {
    LFifo_Write(&usb_cdc.rxFifo, buf, len);
    if (usb_cdc.cbkInIRQ && usb_cdc.rxCallback != NULL) {
        usb_cdc.rxCallback(&usb_cdc.rxFifo);
    }
}

void cdc_acm_data_send_cplt_callback(void) {
    if (!cdc_acm_connected())
        return;
    if (LFifo_IsEmpty(&usb_cdc.txFifo))
        return;
    uint32_t len = 0;
    uint8_t* ptr = cdc_acm_data_send_raw_acquire(&len);
    if (!ptr)
        return;
    len = LFifo_Read(&usb_cdc.txFifo, ptr, len);
    cdc_acm_data_send_raw_commit(len);
}

static inline void cdc_start_transfers(void) {
    if (!cdc_acm_idle())
        return;
    if (LFifo_IsEmpty(&usb_cdc.txFifo))
        return;
    uint32_t len = 0;
    uint8_t* ptr = cdc_acm_data_send_raw_acquire(&len);
    if (!ptr)
        return;
    len = LFifo_Read(&usb_cdc.txFifo, ptr, len);
    cdc_acm_data_send_raw_commit(len);
}

static inline uint8_t cdc_connected(void) {
    return cdc_acm_connected();
}

static inline uint8_t cdc_idle(void) {
    return cdc_acm_idle();
}
#else
#error "Please select a USB CDC implementation"
#endif

lfifo_t* cdc_fifo_init(uint8_t* txBuf, size_t txBufSize, uint8_t* rxBuf,
                       size_t rxBufSize, void (*rxCallback)(lfifo_t* fifo),
                       uint8_t cbkInIRQ) {
    if (!txBuf) {
        if (LFifo_Init(&usb_cdc.txFifo, txBufSize) != 0)
            return NULL;
    } else {
        LFifo_AssignBuf(&usb_cdc.txFifo, txBuf, txBufSize);
    }
    if (!rxBuf) {
        if (LFifo_Init(&usb_cdc.rxFifo, rxBufSize) != 0)
            return NULL;
    } else {
        LFifo_AssignBuf(&usb_cdc.rxFifo, rxBuf, rxBufSize);
    }
    usb_cdc.rxCallback = rxCallback;
    usb_cdc.cbkInIRQ = cbkInIRQ;
    return &usb_cdc.rxFifo;
}

void cdc_check_callback(void) {
    if (usb_cdc.rxCallback == NULL)
        return;
    if (!LFifo_IsEmpty(&usb_cdc.rxFifo)) {
        usb_cdc.rxCallback(&usb_cdc.rxFifo);
    }
}

void cdc_write(uint8_t* buf, size_t len) {
    if (!cdc_connected())
        return;
    size_t wr;
    wr = LFifo_Write(&usb_cdc.txFifo, buf, len);
    len -= wr;
    buf += wr;
    cdc_start_transfers();
    if (len == 0)
        return;
    if (UIO_CFG_CDC_TIMEOUT <= 0)
        return;
    m_time_t _cdc_start_time = m_time_ms();
    while (len) {
        // m_delay_ms(1);
        wr = LFifo_Write(&usb_cdc.txFifo, buf, len);
        len -= wr;
        buf += wr;
        cdc_start_transfers();
        if (m_time_ms() - _cdc_start_time > UIO_CFG_CDC_TIMEOUT)
            return;
    }
}

static int cdc_lwprintf_fn(int ch, lwprintf_t* lwobj) {
    if (ch == '\0') {
        cdc_start_transfers();
        return 0;
    }
    if (!LFifo_WriteByte(&usb_cdc.txFifo, ch))
        return ch;
    return -1;
}

int cdc_printf(char* fmt, ...) {
    static lwprintf_t lw_cdc = {.out_fn = cdc_lwprintf_fn, .arg = NULL};
    va_list ap;
    va_start(ap, fmt);
    int sendLen = lwprintf_vprintf_ex(&lw_cdc, fmt, ap);
    va_end(ap);
    return sendLen;
}

void cdc_flush(void) {
    m_time_t _cdc_start_time = m_time_ms();
    while (!cdc_idle() || !LFifo_IsEmpty(&usb_cdc.txFifo)) {
        if (!cdc_connected())
            return;
        m_delay_ms(1);
        if (m_time_ms() - _cdc_start_time > UIO_CFG_CDC_TIMEOUT)
            return;
    }
}

void cdc_wait_for_connect(int timeout_ms) {
    m_time_t _cdc_start_time = m_time_ms();
    while (!cdc_connected()) {
        if (timeout_ms > 0 && m_time_ms() - _cdc_start_time > timeout_ms)
            return;
        m_delay_ms(1);
    }
}

uint8_t cdc_is_connected(void) {
    return cdc_connected();
}
#endif  // UIO_CFG_ENABLE_CDC

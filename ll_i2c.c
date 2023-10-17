/**
 * @file ll_i2c.c
 * @brief LL I2C interface implementation
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-10-15
 *
 * THINK DIFFERENTLY
 */

#include "ll_i2c.h"

#define _INLINE __attribute__((always_inline))
#if _LL_IIC_CONVERT_SLAVEADDR
#define SLAVEADDR(addr) ((addr) << 1)
#else
#define SLAVEADDR(addr) (addr)
#endif

#if _LL_IIC_USE_IT
#include "FreeRTOS.h"
#include "semphr.h"
static SemaphoreHandle_t xSemphr;
static SemaphoreHandle_t xMutex;
#endif

static I2C_TypeDef* I2Cx = _LL_IIC_INSTANCE;

void LL_IIC_Init(void) {
  // Hardware init procedure should be done by CubeMX

#if _LL_IIC_USE_IT  // 创建信号量
  xSemphr = xSemaphoreCreateBinary();
  xMutex = xSemaphoreCreateMutex();
#endif
}

#if !_LL_IIC_USE_IT

typedef enum {
  I2C_TRANSMITTER,
  I2C_RECEIVER,
  I2C_RECEIVER_RESTART
} i2c_direction_t;

static bool I2Cx_StartTransmission(i2c_direction_t Direction, uint8_t SlaveAddr,
                                   uint8_t TransferSize) {
  uint32_t Timeout = _LL_IIC_BYTE_TIMEOUT_MS;
  uint32_t Request;

  switch (Direction) {
    case I2C_TRANSMITTER:
      Request = LL_I2C_GENERATE_START_WRITE;
      break;
    case I2C_RECEIVER:
      Request = LL_I2C_GENERATE_START_READ;
      break;
    case I2C_RECEIVER_RESTART:
      Request = LL_I2C_GENERATE_RESTART_7BIT_READ;
      break;
    default:
      return false;
  }

  LL_I2C_HandleTransfer(I2Cx, SlaveAddr, LL_I2C_ADDRSLAVE_7BIT, TransferSize,
                        LL_I2C_MODE_SOFTEND, Request);

  while (!LL_I2C_IsActiveFlag_TXIS(I2Cx) && !LL_I2C_IsActiveFlag_RXNE(I2Cx)) {
    if (LL_I2C_IsActiveFlag_NACK(I2Cx)) {
      LL_I2C_ClearFlag_NACK(I2Cx);
      return false;
    }
    if (LL_SYSTICK_IsActiveCounterFlag()) {
      if (Timeout-- == 0) {
        return false;
      }
    }
  }

  return true;
}

static bool I2Cx_SendByte(uint8_t byte) {
  uint32_t Timeout = _LL_IIC_BYTE_TIMEOUT_MS;

  LL_I2C_TransmitData8(I2Cx, byte);
  while (!LL_I2C_IsActiveFlag_TXIS(I2Cx) && !LL_I2C_IsActiveFlag_TC(I2Cx)) {
    /* Break if ACK failed */
    if (LL_I2C_IsActiveFlag_NACK(I2Cx)) {
      LL_I2C_ClearFlag_NACK(I2Cx);
      return false;
    }
    if (LL_SYSTICK_IsActiveCounterFlag()) {
      if (Timeout-- == 0) {
        return false;
      }
    }
  }

  return true;
}

static bool I2Cx_ReceiveByte(uint8_t* Byte) {
  uint32_t Timeout = _LL_IIC_BYTE_TIMEOUT_MS;

  if (!Byte) return false;

  while (!LL_I2C_IsActiveFlag_RXNE(I2Cx) && !LL_I2C_IsActiveFlag_TC(I2Cx)) {
    if (LL_SYSTICK_IsActiveCounterFlag()) {
      if (Timeout-- == 0) {
        return false;
      }
    }
  }

  *Byte = LL_I2C_ReceiveData8(I2Cx);

  return true;
}

static void I2Cx_StopTransmission(void) {
  /* Send STOP bit */
  LL_I2C_GenerateStopCondition(I2Cx);
}

bool I2Cx_ReadData(uint8_t SlaveAddr, uint16_t ReadAddr, uint8_t AddrLen,
                   uint8_t* pBuffer, uint8_t NumBytesToRead) {
  if (!pBuffer || !NumBytesToRead) {
    return false;
  }

  SlaveAddr = SLAVEADDR(SlaveAddr);

  if (AddrLen) {
    if (!I2Cx_StartTransmission(I2C_TRANSMITTER, SlaveAddr, AddrLen)) {
      return false;
    }
    if (AddrLen == 2 && !I2Cx_SendByte(ReadAddr >> 8)) {
      I2Cx_StopTransmission();
      return false;
    }
    if (!I2Cx_SendByte(ReadAddr & 0xFF)) {
      I2Cx_StopTransmission();
      return false;
    }
  }

  if (!I2Cx_StartTransmission(AddrLen ? I2C_RECEIVER_RESTART : I2C_RECEIVER,
                              SlaveAddr, NumBytesToRead)) {
    return false;
  }

  while (NumBytesToRead--) {
    if (!I2Cx_ReceiveByte(pBuffer++)) {
      I2Cx_StopTransmission();
      return false;
    }
    if (NumBytesToRead == 0) {
      /* Send STOP after the last byte */
      I2Cx_StopTransmission();
    }
  }

  return true;
}

bool I2Cx_WriteData(uint8_t SlaveAddr, uint16_t WriteAddr, uint8_t AddrLen,
                    uint8_t* pBuffer, uint8_t NumBytesToWrite) {
  bool Result = true;

  if (!pBuffer || !NumBytesToWrite) {
    return false;
  }

  SlaveAddr = SLAVEADDR(SlaveAddr);

  if (!I2Cx_StartTransmission(I2C_TRANSMITTER, SlaveAddr,
                              AddrLen + NumBytesToWrite)) {
    return false;
  }

  if (AddrLen == 2 && !I2Cx_SendByte(WriteAddr >> 8)) {
    I2Cx_StopTransmission();
    return false;
  }
  if (AddrLen >= 1 && !I2Cx_SendByte(WriteAddr & 0xFF)) {
    I2Cx_StopTransmission();
    return false;
  }

  while (NumBytesToWrite--) {
    if (!I2Cx_SendByte(*pBuffer++)) {
      Result = false;
      break;
    }
  }
  I2Cx_StopTransmission();

  return Result;
}

#else /* _HAL_I2C_USE_IT */
typedef enum { RESTART, CONTINUE, FINISHED, FAILED } i2c_state_t;

typedef enum { I2C_TRANSMITTER, I2C_RECEIVER } i2c_direction_t;

static struct IfaceStruct {
  uint8_t SlaveAddr;
  i2c_direction_t Direction;
  struct {
    uint16_t Len;
    uint16_t Idx;
    uint8_t* Buf;
  } Rx;
  struct Tx {
    uint16_t Len;
    uint16_t Idx;
    uint8_t* Buf;
  } Tx;
  struct {
    uint16_t Val;
    uint8_t Len;
    uint8_t Idx;
  } Addr;
} Iface;

static SemaphoreHandle_t xSemphr;
static SemaphoreHandle_t xMutex;

static void ResetIface(void) {
  Iface.Addr.Idx = 0;
  Iface.Addr.Len = 0;

  Iface.Rx.Idx = 0;
  Iface.Rx.Len = 0;
  Iface.Rx.Buf = NULL;

  Iface.Tx.Idx = 0;
  Iface.Tx.Len = 0;
  Iface.Tx.Buf = NULL;
}

static bool I2Cx_StartTransmission(i2c_direction_t Direction, uint8_t SlaveAddr,
                                   uint8_t TransferSize) {
  Iface.SlaveAddr = SLAVEADDR(SlaveAddr);
  Iface.Direction = Direction;

  LL_I2C_EnableIT_TX(I2Cx);
  LL_I2C_EnableIT_TC(I2Cx);

  LL_I2C_HandleTransfer(I2Cx, Iface.SlaveAddr, LL_I2C_ADDRSLAVE_7BIT,
                        TransferSize, LL_I2C_MODE_SOFTEND,
                        Direction == I2C_TRANSMITTER
                            ? LL_I2C_GENERATE_START_WRITE
                            : LL_I2C_GENERATE_START_READ);

  return true;
}

static void I2Cx_StopTransmissionISR(bool GenerateStop) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  /* Send STOP bit if required */
  if (GenerateStop) {
    LL_I2C_GenerateStopCondition(I2Cx);
  }

  xSemaphoreGiveFromISR(xSemphr, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

bool I2Cx_ReadData(uint8_t SlaveAddr, uint16_t ReadAddr, uint8_t AddrLen,
                   uint8_t* pBuffer, uint8_t NumBytesToRead) {
  bool Result = false;

  if (xSemaphoreTake(xMutex, 5000) != pdTRUE) {
    return false;
  }

  if (!pBuffer || !NumBytesToRead) {
    xSemaphoreGive(xMutex);
    return false;
  }

  ResetIface();

  Iface.Addr.Val = ReadAddr;
  Iface.Addr.Len = AddrLen;

  Iface.Rx.Buf = pBuffer;
  Iface.Rx.Len = NumBytesToRead;

  if (!I2Cx_StartTransmission(I2C_TRANSMITTER, SlaveAddr,
                              AddrLen ? AddrLen : NumBytesToRead)) {
    xSemaphoreGive(xMutex);
    return false;
  }

  /* Block until end of transaction */
  if (xSemaphoreTake(xSemphr, I2C_TRANSFER_TIMEOUT_MS(
                                  AddrLen + NumBytesToRead)) == pdTRUE) {
    Result = (Iface.Rx.Idx == Iface.Rx.Len);
  } else {
    if (LL_I2C_IsActiveFlag_NACK(I2Cx)) {
      /* STOP is sent automatically if NACK was received */
      LL_I2C_ClearFlag_NACK(I2Cx);
    }
  }

  xSemaphoreGive(xMutex);
  return Result;
}

bool I2Cx_WriteData(uint8_t SlaveAddr, uint16_t WriteAddr, uint8_t AddrLen,
                    uint8_t* pBuffer, uint8_t NumBytesToWrite) {
  bool Result = false;

  if (xSemaphoreTake(xMutex, 5000) != pdTRUE) {
    return false;
  }

  if (!pBuffer || !NumBytesToWrite) {
    xSemaphoreGive(xMutex);
    return false;
  }

  ResetIface();

  Iface.Addr.Val = WriteAddr;
  Iface.Addr.Len = AddrLen;

  Iface.Tx.Buf = pBuffer;
  Iface.Tx.Len = NumBytesToWrite;

  if (!I2Cx_StartTransmission(I2C_TRANSMITTER, SlaveAddr,
                              AddrLen + NumBytesToWrite)) {
    xSemaphoreGive(xMutex);
    return false;
  }

  /* Block until end of transaction */
  if (xSemaphoreTake(xSemphr, I2C_TRANSFER_TIMEOUT_MS(
                                  AddrLen + NumBytesToWrite)) == pdTRUE) {
    Result = (Iface.Tx.Idx == Iface.Tx.Len);
  } else {
    if (LL_I2C_IsActiveFlag_NACK(I2Cx)) {
      /* STOP is sent automatically if NACK was received */
      LL_I2C_ClearFlag_NACK(I2Cx);
    }
  }

  xSemaphoreGive(xMutex);
  return Result;
}

static i2c_state_t I2Cx_SendByte_Callback(void) {
  if (Iface.Addr.Len && (Iface.Addr.Idx < Iface.Addr.Len)) {
    if (Iface.Addr.Len == 2 && Iface.Addr.Idx == 0) {
      /* MSB */
      LL_I2C_TransmitData8(I2Cx, Iface.Addr.Val >> 8);
    } else {
      /* LSB */
      LL_I2C_TransmitData8(I2Cx, Iface.Addr.Val & 0xFF);
    }
    /* Address sent, disable TXE and wait TC */
    if (++Iface.Addr.Idx == Iface.Addr.Len) {
      /* Generate Restart only ro read requests */
      if (Iface.Rx.Len) {
        /* Address sent, disable TXE and wait TC */
        LL_I2C_DisableIT_TX(I2Cx);
        return RESTART;
      }
    }
  } else if (Iface.Tx.Len && (Iface.Tx.Idx < Iface.Tx.Len)) {
    LL_I2C_TransmitData8(I2Cx, Iface.Tx.Buf[Iface.Tx.Idx++]);
    if (Iface.Tx.Idx == Iface.Tx.Len) {
      /* All data sent, disable TXE and wait TC */
      LL_I2C_DisableIT_TX(I2Cx);
      return FINISHED;
    }
  } else {
    LL_I2C_ClearFlag_TXE(I2Cx);
  }
  return CONTINUE;
}

static void I2Cx_ReceiveByte_Callback(uint8_t byte) {
  Iface.Rx.Buf[Iface.Rx.Idx++] = byte;
  if (Iface.Rx.Idx == Iface.Rx.Len - 1) {
    /* Send STOP after the last byte */
    LL_I2C_GenerateStopCondition(I2Cx);
  } else if (Iface.Rx.Idx == Iface.Rx.Len) {
    /* All data received.
     * Send STOP explicitly for a sigle byte transactions */
    I2Cx_StopTransmissionISR((Iface.Rx.Len == 1));
  }
}

static void EV_Handler(void) {
  /* TXE set, TC cleared */
  if (LL_I2C_IsActiveFlag_TXE(I2Cx) && !LL_I2C_IsActiveFlag_TC(I2Cx)) {
    State = I2Cx_SendByte_Callback();
  }
  /* TC set */
  if (LL_I2C_IsActiveFlag_TC(I2Cx)) {
    if (State == RESTART) {
      Iface.Direction = Iface.Tx.Len > 0 ? I2C_TRANSMITTER : I2C_RECEIVER;

      LL_I2C_DisableIT_TX(I2Cx);
      LL_I2C_EnableIT_RX(I2Cx);

      LL_I2C_HandleTransfer(I2Cx, Iface.SlaveAddr, LL_I2C_ADDRSLAVE_7BIT,
                            Iface.Rx.Len, LL_I2C_MODE_SOFTEND,
                            LL_I2C_GENERATE_RESTART_7BIT_READ);

      State = CONTINUE;
    } else if (State == FINISHED) {
      I2Cx_StopTransmissionISR(true);
    }
  }
  /* RXNE set */
  if (LL_I2C_IsActiveFlag_RXNE(I2Cx)) {
    I2Cx_ReceiveByte_Callback(LL_I2C_ReceiveData8(I2Cx));
  }
}

static void ER_Handler(void) {
  /* I2C Bus error */
  if (LL_I2C_IsActiveFlag_BERR(I2Cx)) {
    /* Clear BERR flag */
    LL_I2C_ClearFlag_BERR(I2Cx);
  }

  /* I2C Arbitration Lost */
  if (LL_I2C_IsActiveFlag_ARLO(I2Cx)) {
    /* Clear ARLO flag */
    LL_I2C_ClearFlag_ARLO(I2Cx);
  }

  /* I2C Acknowledge failure */
  if (LL_I2C_IsActiveFlag_NACK(I2Cx)) {
    /* Special case: NACK was received with the last byte */
    if (Iface.Tx.Len && (Iface.Tx.Idx == Iface.Tx.Len)) {
      /* Set `Iface.Tx.Idx != Iface.Tx.Len` in order
       * to fail transaction result */
      Iface.Tx.Idx--;
    }
    /* STOP is sent automatically if NACK was received */
    I2Cx_StopTransmissionISR(false);
    /* Clear AF flag */
    LL_I2C_ClearFlag_NACK(I2Cx);
  }

  /* I2C Over-Run/Under-Run */
  if (LL_I2C_IsActiveFlag_OVR(I2Cx)) {
    /* Clear OVR flag */
    LL_I2C_ClearFlag_OVR(I2Cx);
  }
}

/* EV handlers */

// #define BUILD_EV_IRQ_HANDLER(IICI) IICI##_EV_IRQHandler(void)
// #define _BUILD_EV_IRQ_HANDLER(IICI) BUILD_EV_IRQ_HANDLER(IICI)
//
// #define BUILD_ER_IRQ_HANDLER(IICI) IICI##_ER_IRQHandler(void)
// #define _BUILD_ER_IRQ_HANDLER(IICI) BUILD_ER_IRQ_HANDLER(IICI)
//
// #define BUILD_IRQ_HANDLER(IICI) IICI##_IRQHandler(void)
// #define _BUILD_IRQ_HANDLER(IICI) BUILD_IRQ_HANDLER(IICI)
//
// void _BUILD_EV_IRQ_HANDLER(_LL_IIC_INSTANCE) { EV_Handler(); }
// void _BUILD_ER_IRQ_HANDLER(_LL_IIC_INSTANCE) { ER_Handler(); }
// void _BUILD_IRQ_HANDLER(_LL_IIC_INSTANCE) {
//   EV_Handler();
//   ER_Handler();
// }

_INLINE void LL_IIC_IRQHandler(void) {
  EV_Handler();
  ER_Handler();
}

_INLINE void LL_IIC_EV_IRQHandler(void) { EV_Handler(); }
_INLINE void LL_IIC_ER_IRQHandler(void) { ER_Handler(); }

#endif

_INLINE void LL_IIC_Write_Data(uint8_t data) { I2Cx_SendByte(data); }
_INLINE uint8_t LL_IIC_Read_Data(void) {
  uint8_t data;
  I2Cx_ReceiveByte(&data);
  return data;
}
_INLINE uint8_t LL_IIC_Read_8addr(uint8_t SlaveAddr, uint8_t RegAddr,
                                  uint8_t* pdata, uint8_t rcnt) {
  return I2Cx_ReadData(SlaveAddr, RegAddr, 1, pdata, rcnt);
}
_INLINE uint8_t LL_IIC_Read_16addr(uint8_t SlaveAddr, uint16_t RegAddr,
                                   uint8_t* pdata, uint8_t rcnt) {
  return I2Cx_ReadData(SlaveAddr, RegAddr, 2, pdata, rcnt);
}
_INLINE uint8_t LL_IIC_Write_8addr(uint8_t SlaveAddr, uint8_t RegAddr,
                                   uint8_t* pdata, uint8_t rcnt) {
  return I2Cx_WriteData(SlaveAddr, RegAddr, 1, pdata, rcnt);
}
_INLINE uint8_t LL_IIC_Write_16addr(uint8_t SlaveAddr, uint16_t RegAddr,
                                    uint8_t* pdata, uint8_t rcnt) {
  return I2Cx_WriteData(SlaveAddr, RegAddr, 2, pdata, rcnt);
}
uint8_t LL_IIC_Check_SlaveAddr(uint8_t SlaveAddr) {
  if (!I2Cx_StartTransmission(I2C_TRANSMITTER, SLAVEADDR(SlaveAddr), 1)) {
    return 0;
  }
  I2Cx_SendByte(0x00);
  I2Cx_StopTransmission();
  return 1;
}

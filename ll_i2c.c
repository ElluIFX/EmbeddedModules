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

typedef enum {
  I2C_TRANSMITTER,
  I2C_RECEIVER,
  I2C_RECEIVER_RESTART
} i2c_direction_t;

static void I2Cx_CheckError(I2C_TypeDef* I2Cx) {
  if (LL_I2C_IsActiveFlag_BERR(I2Cx) || LL_I2C_IsActiveFlag_ARLO(I2Cx) ||
      LL_I2C_IsActiveFlag_OVR(I2Cx)) {
    LL_I2C_ClearFlag_BERR(I2Cx);
    LL_I2C_ClearFlag_ARLO(I2Cx);
    LL_I2C_ClearFlag_OVR(I2Cx);
    LL_I2C_Disable(I2Cx);
    LL_I2C_Enable(I2Cx);
  }
}

static bool I2Cx_StartTransmission(I2C_TypeDef* I2Cx, i2c_direction_t Direction,
                                   uint8_t SlaveAddr, uint8_t TransferSize) {
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

static bool I2Cx_SendByte(I2C_TypeDef* I2Cx, uint8_t byte) {
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

static bool I2Cx_ReceiveByte(I2C_TypeDef* I2Cx, uint8_t* Byte) {
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

static void I2Cx_StopTransmission(I2C_TypeDef* I2Cx) {
  /* Send STOP bit */
  LL_I2C_GenerateStopCondition(I2Cx);
}

static bool I2Cx_ReadData(I2C_TypeDef* I2Cx, uint8_t SlaveAddr,
                          uint16_t ReadAddr, uint8_t AddrLen, uint8_t* pBuffer,
                          uint8_t NumBytesToRead) {
  if (!pBuffer || !NumBytesToRead) {
    return false;
  }

  SlaveAddr = SLAVEADDR(SlaveAddr);

  if (AddrLen) {
    if (!I2Cx_StartTransmission(I2Cx, I2C_TRANSMITTER, SlaveAddr, AddrLen)) {
      return false;
    }
    if (AddrLen == 2 && !I2Cx_SendByte(I2Cx, ReadAddr >> 8)) {
      I2Cx_StopTransmission(I2Cx);
      return false;
    }
    if (!I2Cx_SendByte(I2Cx, ReadAddr & 0xFF)) {
      I2Cx_StopTransmission(I2Cx);
      return false;
    }
  }

  if (!I2Cx_StartTransmission(I2Cx,
                              AddrLen ? I2C_RECEIVER_RESTART : I2C_RECEIVER,
                              SlaveAddr, NumBytesToRead)) {
    return false;
  }

  while (NumBytesToRead--) {
    if (!I2Cx_ReceiveByte(I2Cx, pBuffer++)) {
      I2Cx_StopTransmission(I2Cx);
      return false;
    }
    if (NumBytesToRead == 0) {
      /* Send STOP after the last byte */
      I2Cx_StopTransmission(I2Cx);
    }
  }

  return true;
}

static bool I2Cx_WriteData(I2C_TypeDef* I2Cx, uint8_t SlaveAddr,
                           uint16_t WriteAddr, uint8_t AddrLen,
                           uint8_t* pBuffer, uint8_t NumBytesToWrite) {
  bool Result = true;

  if (!pBuffer || !NumBytesToWrite) {
    return false;
  }

  SlaveAddr = SLAVEADDR(SlaveAddr);

  if (!I2Cx_StartTransmission(I2Cx, I2C_TRANSMITTER, SlaveAddr,
                              AddrLen + NumBytesToWrite)) {
    return false;
  }

  if (AddrLen == 2 && !I2Cx_SendByte(I2Cx, WriteAddr >> 8)) {
    I2Cx_StopTransmission(I2Cx);
    return false;
  }
  if (AddrLen >= 1 && !I2Cx_SendByte(I2Cx, WriteAddr & 0xFF)) {
    I2Cx_StopTransmission(I2Cx);
    return false;
  }

  while (NumBytesToWrite--) {
    if (!I2Cx_SendByte(I2Cx, *pBuffer++)) {
      Result = false;
      break;
    }
  }
  I2Cx_StopTransmission(I2Cx);

  return Result;
}

_INLINE void LL_IIC_Write_Data(I2C_TypeDef* I2Cx, uint8_t data) {
  I2Cx_SendByte(I2Cx, data);
}
_INLINE uint8_t LL_IIC_Read_Data(I2C_TypeDef* I2Cx) {
  uint8_t data = 0;
  I2Cx_ReceiveByte(I2Cx, &data);
  return data;
}
_INLINE bool LL_IIC_Read_8addr(I2C_TypeDef* I2Cx, uint8_t SlaveAddr,
                               uint8_t RegAddr, uint8_t* pdata, uint8_t rcnt) {
  return I2Cx_ReadData(I2Cx, SlaveAddr, RegAddr, 1, pdata, rcnt);
}
_INLINE bool LL_IIC_Read_16addr(I2C_TypeDef* I2Cx, uint8_t SlaveAddr,
                                uint16_t RegAddr, uint8_t* pdata,
                                uint8_t rcnt) {
  return I2Cx_ReadData(I2Cx, SlaveAddr, RegAddr, 2, pdata, rcnt);
}
_INLINE bool LL_IIC_Write_8addr(I2C_TypeDef* I2Cx, uint8_t SlaveAddr,
                                uint8_t RegAddr, uint8_t* pdata, uint8_t rcnt) {
  return I2Cx_WriteData(I2Cx, SlaveAddr, RegAddr, 1, pdata, rcnt);
}
_INLINE bool LL_IIC_Write_16addr(I2C_TypeDef* I2Cx, uint8_t SlaveAddr,
                                 uint16_t RegAddr, uint8_t* pdata,
                                 uint8_t rcnt) {
  return I2Cx_WriteData(I2Cx, SlaveAddr, RegAddr, 2, pdata, rcnt);
}
bool LL_IIC_Check_SlaveAddr(I2C_TypeDef* I2Cx, uint8_t SlaveAddr) {
  if (!I2Cx_StartTransmission(I2Cx, I2C_TRANSMITTER, SLAVEADDR(SlaveAddr), 1)) {
    return false;
  }
  I2Cx_SendByte(I2Cx, 0x00);
  I2Cx_StopTransmission(I2Cx);
  return true;
}

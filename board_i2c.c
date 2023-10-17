/**
 * @file board_i2c.c
 * @brief I2C interface implementation
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-10-14
 *
 * THINK DIFFERENTLY
 */

#include "board_i2c.h"

#define _INLINE __attribute__((always_inline))

#if _BOARD_I2C_USE_SW_IIC
#include "sw_i2c.h"

void I2C_Init(void) { SW_IIC_Init(); }

_INLINE void I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data) {
  SW_IIC_Write_8addr(addr, reg, &data, 1);
}

_INLINE uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg) {
  uint8_t temp;
  SW_IIC_Read_8addr(addr, reg, &temp, 1);
  return temp;
}

_INLINE void I2C_Write_Word(uint8_t addr, uint8_t reg, uint16_t data) {
  uint8_t buf[2] = {data >> 8, data};
  SW_IIC_Write_8addr(addr, reg, buf, 2);
}

_INLINE uint16_t I2C_Read_Word(uint8_t addr, uint8_t reg) {
  uint8_t buf[2] = {0, 0};
  SW_IIC_Read_8addr(addr, reg, buf, 2);
  return ((buf[1] << 8) | (buf[0] & 0xff));
}

_INLINE void I2C_Write_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                              uint8_t len) {
  SW_IIC_Write_8addr(addr, reg, data, len);
}

_INLINE void I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                             uint8_t len) {
  SW_IIC_Read_8addr(addr, reg, data, len);
}

_INLINE void I2C_Write_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                   uint8_t len) {
  SW_IIC_Write_8addr(addr, reg, (uint8_t *)data, len * 2);
}

_INLINE void I2C_Read_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                  uint8_t len) {
  SW_IIC_Read_8addr(addr, reg, (uint8_t *)data, len * 2);
}

_INLINE uint8_t I2C_Check_Device(uint8_t addr) {
  return SW_IIC_Check_SlaveAddr(addr);
}

#elif _BOARD_I2C_USE_LL_I2C
#include "ll_i2c.h"

_INLINE void I2C_Init(void) { LL_IIC_Init(); }

_INLINE void I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data) {
  LL_IIC_Write_8addr(addr, reg, &data, 1);
}

_INLINE uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg) {
  uint8_t temp;
  LL_IIC_Read_8addr(addr, reg, &temp, 1);
  return temp;
}

_INLINE void I2C_Write_Word(uint8_t addr, uint8_t reg, uint16_t data) {
  uint8_t buf[2] = {data >> 8, data};
  LL_IIC_Write_8addr(addr, reg, buf, 2);
}

_INLINE uint16_t I2C_Read_Word(uint8_t addr, uint8_t reg) {
  uint8_t buf[2] = {0, 0};
  LL_IIC_Read_8addr(addr, reg, buf, 2);
  return ((buf[1] << 8) | (buf[0] & 0xff));
}

_INLINE void I2C_Write_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                              uint8_t len) {
  LL_IIC_Write_8addr(addr, reg, data, len);
}

_INLINE void I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                             uint8_t len) {
  LL_IIC_Read_8addr(addr, reg, data, len);
}

_INLINE void I2C_Write_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                   uint8_t len) {
  LL_IIC_Write_8addr(addr, reg, (uint8_t *)data, len * 2);
}

_INLINE void I2C_Read_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                  uint8_t len) {
  LL_IIC_Read_8addr(addr, reg, (uint8_t *)data, len * 2);
}

_INLINE uint8_t I2C_Check_Device(uint8_t addr) {
  return LL_IIC_Check_SlaveAddr(addr);
}

#elif _BOARD_I2C_USE_HAL_I2C
#error "TODO: HAL interface not implemented"
_INLINE void I2C_Init(void) {}

_INLINE void I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data) {}

_INLINE uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg) {}

_INLINE void I2C_Write_Word(uint8_t addr, uint8_t reg, uint16_t data) {}

_INLINE uint16_t I2C_Read_Word(uint8_t addr, uint8_t reg) {}

_INLINE void I2C_Write_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                              uint8_t len) {}

_INLINE void I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                             uint8_t len) {}

_INLINE void I2C_Write_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                   uint8_t len) {}

_INLINE void I2C_Read_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                  uint8_t len) {}

_INLINE uint8_t I2C_Check_Device(uint8_t addr) {}

#else
#error "Please select a Board I2C interface"
#endif

#include "log.h"

void I2C_Bus_Scan(void) {
  LOG_D("> I2C Bus Scan Start");
  for (uint8_t i = 1; i < 128; i++) {
    if (I2C_Check_Device(i << 1)) {
      LOG_D("Found Device on Addr:0x%02X", i);
    }
  }
  LOG_D("> I2C Bus Scan End");
}

void I2C_Dump_Reg(uint8_t SlaveAddr, uint8_t StartReg, uint8_t StopReg) {
#define _CPL 8
  uint8_t data[_CPL];
  LOG_D("> I2C/0x%02X Reg Data", SlaveAddr >> 1);
  uint8_t line = (StopReg - StartReg) / _CPL;
  for (uint8_t i = 0; i <= line; i++) {
    I2C_Read_Buffer(SlaveAddr, StartReg, data, _CPL);
    LOG_RAW("0x%02X:", StartReg);
    for (uint8_t j = 0; j < (i == line ? (StopReg - StartReg) : _CPL); j++) {
      LOG_RAW(" %02X", data[j]);
    }
    LOG_RAW("\r\n");
    StartReg += _CPL;
  }
  LOG_D("> Dump Reg End");
}

void I2C_Update_Register(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t Mask,
                         uint8_t Data) {
  uint8_t temp = I2C_Read_Byte(SlaveAddr, RegAddr);
  temp &= ~Mask;
  temp |= Data & Mask;
  I2C_Write_Byte(SlaveAddr, RegAddr, temp);
}

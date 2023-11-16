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

static sw_iic_t _i2c_dev = {
    .sclPort = _BOARD_I2C_SW_SCL_PORT,
    .sclPin = _BOARD_I2C_SW_SCL_PIN,
    .sdaPort = _BOARD_I2C_SW_SDA_PORT,
    .sdaPin = _BOARD_I2C_SW_SDA_PIN,
};

void I2C_Init(void) { SW_IIC_Init(&_i2c_dev); }

_INLINE void I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data) {
  SW_IIC_Write_8addr(&_i2c_dev, addr, reg, &data, 1);
}

_INLINE uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg) {
  uint8_t temp;
  SW_IIC_Read_8addr(&_i2c_dev, addr, reg, &temp, 1);
  return temp;
}

_INLINE void I2C_Write_Word(uint8_t addr, uint8_t reg, uint16_t data) {
  uint8_t buf[2] = {data >> 8, data};
  SW_IIC_Write_8addr(&_i2c_dev, addr, reg, buf, 2);
}

_INLINE uint16_t I2C_Read_Word(uint8_t addr, uint8_t reg) {
  uint8_t buf[2] = {0, 0};
  SW_IIC_Read_8addr(&_i2c_dev, addr, reg, buf, 2);
  return ((buf[1] << 8) | (buf[0] & 0xff));
}

_INLINE void I2C_Write_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                              uint8_t len) {
  SW_IIC_Write_8addr(&_i2c_dev, addr, reg, data, len);
}

_INLINE void I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                             uint8_t len) {
  SW_IIC_Read_8addr(&_i2c_dev, addr, reg, data, len);
}

_INLINE void I2C_Write_Buffer_16Addr(uint8_t addr, uint8_t reg, uint8_t *data,
                                     uint8_t len) {
  SW_IIC_Write_16addr(&_i2c_dev, addr, reg, data, len);
}

_INLINE void I2C_Read_Buffer_16Addr(uint8_t addr, uint8_t reg, uint8_t *data,
                                    uint8_t len) {
  SW_IIC_Read_16addr(&_i2c_dev, addr, reg, data, len);
}

_INLINE void I2C_Write_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                   uint8_t len) {
  SW_IIC_Write_8addr(&_i2c_dev, addr, reg, (uint8_t *)data, len * 2);
}

_INLINE void I2C_Read_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                  uint8_t len) {
  SW_IIC_Read_8addr(&_i2c_dev, addr, reg, (uint8_t *)data, len * 2);
}

_INLINE uint8_t I2C_Check_Device(uint8_t addr) {
  return SW_IIC_Check_SlaveAddr(&_i2c_dev, addr);
}

#elif _BOARD_I2C_USE_LL_I2C
#include "ll_i2c.h"

_INLINE void I2C_Init(void) {}

_INLINE void I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data) {
  LL_IIC_Write_8addr(_BOARD_I2C_LL_INSTANCE, addr, reg, &data, 1);
}

_INLINE uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg) {
  uint8_t temp;
  LL_IIC_Read_8addr(_BOARD_I2C_LL_INSTANCE, addr, reg, &temp, 1);
  return temp;
}

_INLINE void I2C_Write_Word(uint8_t addr, uint8_t reg, uint16_t data) {
  uint8_t buf[2] = {data >> 8, data};
  LL_IIC_Write_8addr(_BOARD_I2C_LL_INSTANCE, addr, reg, buf, 2);
}

_INLINE uint16_t I2C_Read_Word(uint8_t addr, uint8_t reg) {
  uint8_t buf[2] = {0, 0};
  LL_IIC_Read_8addr(_BOARD_I2C_LL_INSTANCE, addr, reg, buf, 2);
  return ((buf[1] << 8) | (buf[0] & 0xff));
}

_INLINE void I2C_Write_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                              uint8_t len) {
  LL_IIC_Write_8addr(_BOARD_I2C_LL_INSTANCE, addr, reg, data, len);
}

_INLINE void I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                             uint8_t len) {
  LL_IIC_Read_8addr(_BOARD_I2C_LL_INSTANCE, addr, reg, data, len);
}

_INLINE void I2C_Write_Buffer_16Addr(uint8_t addr, uint8_t reg, uint8_t *data,
                                     uint8_t len) {
  LL_IIC_Write_16addr(_BOARD_I2C_LL_INSTANCE, addr, reg, data, len);
}

_INLINE void I2C_Read_Buffer_16Addr(uint8_t addr, uint8_t reg, uint8_t *data,
                                    uint8_t len) {
  LL_IIC_Read_16addr(_BOARD_I2C_LL_INSTANCE, addr, reg, data, len);
}

_INLINE void I2C_Write_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                   uint8_t len) {
  LL_IIC_Write_8addr(_BOARD_I2C_LL_INSTANCE, addr, reg, (uint8_t *)data,
                     len * 2);
}

_INLINE void I2C_Read_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                  uint8_t len) {
  LL_IIC_Read_8addr(_BOARD_I2C_LL_INSTANCE, addr, reg, (uint8_t *)data,
                    len * 2);
}

_INLINE uint8_t I2C_Check_Device(uint8_t addr) {
  return LL_IIC_Check_SlaveAddr(_BOARD_I2C_LL_INSTANCE, addr);
}

#elif _BOARD_I2C_USE_HAL_I2C
#include "i2c.h"

_INLINE void I2C_Init(void) {}

_INLINE void I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data) {
  HAL_I2C_Mem_Write(_BOARD_I2C_HAL_INSTANCE, addr, reg, 1, &data, 1, 1000);
}

_INLINE uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg) {
  uint8_t temp;
  HAL_I2C_Mem_Read(_BOARD_I2C_HAL_INSTANCE, addr, reg, 1, &temp, 1, 1000);
  return temp;
}

_INLINE void I2C_Write_Word(uint8_t addr, uint8_t reg, uint16_t data) {
  uint8_t buf[2] = {data >> 8, data};
  HAL_I2C_Mem_Write(_BOARD_I2C_HAL_INSTANCE, addr, reg, 1, buf, 2, 1000);
}

_INLINE uint16_t I2C_Read_Word(uint8_t addr, uint8_t reg) {
  uint8_t buf[2] = {0, 0};
  HAL_I2C_Mem_Read(_BOARD_I2C_HAL_INSTANCE, addr, reg, 1, buf, 2, 1000);
  return ((buf[1] << 8) | (buf[0] & 0xff));
}

_INLINE void I2C_Write_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                              uint8_t len) {
  HAL_I2C_Mem_Write(_BOARD_I2C_HAL_INSTANCE, addr, reg, 1, data, len, 1000);
}

_INLINE void I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                             uint8_t len) {
  HAL_I2C_Mem_Read(_BOARD_I2C_HAL_INSTANCE, addr, reg, 1, data, len, 1000);
}

_INLINE void I2C_Write_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                   uint8_t len) {
  HAL_I2C_Mem_Write(_BOARD_I2C_HAL_INSTANCE, addr, reg, 1, (uint8_t *)data,
                    len * 2, 1000);
}

_INLINE void I2C_Read_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                  uint8_t len) {
  HAL_I2C_Mem_Read(_BOARD_I2C_HAL_INSTANCE, addr, reg, 1, (uint8_t *)data,
                   len * 2, 1000);
}

_INLINE uint8_t I2C_Check_Device(uint8_t addr) {
  return HAL_I2C_IsDeviceReady(_BOARD_I2C_HAL_INSTANCE, addr, 1, 1000);
}

#else
#define _NO_INTERFACE
#endif

#ifndef _NO_INTERFACE
#include "log.h"

void I2C_Bus_Scan(void) {
  LOG_RAWLN(T_FMT(T_YELLOW) "> I2C Bus Scan Start");
  for (uint8_t i = 1; i < 128; i++) {
    // dummy read for waking up some device
    I2C_Read_Byte(i << 1, 0);
    if (I2C_Check_Device(i << 1)) {
      LOG_RAWLN(T_FMT(T_CYAN) "- Found Device: 0x%02X", i);
    }
  }
  LOG_RAWLN(T_FMT(T_YELLOW) "> I2C Bus Scan End" T_FMT(T_RESET));
}

void I2C_Dump_Reg(uint8_t SlaveAddr, uint8_t StartReg, uint8_t StopReg) {
#define ITEM_PL 8
  uint8_t data[ITEM_PL];
  LOG_RAWLN(T_FMT(T_YELLOW) "> I2C/0x%02X Reg Data" T_FMT(T_CYAN),
            SlaveAddr >> 1);
  uint8_t line = (StopReg - StartReg) / ITEM_PL;
  for (uint8_t i = 0; i <= line; i++) {
    I2C_Read_Buffer(SlaveAddr, StartReg, data, ITEM_PL);
    LOG_RAW("0x%02X:", StartReg);
    for (uint8_t j = 0; j < (i == line ? (StopReg - StartReg) : ITEM_PL); j++) {
      LOG_RAW(" %02X", data[j]);
    }
    LOG_RAWLN();
    StartReg += ITEM_PL;
  }
  LOG_RAWLN(T_FMT(T_YELLOW) "> Dump Reg End" T_FMT(T_RESET));
}

void I2C_Update_Register(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t Mask,
                         uint8_t Data) {
  uint8_t temp = I2C_Read_Byte(SlaveAddr, RegAddr);
  temp &= ~Mask;
  temp |= Data & Mask;
  I2C_Write_Byte(SlaveAddr, RegAddr, temp);
}
#endif  // !_NO_INTERFACE

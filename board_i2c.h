/**
 * @file board_i2c.h
 * @brief I2C interface definition
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-10-14
 *
 * THINK DIFFERENTLY
 */

#ifndef __BOARD_I2C_H__
#define __BOARD_I2C_H__


#include "modules.h"

#define _BOARD_I2C_USE_SW_IIC 0

#define _BOARD_I2C_USE_LL_I2C 1
#define _BOARD_I2C_LL_INSTANCE I2C2

#define _BOARD_I2C_USE_HAL_I2C 0
#define _BOARD_I2C_HAL_INSTANCE hi2c1

extern void I2C_Init(void);
extern void I2C_Write_Byte(uint8_t addr, uint8_t reg,uint8_t data);
extern uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg);
extern void I2C_Write_Word(uint8_t addr, uint8_t reg,uint16_t data);
extern uint16_t I2C_Read_Word(uint8_t addr, uint8_t reg);
extern void I2C_Write_Buffer(uint8_t addr, uint8_t reg,uint8_t *data, uint8_t len);
extern void I2C_Read_Buffer(uint8_t addr, uint8_t reg,uint8_t *data, uint8_t len);
extern void I2C_Write_Buffer_Word(uint8_t addr, uint8_t reg,uint16_t *data, uint8_t len);
extern void I2C_Read_Buffer_Word(uint8_t addr, uint8_t reg,uint16_t *data, uint8_t len);
extern uint8_t I2C_Check_Device(uint8_t addr);
extern void I2C_Bus_Scan(void);
extern void I2C_Dump_Reg(uint8_t SlaveAddr, uint8_t StartReg, uint8_t StopReg);
#endif /* __BOARD_I2C_H__ */

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

/**
 * @brief 初始化板载I2C
 */
extern void I2C_Init(void);

/**
 * @brief I2C写入一个字节
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 */
extern void I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data);

/**
 * @brief I2C读取一个字节
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @return uint8_t 读取的数据
 */
extern uint8_t I2C_Read_Byte(uint8_t addr, uint8_t reg);

/**
 * @brief I2C写入一个字
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 */
extern void I2C_Write_Word(uint8_t addr, uint8_t reg, uint16_t data);

/**
 * @brief I2C读取一个字
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @return uint16_t 读取的数据
 */
extern uint16_t I2C_Read_Word(uint8_t addr, uint8_t reg);

/**
 * @brief I2C写入多个字节
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void I2C_Write_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                             uint8_t len);

/**
 * @brief I2C读取多个字节
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t *data,
                            uint8_t len);

/**
 * @brief I2C写入多个字节
 *
 * @param addr 从机地址
 * @param reg 十六位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void I2C_Write_Buffer_16Addr(uint8_t addr, uint8_t reg, uint8_t *data,
                                    uint8_t len);

/**
 * @brief I2C读取多个字节
 *
 * @param addr 从机地址
 * @param reg 十六位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void I2C_Read_Buffer_16Addr(uint8_t addr, uint8_t reg, uint8_t *data,
                                   uint8_t len);

/**
 * @brief I2C写入多个字
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void I2C_Write_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                  uint8_t len);

/**
 * @brief I2C读取多个字
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void I2C_Read_Buffer_Word(uint8_t addr, uint8_t reg, uint16_t *data,
                                 uint8_t len);

/**
 * @brief I2C检查设备是否存在
 *
 * @param addr 从机地址
 * @return uint8_t 0:不存在 1:存在
 */
extern uint8_t I2C_Check_Device(uint8_t addr);

/**
 * @brief I2C扫描总线
 *
 */
extern void I2C_Bus_Scan(void);

/**
 * @brief I2C打印寄存器
 *
 * @param SlaveAddr 从机地址
 * @param StartReg 起始寄存器地址
 * @param StopReg 结束寄存器地址
 */
extern void I2C_Dump_Reg(uint8_t SlaveAddr, uint8_t StartReg, uint8_t StopReg);
#endif /* __BOARD_I2C_H__ */

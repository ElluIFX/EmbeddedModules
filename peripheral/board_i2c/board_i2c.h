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
#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

#if !KCONFIG_AVAILABLE
#define BOARD_I2C_CFG_USE_SW_IIC 0       // 是否使用软件IIC
#define BOARD_I2C_CFG_USE_LL_I2C 0       // 是否使用LL库IIC
#define BOARD_I2C_CFG_USE_HAL_I2C 0      // 是否使用HAL库IIC
#define BOARD_I2C_CFG_SW_SCL_PORT GPIOB  // 软件IIC SCL引脚
#define BOARD_I2C_CFG_SW_SCL_PIN GPIO_PIN_6
#define BOARD_I2C_CFG_SW_SDA_PORT GPIOB  // 软件IIC SDA引脚
#define BOARD_I2C_CFG_SW_SDA_PIN GPIO_PIN_7
#define BOARD_I2C_CFG_LL_INSTANCE I2C1    // LL库IIC实例
#define BOARD_I2C_CFG_HAL_INSTANCE hi2c1  // HAL库IIC实例

#endif  // !KCONFIG_AVAILABLE

/**
 * @brief 初始化板载I2C
 */
extern void i2c_init(void);

/**
 * @brief I2C写入一个字节
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 */
extern void i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t data);

/**
 * @brief I2C读取一个字节
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @return uint8_t 读取的数据
 */
extern uint8_t i2c_read_byte(uint8_t addr, uint8_t reg);

/**
 * @brief I2C写入一个字
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 */
extern void i2c_write_word(uint8_t addr, uint8_t reg, uint16_t data);

/**
 * @brief I2C读取一个字
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @return uint16_t 读取的数据
 */
extern uint16_t i2c_read_word(uint8_t addr, uint8_t reg);

/**
 * @brief I2C写入多个字节
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void i2c_write(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len);

/**
 * @brief I2C读取多个字节
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len);

/**
 * @brief I2C写入多个字节
 *
 * @param addr 从机地址
 * @param reg 十六位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void i2c_write_16addr(uint8_t addr, uint8_t reg, uint8_t* data,
                             uint8_t len);

/**
 * @brief I2C读取多个字节
 *
 * @param addr 从机地址
 * @param reg 十六位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void i2c_read_16addr(uint8_t addr, uint8_t reg, uint8_t* data,
                            uint8_t len);

/**
 * @brief I2C写入多个字
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void i2c_write_words(uint8_t addr, uint8_t reg, uint16_t* data,
                            uint8_t len);

/**
 * @brief I2C读取多个字
 *
 * @param addr 从机地址
 * @param reg 八位寄存器地址
 * @param data 数据
 * @param len 数据长度
 */
extern void i2c_read_words(uint8_t addr, uint8_t reg, uint16_t* data,
                           uint8_t len);

/**
 * @brief I2C检查设备是否存在
 *
 * @param addr 从机地址
 * @return uint8_t 0:不存在 1:存在
 */
extern uint8_t i2c_check_slave(uint8_t addr);

/**
 * @brief I2C扫描总线
 *
 */
extern void i2c_bus_scan(void);

/**
 * @brief I2C打印寄存器
 *
 * @param addr 从机地址
 * @param start 起始寄存器地址
 * @param stop 结束寄存器地址
 */
extern void i2c_dump(uint8_t addr, uint8_t start, uint8_t stop);

/**
 * @brief I2C更新寄存器
 *
 * @param addr 从机地址
 * @param reg 寄存器地址
 * @param mask 更新掩码
 * @param data 更新数据
 */
extern void i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t mask,
                          uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_I2C_H__ */

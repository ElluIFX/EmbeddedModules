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

#if BOARD_I2C_CFG_USE_SW_IIC
#include "sw_i2c.h"

static sw_i2c_t _i2c_dev = {
    .sclPort = BOARD_I2C_CFG_SW_SCL_PORT,
    .sclPin = BOARD_I2C_CFG_SW_SCL_PIN,
    .sdaPort = BOARD_I2C_CFG_SW_SDA_PORT,
    .sdaPin = BOARD_I2C_CFG_SW_SDA_PIN,
};

void i2c_init(void) {
    sw_i2c_init(&_i2c_dev);
}

_INLINE void i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t data) {
    sw_i2c_write(&_i2c_dev, addr, reg, &data, 1);
}

_INLINE uint8_t i2c_read_byte(uint8_t addr, uint8_t reg) {
    uint8_t temp;
    sw_i2c_read(&_i2c_dev, addr, reg, &temp, 1);
    return temp;
}

_INLINE void i2c_write_word(uint8_t addr, uint8_t reg, uint16_t data) {
    uint8_t buf[2] = {data >> 8, data};
    sw_i2c_write(&_i2c_dev, addr, reg, buf, 2);
}

_INLINE uint16_t i2c_read_word(uint8_t addr, uint8_t reg) {
    uint8_t buf[2] = {0, 0};
    sw_i2c_read(&_i2c_dev, addr, reg, buf, 2);
    return ((buf[1] << 8) | (buf[0] & 0xff));
}

_INLINE void i2c_write(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len) {
    sw_i2c_write(&_i2c_dev, addr, reg, data, len);
}

_INLINE void i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len) {
    sw_i2c_read(&_i2c_dev, addr, reg, data, len);
}

_INLINE void i2c_write_16addr(uint8_t addr, uint8_t reg, uint8_t* data,
                              uint8_t len) {
    sw_i2c_write_16addr(&_i2c_dev, addr, reg, data, len);
}

_INLINE void i2c_read_16addr(uint8_t addr, uint8_t reg, uint8_t* data,
                             uint8_t len) {
    sw_i2c_read_16addr(&_i2c_dev, addr, reg, data, len);
}

_INLINE void i2c_write_words(uint8_t addr, uint8_t reg, uint16_t* data,
                             uint8_t len) {
    sw_i2c_write(&_i2c_dev, addr, reg, (uint8_t*)data, len * 2);
}

_INLINE void i2c_read_words(uint8_t addr, uint8_t reg, uint16_t* data,
                            uint8_t len) {
    sw_i2c_read(&_i2c_dev, addr, reg, (uint8_t*)data, len * 2);
}

_INLINE uint8_t i2c_check_slave(uint8_t addr) {
    return sw_i2c_check_addr(&_i2c_dev, addr);
}

#elif BOARD_I2C_CFG_USE_LL_I2C
#include "ll_i2c.h"

_INLINE void i2c_init(void) {
    ll_i2c_init(BOARD_I2C_CFG_LL_INSTANCE);
}

_INLINE void i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t data) {
    ll_i2c_write(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, &data, 1);
}

_INLINE uint8_t i2c_read_byte(uint8_t addr, uint8_t reg) {
    uint8_t temp;
    ll_i2c_read(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, &temp, 1);
    return temp;
}

_INLINE void i2c_write_word(uint8_t addr, uint8_t reg, uint16_t data) {
    uint8_t buf[2] = {data >> 8, data};
    ll_i2c_write(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, buf, 2);
}

_INLINE uint16_t i2c_read_word(uint8_t addr, uint8_t reg) {
    uint8_t buf[2] = {0, 0};
    ll_i2c_read(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, buf, 2);
    return ((buf[1] << 8) | (buf[0] & 0xff));
}

_INLINE void i2c_write(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len) {
    ll_i2c_write(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, data, len);
}

_INLINE void i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len) {
    ll_i2c_read(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, data, len);
}

_INLINE void i2c_write_16addr(uint8_t addr, uint8_t reg, uint8_t* data,
                              uint8_t len) {
    ll_i2c_write_16addr(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, data, len);
}

_INLINE void i2c_read_16addr(uint8_t addr, uint8_t reg, uint8_t* data,
                             uint8_t len) {
    ll_i2c_read_16addr(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, data, len);
}

_INLINE void i2c_write_words(uint8_t addr, uint8_t reg, uint16_t* data,
                             uint8_t len) {
    ll_i2c_write(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, (uint8_t*)data, len * 2);
}

_INLINE void i2c_read_words(uint8_t addr, uint8_t reg, uint16_t* data,
                            uint8_t len) {
    ll_i2c_read(BOARD_I2C_CFG_LL_INSTANCE, addr, reg, (uint8_t*)data, len * 2);
}

_INLINE uint8_t i2c_check_slave(uint8_t addr) {
    return ll_i2c_check_addr(BOARD_I2C_CFG_LL_INSTANCE, addr);
}

#elif BOARD_I2C_CFG_USE_HAL_I2C
#include "i2c.h"

_INLINE void i2c_init(void) {}

_INLINE void i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t data) {
    HAL_I2C_Mem_Write(BOARD_I2C_CFG_HAL_INSTANCE, addr, reg, 1, &data, 1, 1000);
}

_INLINE uint8_t i2c_read_byte(uint8_t addr, uint8_t reg) {
    uint8_t temp;
    HAL_I2C_Mem_Read(BOARD_I2C_CFG_HAL_INSTANCE, addr, reg, 1, &temp, 1, 1000);
    return temp;
}

_INLINE void i2c_write_word(uint8_t addr, uint8_t reg, uint16_t data) {
    uint8_t buf[2] = {data >> 8, data};
    HAL_I2C_Mem_Write(BOARD_I2C_CFG_HAL_INSTANCE, addr, reg, 1, buf, 2, 1000);
}

_INLINE uint16_t i2c_read_word(uint8_t addr, uint8_t reg) {
    uint8_t buf[2] = {0, 0};
    HAL_I2C_Mem_Read(BOARD_I2C_CFG_HAL_INSTANCE, addr, reg, 1, buf, 2, 1000);
    return ((buf[1] << 8) | (buf[0] & 0xff));
}

_INLINE void i2c_write(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len) {
    HAL_I2C_Mem_Write(BOARD_I2C_CFG_HAL_INSTANCE, addr, reg, 1, data, len,
                      1000);
}

_INLINE void i2c_read(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len) {
    HAL_I2C_Mem_Read(BOARD_I2C_CFG_HAL_INSTANCE, addr, reg, 1, data, len, 1000);
}

_INLINE void i2c_write_words(uint8_t addr, uint8_t reg, uint16_t* data,
                             uint8_t len) {
    HAL_I2C_Mem_Write(BOARD_I2C_CFG_HAL_INSTANCE, addr, reg, 1, (uint8_t*)data,
                      len * 2, 1000);
}

_INLINE void i2c_read_words(uint8_t addr, uint8_t reg, uint16_t* data,
                            uint8_t len) {
    HAL_I2C_Mem_Read(BOARD_I2C_CFG_HAL_INSTANCE, addr, reg, 1, (uint8_t*)data,
                     len * 2, 1000);
}

_INLINE uint8_t i2c_check_slave(uint8_t addr) {
    return HAL_I2C_IsDeviceReady(BOARD_I2C_CFG_HAL_INSTANCE, addr, 1, 1000);
}

#else
#define _NO_INTERFACE
#endif

#ifndef _NO_INTERFACE
#define LOG_MODULE "i2c"
#include "log.h"

void i2c_bus_scan(void) {
    PRINTLN(T_FMT(T_YELLOW) "> I2C Bus Scan Start");
    for (uint8_t i = 1; i < 128; i++) {
        // dummy read for waking up some device
        i2c_read_byte(i << 1, 0);
        if (i2c_check_slave(i << 1)) {
            PRINTLN(T_FMT(T_CYAN) "- Found Device: 0x%02X", i);
        }
    }
    PRINTLN(T_FMT(T_YELLOW) "> I2C Bus Scan End" T_FMT(T_RESET));
}

void i2c_dump(uint8_t addr, uint8_t start, uint8_t stop) {
#define ITEM_PL 8
    uint8_t data[ITEM_PL];
    PRINTLN(T_FMT(T_YELLOW) "> I2C/0x%02X Reg Data" T_FMT(T_CYAN), addr >> 1);
    uint8_t line = (stop - start) / ITEM_PL;
    for (uint8_t i = 0; i <= line; i++) {
        i2c_read(addr, start, data, ITEM_PL);
        PRINT("0x%02X:", start);
        for (uint8_t j = 0; j < (i == line ? (stop - start) : ITEM_PL); j++) {
            PRINT(" %02X", data[j]);
        }
        PRINTLN();
        start += ITEM_PL;
    }
    PRINTLN(T_FMT(T_YELLOW) "> Dump Reg End" T_FMT(T_RESET));
}

void i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t mask, uint8_t data) {
    uint8_t temp = i2c_read_byte(addr, reg);
    temp &= ~mask;
    temp |= data & mask;
    i2c_write_byte(addr, reg, temp);
}
#endif  // !_NO_INTERFACE

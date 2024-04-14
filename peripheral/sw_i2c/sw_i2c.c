#include "sw_i2c.h"

#define LOG_MODULE "sw-i2c"
#include "log.h"

#define I2C_READ 0x01
#define READ_CMD 1
#define WRITE_CMD 0

#define PULL_MODE GPIO_NOPULL

// #define PULL_MODE GPIO_PULLUP

static void sda_in_mode(sw_i2c_t* dev) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = PULL_MODE;
    GPIO_InitStruct.Pin = dev->sdaPin;
    HAL_GPIO_Init(dev->sdaPort, &GPIO_InitStruct);
}

static void sda_out_mode(sw_i2c_t* dev) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = PULL_MODE;
    GPIO_InitStruct.Pin = dev->sdaPin;
    HAL_GPIO_Init(dev->sdaPort, &GPIO_InitStruct);
}

static void scl_in_mode(sw_i2c_t* dev) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = PULL_MODE;
    GPIO_InitStruct.Pin = dev->sclPin;
    HAL_GPIO_Init(dev->sclPort, &GPIO_InitStruct);
}

static void scl_out_mode(sw_i2c_t* dev) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = PULL_MODE;
    GPIO_InitStruct.Pin = dev->sclPin;
    HAL_GPIO_Init(dev->sclPort, &GPIO_InitStruct);
}

#define wait_i2c() m_delay_us(dev->waitTime)
#define wait_i2c_long() m_delay_us(dev->waitTimeLong)

#define SSHigh(GPIOx, Pinx) ((GPIOx)->BSRR = (Pinx))
// ((GPIOx)->BSRR = (Pinx) << 16) or ((GPIOx)->BRR = (Pinx))
#define SSLow(GPIOx, Pinx) ((GPIOx)->BSRR = (Pinx) << 16)
#define SSRead(GPIOx, Pinx) (((GPIOx)->IDR) & (Pinx))
#define sda_high() SSHigh(dev->sdaPort, dev->sdaPin)
#define sda_low() SSLow(dev->sdaPort, dev->sdaPin)
#define scl_high() SSHigh(dev->sclPort, dev->sclPin)
#define scl_low() SSLow(dev->sclPort, dev->sclPin)
#define sda_out(out) ((out) ? sda_high() : sda_low())
#define i2c_clk_data_out() \
    scl_high();            \
    wait_i2c();            \
    scl_low()
#define i2c_port_initial() \
    sda_high();            \
    scl_high()
#define read_sda() SSRead(dev->sdaPort, dev->sdaPin)
#define read_scl() SSRead(dev->sclPort, dev->sclPin)

static void i2c_start_condition(sw_i2c_t* dev) {
    sda_high();
    scl_high();
    wait_i2c();
    sda_low();
    wait_i2c();
    scl_low();
    wait_i2c_long();
}

static void i2c_stop_condition(sw_i2c_t* dev) {
    sda_low();
    scl_high();
    wait_i2c();
    sda_high();
    wait_i2c();
}

static uint8_t i2c_check_ack(sw_i2c_t* dev) {
    uint8_t ack;
    uint8_t i;
    uint8_t temp;
    sda_in_mode(dev);
    scl_high();
    ack = 0;
    wait_i2c();
    for (i = 10; i > 0; i--) {
        temp = !(read_sda());
        if (temp) {
            ack = 1;
            break;
        }
    }
    scl_low();
    sda_out_mode(dev);
    wait_i2c();
    return ack;
}

static void i2c_check_not_ack(sw_i2c_t* dev) {
    sda_in_mode(dev);
    i2c_clk_data_out();
    sda_out_mode(dev);
    wait_i2c();
}

static void i2c_slave_address(sw_i2c_t* dev, uint8_t addr, uint8_t readwrite) {
    int x;

    if (readwrite) {
        addr |= I2C_READ;
    } else {
        addr &= ~I2C_READ;
    }

    scl_low();

    for (x = 7; x >= 0; x--) {
        sda_out(addr & (1 << x));
        wait_i2c();
        i2c_clk_data_out();
    }
}

static void i2c_register_address(sw_i2c_t* dev, uint8_t addr) {
    int x;
    scl_low();
    for (x = 7; x >= 0; x--) {
        sda_out(addr & (1 << x));
        wait_i2c();
        i2c_clk_data_out();
    }
}

static void i2c_send_ack(sw_i2c_t* dev) {
    sda_out_mode(dev);
    sda_low();
    wait_i2c();
    scl_high();
    wait_i2c_long();
    sda_low();
    wait_i2c_long();
    scl_low();
    sda_out_mode(dev);
    wait_i2c();
}

#define DEFAULT_WAIT_TIME 1
#define DEFAULT_WAIT_TIME_LONG 3

// (1,3) -> 400KHz Fast Mode
// (4,10) -> 100KHz Standard Mode

/**
 * @brief Initializes the software I2C device.
 *
 * @param dev Pointer to the software I2C device structure.
 * @note The required GPIO pins must be enabled in CubeMX first.
 */
void sw_i2c_init(sw_i2c_t* dev) {
    if (!dev->waitTime)
        dev->waitTime = DEFAULT_WAIT_TIME;
    if (!dev->waitTimeLong)
        dev->waitTimeLong = DEFAULT_WAIT_TIME_LONG;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pull = PULL_MODE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pin = dev->sclPin;
    HAL_GPIO_Init(dev->sclPort, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = dev->sdaPin;
    HAL_GPIO_Init(dev->sdaPort, &GPIO_InitStruct);

    SSHigh(dev->sclPort, dev->sclPin);
    SSHigh(dev->sdaPort, dev->sdaPin);
}

/**
 * @brief Writes a byte of data to the I2C bus using software I2C.
 *
 * @param dev Pointer to the software I2C device structure.
 * @param data The byte of data to write.
 */
void sw_i2c_write_byte(sw_i2c_t* dev, uint8_t data) {
    int x;
    scl_low();
    for (x = 7; x >= 0; x--) {
        sda_out(data & (1 << x));
        wait_i2c();
        i2c_clk_data_out();
    }
}

/**
 * @brief Reads a byte of data from the I2C bus using software I2C.
 *
 * @param dev Pointer to the software I2C device structure.
 * @return The byte of data read from the I2C bus.
 */
uint8_t sw_i2c_read_byte(sw_i2c_t* dev) {
    uint8_t x;
    uint8_t readdata = 0;
    sda_in_mode(dev);
    for (x = 8; x--;) {
        scl_high();
        readdata <<= 1;
        if (read_sda())
            readdata |= 0x01;
        wait_i2c();
        scl_low();
        wait_i2c();
    }
    sda_out_mode(dev);
    return readdata;
}

/**
 * @brief Reads multiple bytes of data from the I2C bus using software I2C.
 *
 * @param dev Pointer to the software I2C device structure.
 * @param addr The slave address of the I2C device to read from.
 * @param reg The register address to start reading from.
 * @param pdata Pointer to the buffer to store the read data.
 * @param rcnt The number of bytes to read.
 * @return 1 if the read was successful, 0 otherwise.
 */
uint8_t sw_i2c_read(sw_i2c_t* dev, uint8_t addr, uint8_t reg, uint8_t* pdata,
                    uint8_t rcnt) {
    uint8_t returnack = 1;
    uint8_t index;

    if (!rcnt)
        return 0;

    i2c_port_initial();
    i2c_start_condition(dev);
    i2c_slave_address(dev, addr, WRITE_CMD);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    i2c_register_address(dev, reg);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    i2c_start_condition(dev);
    i2c_slave_address(dev, addr, READ_CMD);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    if (rcnt > 1) {
        for (index = 0; index < (rcnt - 1); index++) {
            wait_i2c();
            pdata[index] = sw_i2c_read_byte(dev);
            i2c_send_ack(dev);
        }
    }
    wait_i2c();
    pdata[rcnt - 1] = sw_i2c_read_byte(dev);
    i2c_check_not_ack(dev);
    i2c_stop_condition(dev);

    return returnack;
}

/**
 * @brief Reads multiple bytes of data from the I2C bus using software I2C.
 *
 * @param dev Pointer to the software I2C device structure.
 * @param addr The slave address of the I2C device to read from.
 * @param reg The register address to start reading from.
 * @param pdata Pointer to the buffer to store the read data.
 * @param rcnt The number of bytes to read.
 * @return 1 if the read was successful, 0 otherwise.
 */
uint8_t sw_i2c_read_16addr(sw_i2c_t* dev, uint8_t addr, uint16_t reg,
                           uint8_t* pdata, uint8_t rcnt) {
    uint8_t returnack = 1;
    uint8_t index;

    if (!rcnt)
        return 0;

    i2c_port_initial();
    i2c_start_condition(dev);
    // 写ID
    i2c_slave_address(dev, addr, WRITE_CMD);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    // 写高八位地址
    i2c_register_address(dev, (uint8_t)(reg >> 8));
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    // 写低八位地址
    i2c_register_address(dev, (uint8_t)reg);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    // 重启I2C总线
    i2c_start_condition(dev);
    // 读ID
    i2c_slave_address(dev, addr, READ_CMD);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    // 循环读数据
    if (rcnt > 1) {
        for (index = 0; index < (rcnt - 1); index++) {
            wait_i2c();
            pdata[index] = sw_i2c_read_byte(dev);
            i2c_send_ack(dev);
        }
    }
    wait_i2c();
    pdata[rcnt - 1] = sw_i2c_read_byte(dev);
    i2c_check_not_ack(dev);
    i2c_stop_condition(dev);

    return returnack;
}

/**
 * @brief Writes multiple bytes of data to the I2C bus using software I2C.
 *
 * @param dev Pointer to the software I2C device structure.
 * @param addr The slave address of the I2C device to write to.
 * @param reg The register address to start writing to.
 * @param pdata Pointer to the buffer containing the data to write.
 * @param rcnt The number of bytes to write.
 * @return 1 if the write was successful, 0 otherwise.
 */
uint8_t sw_i2c_write(sw_i2c_t* dev, uint8_t addr, uint8_t reg, uint8_t* pdata,
                     uint8_t rcnt) {
    uint8_t returnack = 1;
    uint8_t index;

    if (!rcnt)
        return 0;

    i2c_port_initial();
    i2c_start_condition(dev);
    i2c_slave_address(dev, addr, WRITE_CMD);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    i2c_register_address(dev, reg);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    for (index = 0; index < rcnt; index++) {
        sw_i2c_write_byte(dev, pdata[index]);
        if (!i2c_check_ack(dev)) {
            returnack = 0;
        }
        wait_i2c();
    }
    i2c_stop_condition(dev);
    return returnack;
}

/**
 * @brief Writes multiple bytes of data to the I2C bus using software I2C.
 *
 * @param dev Pointer to the software I2C device structure.
 * @param addr The slave address of the I2C device to write to.
 * @param reg The register address to start writing to.
 * @param pdata Pointer to the buffer containing the data to write.
 * @param rcnt The number of bytes to write.
 * @return 1 if the write was successful, 0 otherwise.
 */
uint8_t sw_i2c_write_16addr(sw_i2c_t* dev, uint8_t addr, uint16_t reg,
                            uint8_t* pdata, uint8_t rcnt) {
    uint8_t returnack = 1;
    uint8_t index;

    if (!rcnt)
        return 0;

    i2c_port_initial();
    i2c_start_condition(dev);
    // 写ID
    i2c_slave_address(dev, addr, WRITE_CMD);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    // 写高八位地址
    i2c_register_address(dev, (uint8_t)(reg >> 8));
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    // 写低八位地址
    i2c_register_address(dev, (uint8_t)reg);
    if (!i2c_check_ack(dev)) {
        returnack = 0;
    }
    wait_i2c();
    // 写数据
    for (index = 0; index < rcnt; index++) {
        sw_i2c_write_byte(dev, pdata[index]);
        if (!i2c_check_ack(dev)) {
            returnack = 0;
        }
        wait_i2c();
    }
    i2c_stop_condition(dev);
    return returnack;
}

/**
 * @brief Checks if a slave device is present on the I2C bus using software I2C.
 *
 * @param dev Pointer to the software I2C device structure.
 * @param addr The slave address of the device to check.
 * @return 1 if the device is present, 0 otherwise.
 */
uint8_t sw_i2c_check_addr(sw_i2c_t* dev, uint8_t addr) {
    uint8_t returnack = 1;
    i2c_start_condition(dev);
    i2c_slave_address(dev, addr, WRITE_CMD);
    if (!i2c_check_ack(dev)) {
        i2c_stop_condition(dev);
        returnack = 0;
    }
    i2c_stop_condition(dev);
    return returnack;
}

/**
 * @brief Scans the I2C bus for devices using software I2C.
 *
 * @param dev Pointer to the software I2C device structure.
 * @param addr_list Pointer to the buffer to store the list of addresses found.
 * @param addr_cnt Pointer to the variable to store the number of addresses
 */
void sw_i2c_bus_scan(sw_i2c_t* dev, uint8_t* addr_list, uint8_t* addr_cnt) {
    uint8_t temp;
    if (addr_cnt)
        *addr_cnt = 0;
    PRINTLN(T_FMT(T_YELLOW) "> SW I2C Bus Scan Start");
    for (uint8_t i = 1; i < 128; i++) {
        // dummy read for waking up some device
        sw_i2c_read(dev, i << 1, 0, &temp, 1);
        if (sw_i2c_check_addr(dev, i << 1)) {
            PRINTLN(T_FMT(T_CYAN) "- Found Device: 0x%02X", i);
            if (addr_list)
                *addr_list++ = i;
            if (addr_cnt)
                (*addr_cnt)++;
        }
    }
    PRINTLN(T_FMT(T_YELLOW) "> SW I2C Bus Scan End" T_FMT(T_RESET));
}

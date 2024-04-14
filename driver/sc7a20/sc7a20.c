/**
 * @file sc7a20.c
 * @brief sc7a20 driver
 * @author Ellu (lutaoyu@163.com)
 * @version 1.0
 * @date 2023-08-17
 *
 * THINK DIFFERENTLY
 */

#include "sc7a20.h"

#include "log.h"

#define WHO_AM_I_REG 0x0F
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define CTRL_REG5 0x24
#define CTRL_REG6 0x25
#define ADDR_STATUS_REG 0x27
#define OUT_X_L_REG 0x28
#define OUT_X_H_REG 0x29
#define OUT_Y_L_REG 0x2A
#define OUT_Y_H_REG 0x2B
#define OUT_Z_L_REG 0x2C
#define OUT_Z_H_REG 0x2D
#define OUT_REG_INC (OUT_X_L_REG | 0x80)
#define CHIP_ID 0x11

// 转换12位补码
int16_t Convert12bitComplement(uint8_t msb, uint8_t lsb) {
    int16_t temp;
    temp = msb << 8 | lsb;
    temp = temp >> 4;  // 只有高12位有效
    temp = temp & 0x0fff;
    if (temp & 0x0800)  // 负数 补码==>原码
    {
        temp = temp & 0x07ff;  // 丢弃最高位
        temp = ~temp;
        temp = temp + 1;
        temp = temp & 0x07ff;
        temp = -temp;  // 还原最高位
    }
    return temp;
}

void SC7A20_Init(sc7a20_handle_t* this) {
    uint8_t temp;
    this->read(WHO_AM_I_REG, &temp, 1);
    if (temp != CHIP_ID) {
        LOG_WARN("SC7A20 Not found");
        return;
    }
    SC7A20_SetFreqMode(this, SC7A20_FREQ_100HZ, 0);
    SC7A20_SetAccelRange(this, SC7A20_RANGE_2G, 0);
    LOG_PASS("SC7A20 Initialized");
}

void SC7A20_SetFreqMode(sc7a20_handle_t* this, uint8_t mode,
                        uint8_t low_power) {
    mode <<= 4;
    if (low_power) {
        mode |= 0x08;  // 低功耗模式
    }
    mode |= 0x07;  // 使能XYZ轴
    this->write(CTRL_REG1, &mode, 1);
}

void SC7A20_SetAccelRange(sc7a20_handle_t* this, uint8_t range,
                          uint8_t high_res) {
    range <<= 4;
    if (high_res) {
        range |= 0x08;  // 高分辨率模式
    }
    this->write(CTRL_REG2, &range, 1);
    switch (range) {
        case SC7A20_RANGE_2G:
            this->measure_range = 2.0f;
            break;
        case SC7A20_RANGE_4G:
            this->measure_range = 4.0f;
            break;
        case SC7A20_RANGE_8G:
            this->measure_range = 8.0f;
            break;
        case SC7A20_RANGE_16G:
            this->measure_range = 16.0f;
            break;
    }
}

uint8_t SC7A20_NewDataReady(sc7a20_handle_t* this) {
    uint8_t status;
    this->read(ADDR_STATUS_REG, &status, 1);
    return status & 0x08;
}

void SC7A20_Measure(sc7a20_handle_t* this) {
    uint8_t data[6];
    this->read(OUT_REG_INC, data, 6);
    this->acc_x_raw = Convert12bitComplement(data[1], data[0]);
    this->acc_y_raw = Convert12bitComplement(data[3], data[2]);
    this->acc_z_raw = Convert12bitComplement(data[5], data[4]);

    // Calculate acceleration values in g
    this->acc_x = (float)this->acc_x_raw * this->measure_range / 2048.0f;
    this->acc_y = (float)this->acc_y_raw * this->measure_range / 2048.0f;
    this->acc_z = (float)this->acc_z_raw * this->measure_range / 2048.0f;
}

void SC7A20_EnableInterrupt(sc7a20_handle_t* this) {
    uint8_t temp;
    temp = 0x40;  // AOI1 on int1
    this->write(CTRL_REG3, &temp, 1);
    temp = 0x20;   // AOI2 on int2
    temp |= 0x02;  // latch interrupt
    this->write(CTRL_REG6, &temp, 1);
}

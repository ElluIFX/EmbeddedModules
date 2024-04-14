/*
 * @file sc7a20.h
 * @brief SC7A20 Driver
 * @author Ellu (lutaoyu@163.com)
 * @version 1.0
 * @date 2023-08-17
 *
 * THINK DIFFERENTLY
 */

#ifndef __SC7A20__
#define __SC7A20__

#ifndef __SC7A20_H__
#define __SC7A20_H__

#include "modules.h"

#define SC7A20_FREQ_1HZ 0b0001
#define SC7A20_FREQ_10HZ 0b0010
#define SC7A20_FREQ_25HZ 0b0011
#define SC7A20_FREQ_50HZ 0b0100
#define SC7A20_FREQ_100HZ 0b0101
#define SC7A20_FREQ_200HZ 0b0110
#define SC7A20_FREQ_400HZ 0b0111
#define SC7A20_FREQ_1250HZ 0b1001
#define SC7A20_FREQ_1620HZ_LP 0b1000
#define SC7A20_FREQ_5000HZ_LP 0b1001

#define SC7A20_RANGE_2G 0b00
#define SC7A20_RANGE_4G 0b01
#define SC7A20_RANGE_8G 0b10
#define SC7A20_RANGE_16G 0b11

#define SC7A20_I2C_ADDR 0x18

typedef struct {
    // function
    void (*read)(uint8_t reg, uint8_t* data, uint8_t len);
    void (*write)(uint8_t reg, uint8_t* data, uint8_t len);
    // data
    float acc_x;
    float acc_y;
    float acc_z;
    int16_t acc_x_raw;
    int16_t acc_y_raw;
    int16_t acc_z_raw;
    float measure_range;
} sc7a20_handle_t;

extern void SC7A20_Init(sc7a20_handle_t* this);
extern void SC7A20_SetFreqMode(sc7a20_handle_t* this, uint8_t mode,
                               uint8_t low_power);
extern void SC7A20_SetAccelRange(sc7a20_handle_t* this, uint8_t range,
                                 uint8_t high_res);
extern uint8_t SC7A20_NewDataReady(sc7a20_handle_t* this);
extern void SC7A20_Measure(sc7a20_handle_t* this);
extern void SC7A20_EnableInterrupt(sc7a20_handle_t* this);

#endif /* __SC7A20_H__ */

#endif /* __SC7A20__ */

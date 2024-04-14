/**
 * @file paj7620u2.c
 * @brief PAJ7620U2 driver
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-10-14
 *
 * THINK DIFFERENTLY
 */

#include "paj7620u2.h"

#include "paj7620u2_regs.h"

#define ADDR (PAJ7620U2_I2C_ADDRESS)

PAJ_Mode_t PAJ_Mode;

__weak uint8_t PAJ7620U2_I2C_Read_Byte(uint8_t addr, uint8_t reg) {
    return 0;
}

__weak void PAJ7620U2_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data) {}

__weak void PAJ7620U2_I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t* buf,
                                      uint16_t len) {}

static uint16_t m_I2C_Read_Word(uint8_t addr, uint8_t reg) {
    uint8_t buf[2] = {0};
    PAJ7620U2_I2C_Read_Buffer(addr, reg, buf, 2);
    return (buf[0] | (buf[1] << 8));
}

uint8_t PAJ7620U2_Init(void) {
    PAJ7620U2_I2C_Write_Byte(ADDR, PAJ_BANK_SELECT, 0);
    if (PAJ7620U2_I2C_Read_Byte(ADDR, 0x00) != 0x20)
        return 0;  // Wake up failed
    for (uint16_t i = 0; i < Init_Array; i++) {
        PAJ7620U2_I2C_Write_Byte(
            ADDR, Init_Register_Array[i][0],
            Init_Register_Array[i][1]);  // Power up initialize
    }
    PAJ_Mode = PAJ_MODE_NONE;
    return 1;
}

void PAJ7620U2_Set_Enable(uint8_t enable) {
    PAJ7620U2_I2C_Write_Byte(ADDR, PAJ_BANK_SELECT, 1);
    PAJ7620U2_I2C_Write_Byte(ADDR, 0x72, enable ? 0x01 : 0x00);
    PAJ7620U2_I2C_Write_Byte(ADDR, PAJ_BANK_SELECT, 0);
}

void PAJ7620U2_Enter_Gesture_Mode(void) {
    for (uint16_t i = 0; i < Gesture_Array_SIZE; i++) {
        PAJ7620U2_I2C_Write_Byte(
            ADDR, Init_Gesture_Array[i][0],
            Init_Gesture_Array[i][1]);  // Gesture register initializes
    }
    PAJ_Mode = PAJ_MODE_GESTURE;
}

void PAJ7620U2_Enter_Cursor_Mode(void) {
    for (uint16_t i = 0; i < Cursor_Array_SIZE; i++) {
        PAJ7620U2_I2C_Write_Byte(
            ADDR, Init_Cursor_Array[i][0],
            Init_Cursor_Array[i][1]);  // Cursor register initializes
    }
    PAJ_Mode = PAJ_MODE_CURSOR;
}

uint8_t PAJ7620U2_Read_CursorInSight(void) {
    return PAJ7620U2_I2C_Read_Byte(ADDR, 0x44) == 0x04;
}

void PAJ7602_Read_CursorPos(int16_t* x, int16_t* y) {
    uint8_t buf[4] = {0};
    PAJ7620U2_I2C_Read_Buffer(ADDR, PAJ_CURSOR_X_L, buf, 4);
    uint16_t x_ = (buf[0]) | ((buf[1] & 0x0F) << 8);
    uint16_t y_ = (buf[2]) | ((buf[3] & 0x0F) << 8);
    *x = (int16_t)(x_ - 0x7FF);
    *y = (int16_t)(y_ - 0x7FF);
}

void PAJ7620U2_Set_AxisInvert(uint8_t x_invert, uint8_t y_invert) {
    PAJ7620U2_I2C_Write_Byte(ADDR, PAJ_BANK_SELECT, 1);  // reg in Bank1
    uint8_t temp = PAJ7620U2_I2C_Read_Byte(ADDR, 0x04);
    if (x_invert)
        temp |= 0x01;
    else
        temp &= 0xFE;
    if (y_invert)
        temp |= 0x02;
    else
        temp &= 0xFD;
    PAJ7620U2_I2C_Write_Byte(ADDR, 0x04, temp);
    PAJ7620U2_I2C_Write_Byte(ADDR, PAJ_BANK_SELECT, 0);
}

void PAJ7620U2_Set_ReportMode(PAJ_ReportMode_t mode) {
    uint8_t regIdleTime = 0;
    switch (mode) {
            // Far Mode: 1 report time = (77 + R_IDLE_TIME) * T
        case PAJ_REPMODE_FAR_240FPS:
            regIdleTime = 53;  // 1/(240*T) - 77
            break;
        case PAJ_REPMODE_FAR_120FPS:
            regIdleTime = 183;  // 1/(120*T) - 77
            break;
            // Near Mode: 1 report time = (112 + R_IDLE_TIME) * T
        case PAJ_REPMODE_NEAR_240FPS:
            regIdleTime = 18;  // 1/(240*T) - 112
            break;
        case PAJ_REPMODE_NEAR_120FPS:
            regIdleTime = 148;  // 1/(120*T) - 112
            break;
        default:
            return;
    }
    PAJ7620U2_I2C_Write_Byte(ADDR, PAJ_BANK_SELECT, 1);  // reg in Bank1
    PAJ7620U2_I2C_Write_Byte(ADDR, 0x65, regIdleTime);
    PAJ7620U2_I2C_Write_Byte(ADDR, PAJ_BANK_SELECT, 0);
}

PAJ_Gesture_t PAJ7620U2_Read_Gesture(void) {
    return m_I2C_Read_Word(ADDR, PAJ_INT_FLAG1);
}

uint8_t PAJ7620U2_Read_WaveCount(void) {
    return PAJ7620U2_I2C_Read_Byte(ADDR, 0xB7) & 0x0F;
}

uint8_t PAJ7620U2_Read_NoObjectCount(void) {
    return PAJ7620U2_I2C_Read_Byte(ADDR, 0xB8) & 0x0F;
}

uint8_t PAJ7620U2_Read_NoMotionCount(void) {
    return PAJ7620U2_I2C_Read_Byte(ADDR, 0xB9) & 0x0F;
}

uint8_t PAJ7620U2_Read_ObjectAvailable(void) {
    return PAJ7620U2_Read_NoObjectCount() == 0;
}

void PAJ7620U2_Read_ObjectCenterPos(uint16_t* x, uint16_t* y) {
    uint8_t buf[4] = {0};
    PAJ7620U2_I2C_Read_Buffer(ADDR, 0xAC, buf, 4);
    *x = buf[0] | ((buf[1] & 0x1F) << 8);
    *y = buf[2] | ((buf[3] & 0x1F) << 8);
}

void PAJ7620U2_Read_ObjectVelocity(int16_t* x, int16_t* y) {
    uint8_t buf[4] = {0};
    PAJ7620U2_I2C_Read_Buffer(ADDR, 0xC3, buf, 4);
    *x = (buf[0] & 0x3F);
    if (buf[1])
        *x = -*x;
    *y = (buf[2] & 0x3F);
    if (buf[3])
        *y = -*y;
}

uint8_t PAJ7620U2_Read_ObjectBrightness(void) {
    return PAJ7620U2_I2C_Read_Byte(ADDR, PAJ_OBJ_BRIGHTNESS);
}

uint16_t PAJ7620U2_Read_ObjectSize(void) {
    return m_I2C_Read_Word(ADDR, PAJ_OBJ_SIZE_L);
}

const char* PAJ7620U2_Get_Gesture_Name(PAJ_Gesture_t gesture) {
    switch (gesture) {
        case PAJ_NULL:
            return "GESTURE_NONE";
        case PAJ_UP:
            return "GESTURE_UP";
        case PAJ_DOWN:
            return "GESTURE_DOWN";
        case PAJ_LEFT:
            return "GESTURE_LEFT";
        case PAJ_RIGHT:
            return "GESTURE_RIGHT";
        case PAJ_FORWARD:
            return "GESTURE_FORWARD";
        case PAJ_BACKWARD:
            return "GESTURE_BACKWARD";
        case PAJ_CLOCKWISE:
            return "GESTURE_CLOCKWISE";
        case PAJ_COUNTER_CLOCKWISE:
            return "GESTURE_COUNTER_CLOCKWISE";
        case PAJ_WAVE:
            return "GESTURE_WAVE";
        default:
            return "GESTURE_MULTIPLE";
    }
}

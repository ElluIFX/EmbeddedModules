/**
 * @file paj7620u2.h
 * @brief PAJ7620U2 driver
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-10-14
 *
 * THINK DIFFERENTLY
 */

#ifndef __PAJ7620U2_H__
#define __PAJ7620U2_H__

#include "modules.h"

// i2c address
#define PAJ7620U2_I2C_ADDRESS (0x73)

typedef enum {
    PAJ_NULL = 0x00,
    PAJ_UP = 0x01,
    PAJ_DOWN = 0x02,
    PAJ_LEFT = 0x04,
    PAJ_RIGHT = 0x08,
    PAJ_FORWARD = 0x10,
    PAJ_BACKWARD = 0x20,
    PAJ_CLOCKWISE = 0x40,
    PAJ_COUNTER_CLOCKWISE = 0x80,
    PAJ_WAVE = 0x100,
} PAJ_Gesture_t;

typedef enum {
    PAJ_REPMODE_FAR_240FPS = 0,
    PAJ_REPMODE_FAR_120FPS,
    PAJ_REPMODE_NEAR_240FPS,
    PAJ_REPMODE_NEAR_120FPS,
} PAJ_ReportMode_t;

typedef enum {
    PAJ_MODE_NONE = 0,
    PAJ_MODE_GESTURE,
    PAJ_MODE_CURSOR,
} PAJ_Mode_t;

extern PAJ_Mode_t PAJ_Mode;

// Porting API
extern uint8_t PAJ7620U2_I2C_Read_Byte(uint8_t addr, uint8_t reg);
extern void PAJ7620U2_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data);
extern void PAJ7620U2_I2C_Read_Buffer(uint8_t addr, uint8_t reg, uint8_t* buf,
                                      uint16_t len);
// PAJ7620U2 API
extern uint8_t PAJ7620U2_Init(void);
extern void PAJ7620U2_Set_Enable(uint8_t enable);
extern void PAJ7620U2_Enter_Gesture_Mode(void);
extern void PAJ7620U2_Enter_Cursor_Mode(void);
extern uint8_t PAJ7620U2_Read_CursorInSight(void);
extern void PAJ7602_Read_CursorPos(int16_t* x, int16_t* y);
extern void PAJ7620U2_Set_AxisInvert(uint8_t x_invert, uint8_t y_invert);
extern void PAJ7620U2_Set_ReportMode(PAJ_ReportMode_t mode);
extern PAJ_Gesture_t PAJ7620U2_Read_Gesture(void);
extern uint8_t PAJ7620U2_Read_WaveCount(void);
extern uint8_t PAJ7620U2_Read_NoObjectCount(void);
extern uint8_t PAJ7620U2_Read_NoMotionCount(void);
extern uint8_t PAJ7620U2_Read_ObjectAvailable(void);
extern void PAJ7620U2_Read_ObjectCenterPos(uint16_t* x, uint16_t* y);
extern void PAJ7620U2_Read_ObjectVelocity(int16_t* x, int16_t* y);
extern uint8_t PAJ7620U2_Read_ObjectBrightness(void);
extern uint16_t PAJ7620U2_Read_ObjectSize(void);
extern const char* PAJ7620U2_Get_Gesture_Name(PAJ_Gesture_t gesture);

#endif /* __PAJ7620U2_H__ */

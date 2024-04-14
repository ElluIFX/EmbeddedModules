#ifndef _PAJ7620U2_
#define _PAJ7620U2_

// Register Bank select
#define PAJ_BANK_SELECT 0xEF /*Bank0=0x00,Bank1=0x01*/
// Register Bank 0
#define PAJ_SUSPEND \
    0x03 /*I2C suspend command (Write 0x01 to enter suspend state). I2C wake-up \
          command is slave ID wake-up. Refer to topic ¡°I2C Bus Timing      \
          Characteristics and Protocol¡±*/
#define PAJ_INT_FLAG1_MASK 0x41 /*Gesture detection interrupt flag mask*/
#define PAJ_INT_FLAG2_MASK 0x42 /*Gesture/PS detection interrupt flag mask*/
#define PAJ_INT_FLAG1 0x43      /*Gesture detection interrupt flag*/
#define PAJ_INT_FLAG2 0x44      /*Gesture/PS detection interrupt flag*/
#define PAJ_STATE \
    0x45 /*State indicator for gesture detection (Only functional at gesture \
          detection mode)*/
#define PAJ_PS_HIGH_THRESHOLD \
    0x69 /*PS hysteresis high threshold (Only functional at proximity detection \
          mode)*/
#define PAJ_PS_LOW_THRESHOLD \
    0x6A /*PS hysteresis low threshold (Only functional at proximity detection \
          mode)*/
#define PAJ_PS_APPROACH_STATE \
    0x6B /*PS approach state,  Approach = 1 , (8 bits PS data >= PS high \
          threshold),  Not Approach = 0 , (8 bits PS data <= PS low    \
          threshold)(Only functional at proximity detection mode)*/
#define PAJ_PS_DATA \
    0x6C /*PS 8 bit data(Only functional at gesture detection mode)*/
#define PAJ_OBJ_BRIGHTNESS 0xB0 /*Object Brightness (Max. 255)*/
#define PAJ_OBJ_SIZE_L 0xB1     /*Object Size(Low 8 bit)*/
#define PAJ_OBJ_SIZE_H 0xB2     /*Object Size(High 8 bit)*/

#define PAJ_CURSOR_X_L 0x3B /*Cursor X(Low 8 bit)*/
#define PAJ_CURSOR_X_H 0x3C /*Cursor X(High 8 bit)*/
#define PAJ_CURSOR_Y_L 0x3D /*Cursor Y(Low 8 bit)*/
#define PAJ_CURSOR_Y_H 0x3E /*Cursor Y(High 8 bit)*/

// Register Bank 1
#define PAJ_PS_GAIN \
    0x44 /*PS gain setting (Only functional at proximity detection mode)*/
#define PAJ_IDLE_S1_STEP_L \
    0x67 /*IDLE S1 Step, for setting the S1, Response Factor(Low 8 bit)*/
#define PAJ_IDLE_S1_STEP_H \
    0x68 /*IDLE S1 Step, for setting the S1, Response Factor(High 8 bit)*/
#define PAJ_IDLE_S2_STEP_L \
    0x69 /*IDLE S2 Step, for setting the S2, Response Factor(Low 8 bit)*/
#define PAJ_IDLE_S2_STEP_H \
    0x6A /*IDLE S2 Step, for setting the S2, Response Factor(High 8 bit)*/
#define PAJ_OPTOS1_TIME_L \
    0x6B /*OPtoS1 Step, for setting the OPtoS1 time of operation state to \
          standby 1 state(Low 8 bit)*/
#define PAJ_OPTOS2_TIME_H \
    0x6C /*OPtoS1 Step, for setting the OPtoS1 time of operation state to \
          standby 1 stateHigh 8 bit)*/
#define PAJ_S1TOS2_TIME_L \
    0x6D /*S1toS2 Step, for setting the S1toS2 time of standby 1 state to \
          standby 2 state(Low 8 bit)*/
#define PAJ_S1TOS2_TIME_H \
    0x6E /*S1toS2 Step, for setting the S1toS2 time of standby 1 state to \
          standby 2 stateHigh 8 bit)*/
#define PAJ_EN 0x72 /*Enable/Disable PAJ7620U2*/
// Gesture detection interrupt flag
// #define PAJ_UP 0x01
// #define PAJ_DOWN 0x02
// #define PAJ_LEFT 0x04
// #define PAJ_RIGHT 0x08
// #define PAJ_FORWARD 0x10
// #define PAJ_BACKWARD 0x20
// #define PAJ_CLOCKWISE 0x40
// #define PAJ_COUNTER_CLOCKWISE 0x80
// #define PAJ_WAVE 0x100
// Initialize array size
#define Init_Array sizeof(Init_Register_Array) / 2
#define Cursor_Array_SIZE sizeof(Init_Cursor_Array) / 2
#define Gesture_Array_SIZE sizeof(Init_Gesture_Array) / 2
// Power up initialize array
const unsigned char Init_Register_Array[][2] = {
    {0xEF, 0x00}, {0x37, 0x07}, {0x38, 0x17}, {0x39, 0x06}, {0x41, 0x00},
    {0x42, 0x00}, {0x46, 0x2D}, {0x47, 0x0F}, {0x48, 0x3C}, {0x49, 0x00},
    {0x4A, 0x1E}, {0x4C, 0x20}, {0x51, 0x10}, {0x5E, 0x10}, {0x60, 0x27},
    {0x80, 0x42}, {0x81, 0x44}, {0x82, 0x04}, {0x8B, 0x01}, {0x90, 0x06},
    {0x95, 0x0A}, {0x96, 0x0C}, {0x97, 0x05}, {0x9A, 0x14}, {0x9C, 0x3F},
    {0xA5, 0x19}, {0xCC, 0x19}, {0xCD, 0x0B}, {0xCE, 0x13}, {0xCF, 0x64},
    {0xD0, 0x21}, {0xEF, 0x01}, {0x02, 0x0F}, {0x03, 0x10}, {0x04, 0x02},
    {0x25, 0x01}, {0x27, 0x39}, {0x28, 0x7F}, {0x29, 0x08}, {0x3E, 0xFF},
    {0x5E, 0x3D}, {0x65, 0x96}, {0x67, 0x97}, {0x69, 0xCD}, {0x6A, 0x01},
    {0x6D, 0x2C}, {0x6E, 0x01}, {0x72, 0x01}, {0x73, 0x35}, {0x74, 0x00},
    {0x77, 0x01}, {0xEF, 0x00},
};
// Approaches register initialization array
const unsigned char Init_Cursor_Array[][2] = {
    {0xEF, 0x00}, {0x32, 0x29}, {0x33, 0x01}, {0x34, 0x00},
    {0x35, 0x01}, {0x36, 0x00}, {0x37, 0x03}, {0x38, 0x1B},
    {0x39, 0x03}, {0x3A, 0x1B},  // 03 1B
    {0x41, 0x00}, {0x42, 0x84}, {0x8B, 0x01}, {0x8C, 0x07},
    {0xEF, 0x01}, {0x04, 0x03}, {0x74, 0x03}, {0xEF, 0x00}};

// Gesture register initializes array
const unsigned char Init_Gesture_Array[][2] = {
    {0xEF, 0x00}, {0x41, 0x00}, {0x42, 0x00}, {0xEF, 0x00}, {0x48, 0x3C},
    {0x49, 0x00}, {0x51, 0x10}, {0x83, 0x20}, {0x9F, 0xF9}, {0xEF, 0x01},
    {0x01, 0x1E}, {0x02, 0x0F}, {0x03, 0x10}, {0x04, 0x02}, {0x41, 0x40},
    {0x43, 0x30}, {0x65, 0x96}, {0x66, 0x00}, {0x67, 0x97}, {0x68, 0x01},
    {0x69, 0xCD}, {0x6A, 0x01}, {0x6B, 0xB0}, {0x6C, 0x04}, {0x6D, 0x2C},
    {0x6E, 0x01}, {0x74, 0x00}, {0xEF, 0x00}, {0x41, 0xFF}, {0x42, 0x01},
    {0xEF, 0x00},
};

#endif

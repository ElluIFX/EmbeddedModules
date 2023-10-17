#ifndef __key_H
#define __key_H

#include "modules.h"

#define KEY_CHECK_MS 10         // 按键检测周期，单位ms
#define KEY_BUF_SIZE 16         // 按键事件FIFO大小
#define KEY_SHAKE_FILTER_MS 20  // 按键抖动滤波时间

/******************************************************************************
                           User Interface [START]
*******************************************************************************/

#define KEY_READ_UP 0x00
#define KEY_READ_DOWN 0x01

#define KEY_EVENT_NULL 0x0000             // 无事件
#define KEY_EVENT_DOWN 0x0001             // 按下事件
#define KEY_EVENT_SHORT 0x0002            // 短按事件
#define KEY_EVENT_LONG 0x0003             // 长按事件
#define KEY_EVENT_DOUBLE 0x0004           // 双击事件
#define KEY_EVENT_HOLD 0x0005             // 按住事件
#define KEY_EVENT_UP_HOLD 0x0006          // 按住后松开事件
#define KEY_EVENT_UP_DOUBLE 0x0007        // 双击后松开事件
#define KEY_EVENT_HOLD_CONTINUE 0x0008    // 按住连发事件
#define KEY_EVENT_DOUBLE_CONTINUE 0x0009  // 双击按住连发事件

#define KEY_DOWN(N) (KEY_EVENT_DOWN | N << 8)        // 按键N按下
#define KEY_SHORT(N) (KEY_EVENT_SHORT | N << 8)      // 按键N短按
#define KEY_LONG(N) (KEY_EVENT_LONG | N << 8)        // 按键N长按
#define KEY_DOUBLE(N) (KEY_EVENT_DOUBLE | N << 8)    // 按键N双击
#define KEY_HOLD(N) (KEY_EVENT_HOLD | N << 8)        // 按键N按住
#define KEY_UP_HOLD(N) (KEY_EVENT_UP_HOLD | N << 8)  // 按键N按住后松开
#define KEY_UP_DOUBLE(N) (KEY_EVENT_UP_DOUBLE | N << 8)  // 按键N双击后松开
// 按键N按住连发
#define KEY_HOLD_CONTINUE(N) (KEY_EVENT_HOLD_CONTINUE | N << 8)
// 按键N双击按住连发
#define KEY_DOUBLE_CONTINUE(N) (KEY_EVENT_DOUBLE_CONTINUE | N << 8)

/******************************************************************************
                           User Interface [END]
*******************************************************************************/

extern uint16_t key_set_long_ms;
extern uint16_t key_set_hold_ms;
extern uint16_t key_set_double_ms;
extern uint16_t key_set_continue_wait_ms;
extern uint16_t key_set_continue_send_ms;
extern uint16_t key_set_continue_send_speedup;
extern uint16_t key_set_continue_send_min_ms;

extern void Key_Init(uint8_t (*read_func)(uint8_t idx), uint8_t num);
extern void Key_Tick(void);
extern uint16_t Key_Read(void);
extern void Key_Register_Callback(void (*func)(uint16_t key_event));
extern void Key_Register_Callback_Alt(void (*func)(uint8_t key, uint8_t event));
extern char *Key_Get_Event_Name(uint8_t event);
#endif

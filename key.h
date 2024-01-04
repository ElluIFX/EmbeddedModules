#ifndef __key_H
#define __key_H

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

#define KEY_BUF_SIZE 16  // 按键事件FIFO大小

/******************************************************************************
                           User Interface [START]
*******************************************************************************/

#define KEY_READ_UP 0x00
#define KEY_READ_DOWN 0x01

#define KEY_EVENT_NULL 0x0000             // 无事件
#define KEY_EVENT_DOWN 0x0001             // 按下事件
#define KEY_EVENT_UP 0x0002               // 松开事件
#define KEY_EVENT_SHORT 0x0003            // 短按事件
#define KEY_EVENT_LONG 0x0004             // 长按事件
#define KEY_EVENT_DOUBLE 0x0005           // 双击事件
#define KEY_EVENT_DOUBLE_CONTINUE 0x0006  // 双击按住连发事件
#define KEY_EVENT_HOLD 0x0007             // 按住事件
#define KEY_EVENT_HOLD_CONTINUE 0x0008    // 按住连发事件

#define KEY_DOWN(N) (KEY_EVENT_DOWN | N << 8)      // 按键N按下
#define KEY_UP(N) (KEY_EVENT_UP | N << 8)          // 按键N松开
#define KEY_SHORT(N) (KEY_EVENT_SHORT | N << 8)    // 按键N短按
#define KEY_LONG(N) (KEY_EVENT_LONG | N << 8)      // 按键N长按
#define KEY_DOUBLE(N) (KEY_EVENT_DOUBLE | N << 8)  // 按键N双击
#define KEY_HOLD(N) (KEY_EVENT_HOLD | N << 8)      // 按键N按住
// 按键N按住连发
#define KEY_HOLD_CONTINUE(N) (KEY_EVENT_HOLD_CONTINUE | N << 8)
// 按键N双击按住连发
#define KEY_DOUBLE_CONTINUE(N) (KEY_EVENT_DOUBLE_CONTINUE | N << 8)

/******************************************************************************
                           User Interface [END]
*******************************************************************************/
#pragma pack(1)
typedef struct {
  uint16_t check_period_ms;   // 按键检测周期
  uint16_t shake_filter_ms;   // 按键抖动滤波时间
  uint8_t simple_event;       // 产生简单事件(按下/松开)
  uint8_t complex_event;      // 产生复杂事件(短按/长按/双击...)
  uint16_t long_ms;           // 长按时间
  uint16_t hold_ms;           // 按住时间
  uint16_t double_ms;         // 双击最大间隔时间 (0:关闭)
  uint16_t continue_wait_ms;  // 按住/双击按住连发等待时间 (0:关闭)
  uint16_t continue_send_ms;       // 按住/双击按住连发执行间隔
  uint16_t continue_send_speedup;  // 按住/双击按住连发执行加速
  uint16_t continue_send_min_ms;   // 按住/双击按住连发最小间隔
} key_setting_t;
#pragma pack()
extern key_setting_t key_setting;

/**
 * @brief 按键系统初始化
 * @param  read_func 按键读取函数(传入按键序号，返回按键状态(KEY_READ_UP/DOWN))
 * @param  num            总按键数量
 */
extern void Key_Init(uint8_t (*read_func)(uint8_t idx), uint8_t num);

/**
 * @brief 按键系统周期调用函数(key_setting.check_period_ms)
 */
extern void Key_Tick(void);

/**
 * @brief 按键读取函数
 * @retval uint16_t 按键事件(KEY_EVENT | KEY_ID<<8)
 */
extern uint16_t Key_Read(void);

/**
 * @brief 按键注册回调函数
 * @param  func            回调函数指针(传入同Key_Read的返回值)
 */
extern void Key_RegisterCallback(void (*func)(uint16_t key_event));

/**
 * @brief 按键注册回调函数(带按键序号)
 * @param  func            回调函数指针(分别传入按键序号和事件)
 */
extern void Key_RegisterCallbackAlt(void (*func)(uint8_t key, uint8_t event));

/**
 * @brief 获取按键事件名称字符串
 * @param  event           按键事件
 * @retval char*           按键事件名称字符串
 */
extern char *Key_GetEventName(uint8_t event);

/**
 * @brief 获取按键事件对应的按键ID
 * @param  key             按键序号
 * @retval uint8_t         按键ID
 */
#define Key_Get_Event_ID(key) ((key) >> 8)

#ifdef __cplusplus
}
#endif
#endif

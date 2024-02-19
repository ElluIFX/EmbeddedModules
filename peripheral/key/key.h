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

// 用户定义的按键读取层
// 1.定义读取函数:  KEY_ID -in-> Read_Func -out-> KEY_READ_UP/DOWN
// 2.初始化按键:    Key_Init(dev, Read_Func, ...)
#define KEY_READ_UP 0x00
#define KEY_READ_DOWN 0x01

// 读取方式1: 注册回调函数, 自动返回按键ID和事件类型
// 1.定义回调函数:     KEY_ID, KEY_EVENT_XXX -in-> Callback_Func
// 2.初始化时注册回调: Key_Init(..., Callback_Func)
#define KEY_EVENT_NULL 0x0000                // 无事件
#define KEY_EVENT_DOWN 0x0001                // 按下事件
#define KEY_EVENT_UP 0x0002                  // 松开事件
#define KEY_EVENT_SHORT 0x0003               // 短按事件
#define KEY_EVENT_LONG 0x0004                // 长按事件
#define KEY_EVENT_DOUBLE 0x0005              // 双击事件
#define KEY_EVENT_DOUBLE_REPEAT 0x0006       // 双击按住重复事件
#define KEY_EVENT_DOUBLE_REPEAT_STOP 0x0007  // 双击按住重复停止事件
#define KEY_EVENT_HOLD 0x0008                // 按住事件
#define KEY_EVENT_HOLD_REPEAT 0x0009         // 按住重复事件
#define KEY_EVENT_HOLD_REPEAT_STOP 0x000A    // 按住重复停止事件

// 读取方式2: 手动调用读取函数, 返回单值事件数据 (按键ID<<8 | 事件类型)
// 1.手动读取事件FIFO:     uint16_t event = Key_Read(dev);
// 2.判断是否有事件:       if(event!=KEY_EVENT_NULL)
// 2.使用宏解析事件数据:   switch(event) { case KEY_IS_XXX(KEY_XXX_ID): ... }
#define KEY_IS_DOWN(N) (KEY_EVENT_DOWN | N << 8)      // 按键N按下
#define KEY_IS_UP(N) (KEY_EVENT_UP | N << 8)          // 按键N松开
#define KEY_IS_SHORT(N) (KEY_EVENT_SHORT | N << 8)    // 按键N短按
#define KEY_IS_LONG(N) (KEY_EVENT_LONG | N << 8)      // 按键N长按
#define KEY_IS_DOUBLE(N) (KEY_EVENT_DOUBLE | N << 8)  // 按键N双击
#define KEY_IS_HOLD(N) (KEY_EVENT_HOLD | N << 8)      // 按键N按住
// 按键N按住重复
#define KEY_IS_HOLD_REPEAT(N) (KEY_EVENT_HOLD_REPEAT | N << 8)
// 按键N双击按住重复
#define KEY_IS_DOUBLE_REPEAT(N) (KEY_EVENT_DOUBLE_REPEAT | N << 8)
// 按键N双击按住重复停止
#define KEY_IS_DOUBLE_REPEAT_STOP(N) (KEY_EVENT_DOUBLE_REPEAT_STOP | N << 8)
// 按键N按住重复停止
#define KEY_IS_HOLD_REPEAT_STOP(N) (KEY_EVENT_HOLD_REPEAT_STOP | N << 8)

#pragma pack(1)
typedef struct {             // 按键驱动设置
  uint16_t check_period_ms;  // 按键检测周期 (Key_Tick调用周期)
  uint16_t shake_filter_ms;  // 按键抖动滤波周期 (N*check_period_ms)
  uint8_t simple_event;      // 产生简单事件(按下/松开)
  uint8_t complex_event;     // 产生复杂事件(短按/长按/双击...)
  uint16_t long_ms;          // 长按时间 (0:无长按事件)
  uint16_t hold_ms;          // 按住时间 (0:无按住事件)
  uint16_t double_ms;        // 双击最大间隔时间 (0:无双击事件)
  uint16_t repeat_wait_ms;  // 按住/双击按住重复等待时间 (0:无等待)
  uint16_t repeat_send_ms;  // 按住/双击按住重复执行间隔 (0:无重复事件)
  uint16_t repeat_send_speedup;  // 按住/双击按住重复执行加速 (0:无加速)
  uint16_t repeat_send_min_ms;  // 按住/双击按住重复最小间隔 (加速后)
} key_setting_t;

typedef struct __key_dev {
  struct {
    uint16_t value[KEY_BUF_SIZE];
    uint8_t rd;
    uint8_t wr;
  } key_buf;
  key_setting_t setting;
  uint8_t (*read_func)(uint8_t idx);
  void (*callback)(uint8_t key, uint8_t event);
  uint8_t key_num;
  struct __key {
    void (*status)(struct __key_dev *key_dev, uint8_t key_idx,
                   uint8_t key_read);
    uint16_t count_ms;
    uint16_t count_temp;
  } key_arr[];
} key_dev_t;
#pragma pack()

/******************************* 事件说明 *******************************
0.按键消抖(图表表示按键驱动读取结果KEY_READ_xxx):
0.1 有效的按下状态:
    UP  | #####
        |     #<-shake_filter_ms->
    DOWN|     ####################......

0.2 有效的松开状态:
    UP  |     ####################......
        |     #<-shake_filter_ms->
    DOWN| #####

1.简单事件:
  任意有效的按下/松开状态变化都会产生KEY_EVENT_DOWN/UP事件
  在保留消抖效果的同时, 可用于游戏控制等不需要多状态的应用

2.复杂事件( 图表表示按键状态, 省略消抖过程, 符号^标识事件KEY_EVENT_xxx触发点):
2.1 SHORT:短按, 按下后立即松开
    #####              #####
        #<- <long_ms ->#
        ################
                       ^SHORT

2.2 LONG:长按, 按下并保持一段时间后松开
    #####                          #####
        #<- >long_ms && <hold_ms ->#
        ############################
                                   ^LONG

2.3 DOUBLE:双击, 短按后一定间隔内再次按下
    #####              ##################
        #<- <long_ms ->#<- <double_ms ->#
        ################                #####......
                                        ^DOUBLE

2.4 REPEAT:双击按住重复, 双击并保持一定时间后开始重复触发, 松开触发停止事件
    ...####                                                           ####
          #<-repeat_wait_ms-><-repeat_send_ms0-><-repeat_send_ms1->   #
          #############################################################
          ^DOUBLE           ^DOUBLE_REPEAT     ^DOUBLE_REPEAT     ^...^...STOP
    (加速效果:repeat_send_msN = repeat_send_ms - N * repeat_send_speedup)

2.5 HOLD:按住, 按下不松开直到一定时间
    #####
        #<- hold_ms ->
        ###############......
                      ^KEY_EVENT_HOLD

2.6 REPEAT:按住重复, 按住后一定时间后开始重复触发, 松开触发停止事件
                                                                    ####
         <-repeat_wait_ms-><-repeat_send_ms0-><-repeat_send_ms1->   #
    ...##############################################################
         ^HOLD            ^HOLD_REPEAT       ^HOLD_REPEAT       ^...^...STOP
    (加速效果:repeat_send_msN = repeat_send_ms - N * repeat_send_speedup)

******************************************************************************/

/******************************************************************************
                           User Interface [END]
*******************************************************************************/

/**
 * @brief 按键系统初始化, 设置使用默认值
 * @param  key_dev        按键设备指针(NULL:尝试动态分配内存)
 * @param  read_func      读取函数(传入按键序号，返回按键状态(KEY_READ_UP/DOWN))
 * @param  num            总按键数量
 * @param  callback       事件回调函数(可选)
 * @retval key_dev_t*     按键设备指针(NULL:初始化失败/内存分配失败)
 */
extern key_dev_t *Key_Init(key_dev_t *key_dev,
                           uint8_t (*read_func)(uint8_t idx), uint8_t num,
                           void (*callback)(uint8_t key, uint8_t event));

/**
 * @brief 按键系统周期调用函数(key_setting.check_period_ms)
 * @param  key_dev        按键设备指针
 */
extern void Key_Tick(key_dev_t *key_dev);

/**
 * @brief 读取按键事件FIFO
 * @param  key_dev        按键设备指针
 * @retval uint16_t       按键事件(KEY_EVENT | KEY_ID<<8)
 */
extern uint16_t Key_Read(key_dev_t *key_dev);

/**
 * @brief 读取按键按下状态
 * @param  key_dev       按键设备指针
 * @param  key           按键序号
 * @retval uint8_t       按键状态(KEY_READ_UP/DOWN)
 */
extern uint8_t Key_ReadRaw(key_dev_t *key_dev, uint8_t key);

/**
 * @brief 获取按键事件名称字符串
 * @param  event           按键事件
 * @retval char*           事件名称字符串
 */
extern char *Key_GetEventName(uint16_t event);

/**
 * @brief 获取按键事件对应的按键ID
 * @param  event           按键事件(从Key_Read读取)
 * @retval uint8_t         按键ID
 */
#define Key_Get_Event_ID(event) ((event) >> 8)

#ifdef __cplusplus
}
#endif
#endif

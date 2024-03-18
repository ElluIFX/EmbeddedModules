#ifndef __key_H
#define __key_H

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

/******************************************************************************
                           User Interface [START]
*******************************************************************************/

// 用户定义的按键读取层
// 1.定义读取函数:  KEY_ID -in-> Read_Func -out-> KEY_READ_UP/DOWN
// 2.初始化按键:    key_init(dev, Read_Func, ...)
typedef enum {
  KEY_READ_UP = 0,
  KEY_READ_DOWN = 1,
} key_read_e;

// 读取方式1: 注册回调函数, 自动返回按键ID和事件类型
// 1.定义回调函数:     KEY_ID, KEY_EVENT_XXX -in-> Callback_Func
// 2.初始化时注册回调: key_init(..., Callback_Func)
#define KEY_EVENT_NULL 0x0000                // 无事件
#define KEY_EVENT_DOWN 0x0001                // 按下事件
#define KEY_EVENT_UP 0x0002                  // 松开事件
#define KEY_EVENT_SHORT 0x0009               // 短按事件
#define KEY_EVENT_LONG 0x0003                // 长按事件
#define KEY_EVENT_HOLD 0x0004                // 按住事件
#define KEY_EVENT_HOLD_REPEAT 0x0005         // 按住重复事件
#define KEY_EVENT_HOLD_REPEAT_STOP 0x0006    // 按住重复停止事件
#define KEY_EVENT_DOUBLE 0x000A              // 双击事件
#define KEY_EVENT_DOUBLE_REPEAT 0x0007       // 双击按住重复事件
#define KEY_EVENT_DOUBLE_REPEAT_STOP 0x0008  // 双击按住重复停止事件
#define KEY_EVENT_MULTI(N) (0x000B + (N)-3)  // 多击N次事件(最多63次)

// 读取方式2: 手动调用读取函数, 返回单值事件数据 (按键ID<<8 | 事件类型)
// 1.手动读取事件FIFO:     uint16_t event = key_read_event(dev);
// 2.判断是否有事件:       if(event!=KEY_EVENT_NULL)
// 2.使用宏判断事件来源:   switch(event) { case KEYx_IS_XXX(KEY_XXX_ID): ... }
#define KEYx_IS_DOWN(IDx) (KEY_EVENT_DOWN | IDx << 8)      // 按键x按下
#define KEYx_IS_UP(IDx) (KEY_EVENT_UP | IDx << 8)          // 按键x松开
#define KEYx_IS_SHORT(IDx) (KEY_EVENT_SHORT | IDx << 8)    // 按键x短按
#define KEYx_IS_LONG(IDx) (KEY_EVENT_LONG | IDx << 8)      // 按键x长按
#define KEYx_IS_DOUBLE(IDx) (KEY_EVENT_DOUBLE | IDx << 8)  // 按键x双击
#define KEYx_IS_HOLD(IDx) (KEY_EVENT_HOLD | IDx << 8)      // 按键x按住
// 按键x按住重复
#define KEYx_IS_HOLD_REPEAT(IDx) (KEY_EVENT_HOLD_REPEAT | IDx << 8)
// 按键x双击按住重复
#define KEYx_IS_DOUBLE_REPEAT(IDx) (KEY_EVENT_DOUBLE_REPEAT | IDx << 8)
// 按键x双击按住重复停止
#define KEYx_IS_DOUBLE_REPEAT_STOP(IDx) \
  (KEY_EVENT_DOUBLE_REPEAT_STOP | IDx << 8)
// 按键x按住重复停止
#define KEYx_IS_HOLD_REPEAT_STOP(IDx) (KEY_EVENT_HOLD_REPEAT_STOP | IDx << 8)
// 按键x多击N次
#define KEYx_IS_MULTI(IDx, N) (KEY_EVENT_MULTI(N) | IDx << 8)

#pragma pack(1)
typedef struct {              // 按键设备设置(key_dev->setting)
  uint16_t check_period_ms;   // 按键检测周期 (Key_Tick调用周期)
  uint16_t shake_filter_ms;   // 按键抖动滤波周期 (N*check_period_ms)
  uint8_t simple_event : 1;   // 产生简单事件(按下/松开)
  uint8_t complex_event : 1;  // 产生复杂事件(短按/长按/双击...)
  uint8_t multi_max : 6;  // 多击最大次数 (<3:禁用多击事件, 最大63)
  uint16_t long_ms;       // 长按时间 (0:无长按事件)
  uint16_t hold_ms;       // 按住时间 (0:无按住事件)
  uint16_t multi_ms;      // 多击最大间隔时间 (0:无多击事件)
  uint16_t repeat_wait_ms;  // 按住/双击按住重复等待时间 (0:无等待)
  uint16_t repeat_send_ms;  // 按住/双击按住重复执行间隔 (0:无重复事件)
  uint16_t repeat_send_speedup;  // 按住/双击按住重复执行加速 (0:无加速)
  uint16_t repeat_send_min_ms;  // 按住/双击按住重复最小间隔 (加速后)
} key_setting_t;

#define KEY_BUF_SIZE 16  // 按键事件FIFO大小

typedef struct __key_dev {  // 按键设备结构体
  struct {                  // 事件FIFO
    uint16_t value[KEY_BUF_SIZE];
    uint8_t rd;
    uint8_t wr;
  } event_fifo;
  uint8_t key_num;                               // 按键数量
  key_setting_t setting;                         // 设备设置
  key_read_e (*read_func)(uint8_t id);           // 读取函数
  void (*callback)(uint8_t key, uint8_t event);  // 事件回调函数
  struct __key {                                 // 按键状态机数组
    void (*state)(struct __key_dev *, uint8_t, uint8_t);  // 状态机函数
    uint16_t count_ms;                                    // 按键计时
    uint16_t count_temp;                                  // 消抖计时
    uint8_t multi_count;                                  // 多击计数
  } key_arr[];
} key_dev_t;
#pragma pack()

// 默认按键设置
#define default_key_setting                                             \
  {                                                                     \
    .check_period_ms = 10, .shake_filter_ms = 20, .simple_event = 1,    \
    .complex_event = 1, .multi_max = 6, .long_ms = 300, .hold_ms = 800, \
    .multi_ms = 200, .repeat_wait_ms = 600, .repeat_send_ms = 100,      \
    .repeat_send_speedup = 4, .repeat_send_min_ms = 10,                 \
  }

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

2.复杂事件(图表表示按键状态, 省略消抖过程, 符号^标识事件KEY_EVENT_xxx触发点):
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
    #####              #################                     ################
        #<- <long_ms ->#<- <multi_ms ->#<- <repeat_wait_ms ->#`<- >multi_ms ->
        ################               #######################
                                       ^DOUBLE                       *DOUBLE^
    (*:当multi_max>=3时需要等待抬起后判断多击才能触发双击事件, 这不影响重复事件)

2.4 DREPEAT:双击按住重复, 双击并保持一定时间后开始重复触发, 松开触发停止事件
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

2.6 HREPEAT:按住重复, 按住后一定时间后开始重复触发, 松开触发停止事件
                                                                    ####
         <-repeat_wait_ms-><-repeat_send_ms0-><-repeat_send_ms1->   #
    ...##############################################################
         ^HOLD            ^HOLD_REPEAT       ^HOLD_REPEAT       ^...^...STOP
    (加速效果:repeat_send_msN = repeat_send_ms - N * repeat_send_speedup)

2.7 MULTI:多击事件(>=三击), 与双击逻辑类似, 但不支持重复
    #####          #############                 #############     ############
        # <long_ms # <multi_ms # <repeat_wait_ms # <multi_ms # any # >multi_ms
        ############           ###################           #######
                                                    *MULTI(3)^        MULTI(3)^
    (*:当按键次数N==multi_max时, 会在下降沿直接触发多击事件, 不等待抬起)

******************************************************************************/

/******************************************************************************
                           User Interface [END]
*******************************************************************************/

// 静态声明一个按键设备(指针)(由于结构体包含柔性数组, 因此必须使用该宏声明)
#define KEY_DEV_DEF(name, num)                                         \
  static uint8_t __key_buf_##name[sizeof(key_dev_t) +                  \
                                  (num) * sizeof(struct __key)] = {0}; \
  key_dev_t *name = (key_dev_t *)__key_buf_##name

/**
 * @brief 按键系统初始化  (设置初始化为默认值)
 * @param  key_dev        按键设备指针(NULL:尝试动态分配内存)
 * @param  read_func      读取函数(传入按键ID，返回按键状态(`KEY_READ_UP/DOWN`))
 * @param  num            总按键数量
 * @param  callback       事件回调函数(可选)
 * @retval key_dev_t*     按键设备指针(NULL:初始化失败/内存分配失败)
 * @note  清理时可直接free(key_dev)
 */
extern key_dev_t *key_init(key_dev_t *key_dev,
                           key_read_e (*read_func)(uint8_t id), uint8_t num,
                           void (*callback)(uint8_t key, uint8_t event));

/**
 * @brief 按键系统初始化  (使用自定义设置)
 * @param  key_dev        按键设备指针(NULL:尝试动态分配内存)
 * @param  read_func      读取函数(传入按键ID，返回按键状态(`KEY_READ_UP/DOWN`))
 * @param  num            总按键数量
 * @param  callback       事件回调函数(可选)
 * @param  setting        按键设备设置
 * @retval key_dev_t*     按键设备指针(NULL:初始化失败/内存分配失败)
 * @note  清理时可直接free(key_dev)
 */
extern key_dev_t *key_init_with_setting(
    key_dev_t *key_dev, key_read_e (*read_func)(uint8_t id), uint8_t num,
    void (*callback)(uint8_t key, uint8_t event), key_setting_t setting);

/**
 * @brief 按键系统处理函数
 * @param  key_dev        按键设备指针
 * @note 调用周期必须匹配setting.check_period_ms
 */
extern void key_tick(key_dev_t *key_dev);

/**
 * @brief 读取按键事件FIFO
 * @param  key_dev        按键设备指针
 * @retval uint16_t       单值按键事件(按键ID<<8 | 事件类型)
 * @note 使用宏KEYx_IS_xxx判断事件
 */
extern uint16_t key_read_event(key_dev_t *key_dev);

/**
 * @brief 获取按键按下状态(包含消抖)
 * @param  key_dev       按键设备指针
 * @param  key           按键序号
 * @retval uint8_t       按键状态(KEY_READ_UP/DOWN)
 */
extern uint8_t key_read_raw(key_dev_t *key_dev, uint8_t key);

/**
 * @brief 获取按键事件名称字符串
 * @param  event           按键事件
 * @retval char*           事件名称字符串
 */
extern const char *key_get_event_name(uint16_t event);

/**
 * @brief 获取单值按键事件对应的按键ID
 * @param  event           按键事件(必须从Key_Read读取)
 * @retval uint8_t         按键ID
 */
#define key_get_event_id(event) ((event) >> 8)

/**
 * @brief 获取单值按键事件对应的事件类型
 * @param  event           按键事件(必须从Key_Read读取)
 * @retval uint8_t         事件类型
 */
#define key_get_event_type(event) ((event) & 0xFF)

/**
 * @brief 判断是否为多击事件
 * @param  event           按键事件
 */
#define key_is_multi_event(event) ((event & 0xFF) >= KEY_EVENT_MULTI(3))

/**
 * @brief 获取多击事件的次数
 * @param  event           按键事件
 * @retval uint8_t         多击次数(<3:非多击事件)
 */
#define key_get_multi_event_num(event)                                        \
  ((event & 0xFF) >= KEY_EVENT_MULTI(1) ? (event & 0xFF) - KEY_EVENT_MULTI(0) \
                                        : 0)

#ifdef __cplusplus
}
#endif
#endif

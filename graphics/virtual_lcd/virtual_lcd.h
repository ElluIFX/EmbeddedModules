/**
 * @file virtual_lcd.h
 * @brief 与上位机通信，实现虚拟的lcd屏幕
 * @author Ellu (ellu.grif@gmail.com)
 * @date 2024-02-01
 *
 * THINK DIFFERENTLY
 */

#ifndef __VIRTUAL_LCD_H__
#define __VIRTUAL_LCD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

// Public Defines ---------------------------

// 颜色格式
#define VLCD_COLORFORMAT_RGB565 0x00
#define VLCD_COLORFORMAT_RGB888 0x01
#define VLCD_COLORFORMAT_MONO 0x02
#define VLCD_COLORFORMAT_MONO_INV 0x03
#define VLCD_COLORFORMAT_GRAY_8BIT 0x04

// 旋转角度
#define VLCD_ROTATE_0 0x00
#define VLCD_ROTATE_90 0x01
#define VLCD_ROTATE_180 0x02
#define VLCD_ROTATE_270 0x03

// 启用的输入设备
#define VLCD_FLAG_INDEV_KEYBOARD (1 << 0)
#define VLCD_FLAG_INDEV_TOUCH (1 << 1)
#define VLCD_FLAG_INDEV_MOUSE (1 << 2)
#define VLCD_FLAG_INDEV_BUTTON (1 << 3)
#define VLCD_FLAG_INDEV_ENCODER (1 << 4)

// 键盘动作
#define VLCD_INPKT_KEY_ACTION_UP 0x00
#define VLCD_INPKT_KEY_ACTION_DOWN 0x01
#define VLCD_INPKT_KEY_ACTION_REPEAT_FLAG 0x02

// 鼠标按键
#define VLCD_FLAG_MOUSEKEY_LEFT (1 << 0)
#define VLCD_FLAG_MOUSEKEY_RIGHT (1 << 1)
#define VLCD_FLAG_MOUSEKEY_MIDDLE (1 << 2)
#define VLCD_FLAG_MOUSEKEY_M1 (1 << 3)
#define VLCD_FLAG_MOUSEKEY_M2 (1 << 4)
#define VLCD_FLAG_MOUSEKEY_M3 (1 << 5)
#define VLCD_FLAG_MOUSEKEY_M4 (1 << 6)

// Public Typedefs --------------------------

// Exported Variables -----------------------

// Exported Macros --------------------------

// Exported Functions -----------------------

/**
 * @brief 由用户实现的统一数据发送函数
 * @param  data     将要发送的数据指针
 * @param  length   数据长度
 * @note 如果发送方式是DMA，需要自行复制数据，上层函数不保证数据存活期
 */
void vlcd_send_data_handler(uint8_t* data, uint32_t length);

/**
 * @brief 由用户调用的统一数据接收函数
 * @param  data     接收到的数据指针
 * @param  length   数据长度
 */
void vlcd_recv_data_handler(uint8_t* data, uint32_t length);

/**
 * @brief 初始化虚拟屏幕
 * @param  width       屏幕宽度
 * @param  height      屏幕高度
 * @param  format      屏幕颜色格式（见VLCD_COLORFORMAT_XXX）
 * @param  rotate      屏幕旋转角度（见VLCD_ROTATE_XXX）
 * @param  indev_flags 启用输入设备（见VLCD_FLAG_INDEV_XXX）
 * @note 可同时启用多种输入设备
 * @note 默认绘制窗口初始化为全屏
 */
void vlcd_init_screen(uint16_t width, uint16_t height, uint8_t format,
                      uint8_t rotate, uint8_t indev_flags);

/**
 * @brief 设置虚拟屏幕的绘制窗口，供后续stream_data使用
 * @param  x         窗口左上角x坐标
 * @param  y         窗口左上角y坐标
 * @param  width     窗口宽度
 * @param  height    窗口高度
 */
void vlcd_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/**
 * @brief 流式传输数据到虚拟屏幕
 * @param  data      数据指针
 * @param  length    数据长度
 */
void vlcd_stream_data(uint8_t* data, uint32_t length);

/**
 * @brief 绘制数据到虚拟屏幕, 指定绘制区域（等效同时执行set_window +
 * stream_data）
 * @param  x         左上角x坐标
 * @param  y         左上角y坐标
 * @param  width     宽度
 * @param  height    高度
 * @param  data      数据指针
 * @param  length    数据长度
 */
void vlcd_draw_data(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint8_t* data, uint32_t length);

/**
 * @brief 绘制单个像素到虚拟屏幕（十分低效）
 * @param  x         x坐标
 * @param  y         y坐标
 * @param  color     颜色值
 */
void vlcd_draw_pixel(uint16_t x, uint16_t y, uint32_t color);

/**
 * @brief 键盘事件回调函数
 * @param  action    动作（见VLCD_INPKT_KEY_ACTION_XXX）
 * @param  scancode  扫描码 (标准键盘扫描码)
 * @param  modifier  附加键状态
 * @param  ascii     键值ascii码, 0表示无ascii码
 */
void vlcd_keyboard_callback(uint8_t action, uint16_t scancode, uint8_t modifier,
                            uint8_t ascii);

/**
 * @brief 触摸事件回调函数
 * @param  x         x坐标
 * @param  y         y坐标
 * @param  touched   触摸是否按下
 * @note   与鼠标不同，touched发布一次0后不会再收到触摸事件
 */
void vlcd_touch_callback(uint16_t x, uint16_t y, uint8_t touched);

/**
 * @brief 鼠标事件回调函数
 * @param  x         x坐标
 * @param  y         y坐标
 * @param  wheel     鼠标滚轮(正数为向上滚动，负数为向下滚动)
 * @param  mousekey  鼠标按键状态(见VLCD_FLAG_MOUSEKEY_XXX)
 */
void vlcd_mouse_callback(uint16_t x, uint16_t y, int8_t wheel,
                         uint8_t mousekey);

/**
 * @brief 按键事件回调函数
 * @param  button    按键编号
 * @param  pressed   按键是否按下
 */
void vlcd_button_callback(uint8_t button, uint8_t pressed);

/**
 * @brief 编码器事件回调函数
 * @param  diff      编码器转动差(正数为顺时针，负数为逆时针)
 * @param  pressed   编码器按键是否按下
 */
void vlcd_encoder_callback(int8_t diff, uint8_t pressed);

/**
 * @brief 虚拟屏幕刷新完成回调函数
 */
void vlcd_flush_callback(void);

#ifdef __cplusplus
}
#endif

#endif /* __VIRTUAL_LCD__ */

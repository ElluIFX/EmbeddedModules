/**
 * @file led.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-05-15
 *
 * THINK DIFFERENTLY
 */

#ifndef __LED_H__
#define __LED_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "modules.h"

#if !KCONFIG_AVAILABLE
#define LED_CFG_USE_PWM 0                // 是否使用PWM控制RGB灯
#define LED_CFG_R_HTIM htim8             // 红灯PWM定时器
#define LED_CFG_G_HTIM htim8             // 绿灯PWM定时器
#define LED_CFG_B_HTIM htim8             // 蓝灯PWM定时器
#define LED_CFG_R_CHANNEL TIM_CHANNEL_1  // 红灯PWM通道
#define LED_CFG_G_CHANNEL TIM_CHANNEL_2  // 绿灯PWM通道
#define LED_CFG_B_CHANNEL TIM_CHANNEL_3  // 蓝灯PWM通道
#define LED_CFG_R_PULSE 400              // 红灯最大比较值
#define LED_CFG_G_PULSE 650              // 绿灯最大比较值
#define LED_CFG_B_PULSE 800              // 蓝灯最大比较值
#define LED_CFG_PWMN_OUTPUT 1            // 互补输出

#endif  // !KCONFIG_AVAILABLE

#define ENABLE 0x01
#define DISABLE 0x00
#define IGNORE 0x02
#define TOGGLE 0xFF

#if LED_CFG_USE_PWM

/**
 * @brief LED控制
 * @param  R/G/B: 0-255.0f, <0-忽略
 */
extern void LED(float R, float G, float B);
#else
#if defined(LED_R_Pin) && defined(LED_G_Pin) && defined(LED_B_Pin)

/**
 * @brief LED控制
 * @param  R/G/B: 0-熄灭 1-点亮 0xFF-翻转 其他-忽略
 */
extern void LED(uint8_t R, uint8_t G, uint8_t B);
#elif defined(LED_Pin)

/**
 * @brief LED控制
 * @param  act: 0-熄灭 1-点亮 0xFF-翻转 其他-忽略
 */
extern void LED(uint8_t act);
#endif
#endif  // !_PWM_RGB_LED

#ifdef __cplusplus
}
#endif
#endif  // __LED_H__

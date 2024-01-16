/**
 * @file led.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-05-15
 *
 * THINK DIFFERENTLY
 */

#ifndef _LED_H_
#define _LED_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "modules.h"

#define ENABLE 0x01
#define DISABLE 0x00
#define IGNORE 0x02
#define TOGGLE 0xFF

#if _LED_USE_PWM

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
#endif

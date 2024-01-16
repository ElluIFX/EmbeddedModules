/**
 * @file led.c
 * @brief LED控制
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-05-15
 *
 * THINK DIFFERENTLY
 */

#include "led.h"

#if !_LED_USE_PWM
#define LED_STATE(s) (s ? GPIO_PIN_RESET : GPIO_PIN_SET)

#if defined(LED_R_Pin) && defined(LED_G_Pin) && defined(LED_B_Pin)
void LED(uint8_t R, uint8_t G, uint8_t B) {
  if (R < IGNORE)
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, LED_STATE(R));
  else if (R == TOGGLE)
    HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
  if (G < IGNORE)
    HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, LED_STATE(G));
  else if (G == TOGGLE)
    HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
  if (B < IGNORE)
    HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, LED_STATE(B));
  else if (B == TOGGLE)
    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
}
#elif defined(LED_Pin)
void LED(uint8_t act) {
  if (act < IGNORE)
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_STATE(act));
  else if (act == TOGGLE)
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

#endif

#else
#include "tim.h"
void LED(float R, float G, float B) {
  static uint8_t inited = 0;
  if (R >= 0)
    __HAL_TIM_SET_COMPARE(&_LED_R_HTIM, _LED_R_CHANNEL,
                          R * _LED_R_PULSE / 255.0f);
  if (G >= 0)
    __HAL_TIM_SET_COMPARE(&_LED_G_HTIM, _LED_G_CHANNEL,
                          G * _LED_G_PULSE / 255.0f);
  if (B >= 0)
    __HAL_TIM_SET_COMPARE(&_LED_B_HTIM, _LED_B_CHANNEL,
                          B * _LED_B_PULSE / 255.0f);
  if (!inited) {
#if !_LED_PWMN_OUTPUT
    HAL_TIM_PWM_Start(&_LED_R_HTIM, _LED_R_CHANNEL);
    HAL_TIM_PWM_Start(&_LED_G_HTIM, _LED_G_CHANNEL);
    HAL_TIM_PWM_Start(&_LED_B_HTIM, _LED_B_CHANNEL);
#else
    HAL_TIMEx_PWMN_Start(&_LED_R_HTIM, _LED_R_CHANNEL);
    HAL_TIMEx_PWMN_Start(&_LED_G_HTIM, _LED_G_CHANNEL);
    HAL_TIMEx_PWMN_Start(&_LED_B_HTIM, _LED_B_CHANNEL);
#endif
    inited = 1;
  }
}

#endif  // !_PWM_RGB_LED

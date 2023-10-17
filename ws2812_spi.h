/**
 * @file ws2812_spi.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-07-22
 *
 * THINK DIFFERENTLY
 */

#ifndef __WS2812_SPI_H__
#define __WS2812_SPI_H__
#include "modules.h"
#include "spi.h"

typedef struct {
  uint8_t* buffer;  // 缓冲区
  uint16_t length;  // 灯带长度
  SPI_HandleTypeDef* hspi;
} LEDStrip_t;

extern HAL_StatusTypeDef Strip_Init(LEDStrip_t* strip, uint16_t length,
                                    SPI_HandleTypeDef* hspi);
extern void Strip_DeInit(LEDStrip_t* strip);
extern void Strip_Set(LEDStrip_t* strip, uint16_t index, uint32_t RGBcolor);
extern void Strip_Clear(LEDStrip_t *strip);
extern void Strip_Set_Range(LEDStrip_t* strip, uint16_t start, uint16_t end,
                               uint32_t RGBcolor);
extern uint8_t Strip_IsBusy(LEDStrip_t* strip);
extern HAL_StatusTypeDef Strip_Send(LEDStrip_t* strip);
extern HAL_StatusTypeDef Strip_SendPart(LEDStrip_t *strip, uint16_t num);
extern void Strip_Send_Blocking(LEDStrip_t* strip);
extern uint32_t HSV_To_RGB(float h, uint8_t s, uint8_t v);
#endif

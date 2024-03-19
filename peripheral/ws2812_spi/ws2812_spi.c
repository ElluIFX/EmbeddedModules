/**,,,,
 * @file ws2812_spi.c
 * @brief WS2812B驱动(使用SPI)
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-07-22
 *
 * THINK DIFFERENTLY
 */

#include "ws2812_spi.h"

#if __has_include("spi.h")
#include "log.h"
#include "spi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

// SPI模拟一个bit要发送的数据定义:
// T0H: 350ns T0L: 800ns
// T1H: 700ns T1L: 600ns
// 发送间隔: 50us 允许误差: 150ns
// 根据SPI时钟频率计算得:
// SPI 4Mbps: LEN=4 B1=0b1110 B0=0b1000
// SPI 3Mbps: LEN=3 B1=0b110  B0=0b100
#define BIT_LEN 3  // BIT长度 / bits
#define BIT1 0b110
#define BIT0 0b100
#define BUF_LEN(n) ((n) * BIT_LEN * 3)  // n个灯需要的缓冲区长度 / bytes

#define IS_GRB_FORMAT 1  // 0: RGB 1: GRB

// SPI空闲时MOSI总线为高电平, 这可能造成第一个数据发送错误
// 首尾灯珠不正常时, 在数据前后添加一定数量的低电平或添加一个额外的灯珠,
// 根据实际情况调整
#define HEAD_ZERO 12
#define TAIL_ZERO 12

HAL_StatusTypeDef ws2812_init(ws2812_strip_t *strip, uint16_t length,
                              SPI_HandleTypeDef *hspi) {
  if (hspi == NULL) hspi = strip->hspi;
  if (strip->buffer != NULL || strip->length != 0) {
    LOG_WARN("LED REINIT");  // 最好手动调用Strip_DeInit再重新初始化
    ws2812_deinit(strip);
  }
  strip->buffer = m_alloc(BUF_LEN(length) + HEAD_ZERO + TAIL_ZERO);
  if (strip->buffer == NULL) {
    LOG_ERROR("LED MALLOC FAILED");
    return HAL_ERROR;
  }
  strip->length = length;
  strip->hspi = hspi;
#if HEAD_ZERO || TAIL_ZERO
  memset(strip->buffer, 0, BUF_LEN(length) + HEAD_ZERO + TAIL_ZERO);
#endif
  ws2812_set_range(strip, 0, length - 1, 0x000000);
  return HAL_OK;
}

void ws2812_deinit(ws2812_strip_t *strip) {
  if (strip->buffer != NULL) {
    m_free(strip->buffer);
  }
  strip->length = 0;
}

void ws2812_set(ws2812_strip_t *strip, uint16_t index, uint32_t color) {
  if (index >= strip->length || (!strip->buffer)) return;  // overrun check
  uint8_t *buf = strip->buffer + index * BIT_LEN * 3 + HEAD_ZERO;
#if IS_GRB_FORMAT
  color =
      (color & 0x00FF00) << 8 | (color & 0xFF0000) >> 8 | (color & 0x0000FF);
#endif
  memset(buf, 0, BIT_LEN * 3);
  for (uint16_t bit_offset = 0; bit_offset < BIT_LEN * 8 * 3; bit_offset++) {
    if (color & (((uint32_t)1) << (23 - bit_offset / BIT_LEN))) {
      buf[bit_offset / 8] |=
          ((BIT1 >> (BIT_LEN - 1 - bit_offset % BIT_LEN)) & 0x01)
          << (7 - bit_offset % 8);
    } else {
      buf[bit_offset / 8] |=
          (((BIT0 >> (BIT_LEN - 1 - bit_offset % BIT_LEN)) & 0x01)
           << (7 - bit_offset % 8));
    }
  }
}

void ws2812_set_range(ws2812_strip_t *strip, uint16_t start, uint16_t end,
                      uint32_t RGBcolor) {
  if (!strip->length) return;
  if (end >= strip->length) end = strip->length - 1;
  for (uint16_t i = start; i <= end; i++) {
    ws2812_set(strip, i, RGBcolor);
  }
}

void ws2812_clear(ws2812_strip_t *strip) {
  ws2812_set_range(strip, 0, strip->length - 1, 0x000000);
}

uint8_t ws2812_is_busy(ws2812_strip_t *strip) {
  return (HAL_SPI_GetState(strip->hspi) != HAL_SPI_STATE_READY ||
          HAL_DMA_GetState(strip->hspi->hdmatx) != HAL_DMA_STATE_READY);
}

void ws2812_send_blocking(ws2812_strip_t *strip) {
  if (!strip->length || !strip->buffer) return;
  while (HAL_SPI_GetState(strip->hspi) != HAL_SPI_STATE_READY) {
    __NOP();
  }
  HAL_SPI_Transmit(strip->hspi, strip->buffer,
                   BUF_LEN(strip->length) + HEAD_ZERO + TAIL_ZERO, 100);
}

HAL_StatusTypeDef ws2812_send(ws2812_strip_t *strip) {
  if (!strip->length || !strip->buffer) return HAL_ERROR;
  if (HAL_SPI_GetState(strip->hspi) != HAL_SPI_STATE_READY ||
      HAL_DMA_GetState(strip->hspi->hdmatx) != HAL_DMA_STATE_READY) {
    LOG_ERROR("LED SPI BUSY");
    return HAL_BUSY;
  }
  HAL_SPI_Transmit_DMA(strip->hspi, strip->buffer,
                       BUF_LEN(strip->length) + HEAD_ZERO + TAIL_ZERO);
  return HAL_OK;
}

HAL_StatusTypeDef ws2812_send_part(ws2812_strip_t *strip, uint16_t num) {
  if (!strip->length || !strip->buffer) return HAL_ERROR;
  if (num > strip->length) num = strip->length;
  if (HAL_SPI_GetState(strip->hspi) != HAL_SPI_STATE_READY ||
      HAL_DMA_GetState(strip->hspi->hdmatx) != HAL_DMA_STATE_READY) {
    LOG_ERROR("LED SPI BUSY");
    return HAL_BUSY;
  }
  HAL_SPI_Transmit_DMA(strip->hspi, strip->buffer,
                       BUF_LEN(num) + HEAD_ZERO + TAIL_ZERO);
  return HAL_OK;
}

// R,G,B range 0-255, H range 0-360, S,V range 0-100
uint32_t hsv2rgb(float h, uint8_t s, uint8_t v) {
  uint8_t r, g, b;

#if 0  // S,V range 0-100
  float max = v * 2.55f;
  float min = max * (100 - s) / 100.0f;
#else  // S,V range 0-255
  float max = v;
  float min = max * (255 - s) / 255.0f;
#endif

#if 0  // h is uint16_t
  float adj = (max - min) * (h % 60) / 60.0f;
  switch (h / 60) {
#else  // h is float
  float adj = (max - min) * (h - ((int)h / 60) * 60.0f) / 60.0f;
  switch ((int)h / 60) {
#endif
    case 0:
      r = max;
      g = min + adj;
      b = min;
      break;
    case 1:
      r = max - adj;
      g = max;
      b = min;
      break;
    case 2:
      r = min;
      g = max;
      b = min + adj;
      break;
    case 3:
      r = min;
      g = max - adj;
      b = max;
      break;
    case 4:
      r = min + adj;
      g = min;
      b = max;
      break;
    default:  // case 5:
      r = max;
      g = min;
      b = max - adj;
      break;
  }

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

#endif  // __has_include("spi.h")

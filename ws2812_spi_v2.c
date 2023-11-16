/**,,,,
 * @file ws2812_spi_v2.c
 * @brief WS2812B驱动(使用SPI), 利用SPI半发送完成中断实现循环装载数据,
 * 降低内存占用
 * @author Ellu (ellu.grif@gmail.com)
 * @version 2.0
 * @date 2023-07-22
 *
 * THINK DIFFERENTLY
 */

#include "ws2812_spi_v2.h"

#include "log.h"
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
#define BUF_CNT 2  // 缓冲区内灯珠数量
#define BUF_SIZE(n) (BIT_LEN * (n) * 3)

// SPI空闲时MOSI总线为高电平, 这可能造成第一个数据发送错误
// 首尾灯珠不正常时, 在数据前后添加一定数量的低电平或添加一个额外的灯珠,
// 根据实际情况调整
#define HEAD_ZERO 0
#define TAIL_ZERO 0

enum {
  STATE_IDLE = 0,
  STATE_SEND,
  STATE_FILL_END,
  STATE_WAIT_STOP,
};

HAL_StatusTypeDef Strip_Init(LEDStrip_t *strip, uint16_t length,
                             SPI_HandleTypeDef *hspi) {
  if (hspi == NULL) hspi = strip->hspi;
  if (strip->data != NULL || strip->length != 0 || strip->hspi != NULL) {
    LOG_W("LED REINIT");  // 最好手动调用Strip_DeInit再重新初始化
    Strip_DeInit(strip);
  }
  m_alloc(strip->data, length * 3);
  m_alloc(strip->buffer, BUF_SIZE(BUF_CNT) * 2);  // 两个缓冲区交替使用
  if (strip->data == NULL || strip->buffer == NULL) {
    LOG_E("LED MALLOC FAILED");
    Strip_DeInit(strip);
    return HAL_ERROR;
  }
  strip->length = length;
  strip->hspi = hspi;
  strip->index = 0;
  strip->index_end = 0;
  strip->state = STATE_IDLE;
  return HAL_OK;
}

void Strip_DeInit(LEDStrip_t *strip) {
  if (Strip_IsBusy(strip)) Strip_Stop(strip);
  if (strip->data != NULL) m_free(strip->buffer);
  if (strip->buffer != NULL) m_free(strip->buffer);
  strip->length = 0;
  strip->index = 0;
  strip->index_end = 0;
  strip->state = STATE_IDLE;
}

void Strip_Set(LEDStrip_t *strip, uint16_t index, uint32_t RGBcolor) {
  if (index >= strip->length || (!strip->data)) return;  // overrun check
  strip->data[index * 3] = (RGBcolor >> 8) & 0xFF;       // G
  strip->data[index * 3 + 1] = (RGBcolor >> 16) & 0xFF;  // R
  strip->data[index * 3 + 2] = (RGBcolor) & 0xFF;        // B
}

void Strip_Set_Range(LEDStrip_t *strip, uint16_t start, uint16_t end,
                     uint32_t RGBcolor) {
  if (!strip->length || (!strip->data)) return;
  if (end >= strip->length) end = strip->length - 1;
  for (uint16_t index = start; index <= end; index++) {
    Strip_Set(strip, index, RGBcolor);
  }
}

void Strip_Clear(LEDStrip_t *strip) {
  Strip_Set_Range(strip, 0, strip->length - 1, 0x000000);
}

uint8_t Strip_IsBusy(LEDStrip_t *strip) {
  return (HAL_SPI_GetState(strip->hspi) != HAL_SPI_STATE_READY ||
          HAL_DMA_GetState(strip->hspi->hdmatx) != HAL_DMA_STATE_READY ||
          strip->state != STATE_IDLE);
}

static inline void fill_buffer(uint8_t *buf, uint8_t *data) {
  uint32_t GRBdata = data[0] << 16 | data[1] << 8 | data[2];
  memset(buf, 0, BIT_LEN * 3);
  for (uint16_t bfs = 0; bfs < BIT_LEN * 8 * 3; bfs++) {
    if (GRBdata & (((uint32_t)1) << (23 - bfs / BIT_LEN))) {
      buf[bfs / 8] |= ((BIT1 >> (BIT_LEN - 1 - bfs % BIT_LEN)) & 0x01)
                      << (7 - bfs % 8);
    } else {
      buf[bfs / 8] |=
          (((BIT0 >> (BIT_LEN - 1 - bfs % BIT_LEN)) & 0x01) << (7 - bfs % 8));
    }
  }
}

static inline void fill_strip_buffer(LEDStrip_t *strip, const uint8_t buf_num) {
  uint8_t *buf = NULL;
  if (buf_num == 1) {
    buf = strip->buffer;
  } else {
    buf = strip->buffer + BUF_SIZE(BUF_CNT);
  }
  for (uint8_t i = 0; i < BUF_CNT; i++) {
    if (strip->index > strip->index_end) {
      memset(buf + BUF_SIZE(i), 0, BUF_SIZE(1));
      strip->state = STATE_FILL_END;
    } else {
      fill_buffer(buf + BUF_SIZE(i), strip->data + strip->index * 3);
      strip->index++;
    }
  }
}

static LEDStrip_t *sending_strips[6] = {NULL, NULL, NULL, NULL};

__attribute__((always_inline)) void Strip_SPI_Handler(SPI_HandleTypeDef *hspi,
                                                      const uint8_t buf_num) {
  LEDStrip_t **strip_p = NULL;
  for (uint8_t i = 0; i < sizeof(sending_strips) / sizeof(LEDStrip_t *); i++) {
    if (sending_strips[i] != NULL && sending_strips[i]->hspi == hspi) {
      strip_p = &sending_strips[i];
    }
  }
  if (strip_p == NULL) return;
  if ((*strip_p)->state == STATE_SEND) {
    fill_strip_buffer(*strip_p, buf_num);
  } else if ((*strip_p)->state == STATE_FILL_END) {
    (*strip_p)->state = STATE_WAIT_STOP;
  } else if ((*strip_p)->state == STATE_WAIT_STOP) {
    HAL_SPI_DMAStop((*strip_p)->hspi);
    (*strip_p)->state = STATE_IDLE;
    (*strip_p)->index = 0;
    (*strip_p)->index_end = 0;
    *strip_p = NULL;
  }
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
  Strip_SPI_Handler(hspi, 1);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  Strip_SPI_Handler(hspi, 2);
}

HAL_StatusTypeDef Strip_Send(LEDStrip_t *strip) {
  if (!strip->length || !strip->data) return HAL_ERROR;
  if (Strip_IsBusy(strip)) {
    LOG_E("LED BUSY");
    return HAL_BUSY;
  }
  uint8_t err = 1;
  for (uint8_t i = 0; i < sizeof(sending_strips) / sizeof(LEDStrip_t *); i++) {
    if (sending_strips[i] == NULL) {
      sending_strips[i] = strip;
      err = 0;
      break;
    }
  }
  if (err) {
    LOG_E("LED SEND SEQ FULL");
    return HAL_BUSY;
  }
  strip->index = 0;
  strip->index_end = strip->length - 1;
  strip->state = STATE_SEND;
  fill_strip_buffer(strip, 1);
  fill_strip_buffer(strip, 2);
  HAL_SPI_Transmit_DMA(strip->hspi, strip->buffer, BUF_SIZE(BUF_CNT) * 2);
  return HAL_OK;
}

void Strip_Stop(LEDStrip_t *strip) {
  if (!strip->hspi) return;
  HAL_SPI_DMAStop(strip->hspi);
  strip->index = 0;
  strip->index_end = 0;
  strip->state = STATE_IDLE;
}

// R,G,B range 0-255, H range 0-360, S,V range 0-100
uint32_t HSV_To_RGB(float h, uint8_t s, uint8_t v) {
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

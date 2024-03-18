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
#ifdef __cplusplus
extern "C" {
#endif
#include "modules.h"

#if __has_include("spi.h")
#include "spi.h"
typedef struct {
  uint8_t* buffer;  // 缓冲区
  uint16_t length;  // 灯带长度
  SPI_HandleTypeDef* hspi;
} ws2812_strip_t;

/**
 * @brief WS2812灯带初始化
 * @param  strip        灯带句柄
 * @param  length       灯带长度
 * @param  hspi         发送SPI句柄
 * @retval
 */
extern HAL_StatusTypeDef ws2812_init(ws2812_strip_t* strip, uint16_t length,
                                     SPI_HandleTypeDef* hspi);

/**
 * @brief WS2812灯带注销
 * @param  strip       灯带句柄
 */
extern void ws2812_deinit(ws2812_strip_t* strip);

/**
 * @brief 设置灯带某个LED的颜色
 * @param  strip       灯带句柄
 * @param  index       LED序号
 * @param  RGBcolor    RGB颜色
 */
extern void ws2812_set(ws2812_strip_t* strip, uint16_t index,
                       uint32_t RGBcolor);

/**
 * @brief 清空灯带
 * @param  strip       灯带句柄
 */
extern void ws2812_clear(ws2812_strip_t* strip);

/**
 * @brief 设置灯带某个区间的颜色
 * @param  strip       灯带句柄
 * @param  start       起始LED序号
 * @param  end         结束LED序号
 * @param  RGBcolor    RGB颜色
 */
extern void ws2812_set_range(ws2812_strip_t* strip, uint16_t start,
                             uint16_t end, uint32_t RGBcolor);

/**
 * @brief 检查灯带是否发送忙
 * @param  strip      灯带句柄
 * @retval 0          空闲
 */
extern uint8_t ws2812_is_busy(ws2812_strip_t* strip);

/**
 * @brief 发送灯带数据
 * @param  strip      灯带句柄
 * @retval HAL_StatusTypeDef
 */
extern HAL_StatusTypeDef ws2812_send(ws2812_strip_t* strip);

/**
 * @brief 发送灯带数据
 * @param  strip      灯带句柄
 * @param  num        发送的LED数量
 * @retval HAL_StatusTypeDef
 */
extern HAL_StatusTypeDef ws2812_send_part(ws2812_strip_t* strip, uint16_t num);

/**
 * @brief 发送灯带数据/阻塞
 * @param  strip      灯带句柄
 * @retval HAL_StatusTypeDef
 */
extern void ws2812_send_blocking(ws2812_strip_t* strip);

/**
 * @brief 转换HSV颜色到RGB颜色
 * @param  h          色相 (0-360)
 * @param  s          饱和度 (0-255)
 * @param  v          亮度 (0-255)
 * @retval uint32_t    RGB颜色
 */
extern uint32_t hsv2rgb(float h, uint8_t s, uint8_t v);
#ifdef __cplusplus
}
#endif
#endif  // __has_include("spi.h")
#endif  // __WS2812_SPI_H__

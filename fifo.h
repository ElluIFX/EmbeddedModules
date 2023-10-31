/**,.,
 * @file fifo.h
 * @brief 使用宏函数的方式简化fifo的初始化和使用
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-07-20
 *
 * THINK DIFFERENTLY
 */

#ifndef __FIFO_H__
#define __FIFO_H__
#include "stdint.h"
/**
 * @brief 定义一个FIFO缓冲区，包括缓冲区名、缓冲区类型和缓冲区长度
 *
 * @param FIFO_NAME 缓冲区名
 * @param FIFO_TYPE 缓冲区类型
 * @param FIFO_LENGTH 缓冲区长度
 */
#define FIFO_INIT(FIFO_NAME, FIFO_TYPE, FIFO_LENGTH) \
  FIFO_TYPE _fifo_##FIFO_NAME##_buffer[FIFO_LENGTH]; \
  uint16_t _fifo_##FIFO_NAME##_write = 0;            \
  uint16_t _fifo_##FIFO_NAME##_read = 0;             \
  const uint16_t _fifo_##FIFO_NAME##_length = FIFO_LENGTH;

/**
 * @brief 声明一个FIFO缓冲区，包括缓冲区名、缓冲区类型和缓冲区长度
 *
 * @param FIFO_NAME 缓冲区名
 * @param FIFO_TYPE 缓冲区类型
 * @param FIFO_LENGTH 缓冲区长度
 */
#define FIFO_EXTERN(FIFO_NAME, FIFO_TYPE, FIFO_LENGTH)      \
  extern FIFO_TYPE _fifo_##FIFO_NAME##_buffer[FIFO_LENGTH]; \
  extern uint16_t _fifo_##FIFO_NAME##_write;                \
  extern uint16_t _fifo_##FIFO_NAME##_read;                 \
  extern const uint16_t _fifo_##FIFO_NAME##_length;

/**
 * @brief 判断FIFO缓冲区是否为空
 *
 * @param FIFO_NAME 缓冲区名
 * @return true 缓冲区为空
 * @return false 缓冲区不为空
 */
#define FIFO_IS_EMPTY(FIFO_NAME) \
  (_fifo_##FIFO_NAME##_write == _fifo_##FIFO_NAME##_read)

/**
 * @brief 判断FIFO缓冲区是否已满
 *
 * @param FIFO_NAME 缓冲区名
 * @return true 缓冲区已满
 * @return false 缓冲区未满
 */
#define FIFO_IS_FULL(FIFO_NAME)                                    \
  ((_fifo_##FIFO_NAME##_write + 1) % _fifo_##FIFO_NAME##_length == \
   _fifo_##FIFO_NAME##_read)

/**
 * @brief 获取FIFO缓冲区长度
 *
 * @param FIFO_NAME 缓冲区名
 * @return const uint16_t 缓冲区长度
 */
#define FIFO_LENGTH(FIFO_NAME) _fifo_##FIFO_NAME##_length

/**
 * @brief 获取FIFO缓冲区剩余空间
 *
 * @param FIFO_NAME 缓冲区名
 * @return uint16_t 缓冲区剩余空间
 */
#define FIFO_FREE_SPACE(FIFO_NAME) \
  (_fifo_##FIFO_NAME##_length - FIFO_DATA_LENGTH(FIFO_NAME) - 1)

/**
 * @brief 获取FIFO缓冲区数据长度
 *
 * @param FIFO_NAME 缓冲区名
 * @return uint16_t 缓冲区数据长度
 */
#define FIFO_DATA_LENGTH(FIFO_NAME)                               \
  (_fifo_##FIFO_NAME##_write >= _fifo_##FIFO_NAME##_read          \
       ? _fifo_##FIFO_NAME##_write - _fifo_##FIFO_NAME##_read     \
       : _fifo_##FIFO_NAME##_length + _fifo_##FIFO_NAME##_write - \
             _fifo_##FIFO_NAME##_read)

/**
 * @brief 清空FIFO缓冲区
 *
 * @param FIFO_NAME 缓冲区名
 */
#define FIFO_CLEAR(FIFO_NAME) \
  _fifo_##FIFO_NAME##_write = _fifo_##FIFO_NAME##_read = 0

/**
 * @brief 向FIFO缓冲区中添加数据
 *
 * @param FIFO_NAME 缓冲区名
 * @param DATA 数据
 */
#define FIFO_PUSH(FIFO_NAME, DATA)                              \
  _fifo_##FIFO_NAME##_buffer[_fifo_##FIFO_NAME##_write] = DATA; \
  _fifo_##FIFO_NAME##_write =                                   \
      (_fifo_##FIFO_NAME##_write + 1) % _fifo_##FIFO_NAME##_length

/**
 * @brief 从FIFO缓冲区中弹出数据
 *
 * @param FIFO_NAME 缓冲区名
 * @return typeof(_fifo_##FIFO_NAME##_buffer[0]) 弹出的数据
 */
#define FIFO_POP(FIFO_NAME)                             \
  _fifo_##FIFO_NAME##_buffer[_fifo_##FIFO_NAME##_read]; \
  _fifo_##FIFO_NAME##_read =                            \
      (_fifo_##FIFO_NAME##_read + 1) % _fifo_##FIFO_NAME##_length

/**
 * @brief 获取FIFO缓冲区中的第一个数据
 *
 * @param FIFO_NAME 缓冲区名
 * @return typeof(_fifo_##FIFO_NAME##_buffer[0]) 第一个数据
 */
#define FIFO_PEEK(FIFO_NAME) \
  _fifo_##FIFO_NAME##_buffer[_fifo_##FIFO_NAME##_read]

/**
 * @brief 获取FIFO缓冲区中指定偏移量的数据
 *
 * @param FIFO_NAME 缓冲区名
 * @param OFFSET 偏移量
 * @return typeof(_fifo_##FIFO_NAME##_buffer[0]) 指定偏移量的数据
 */
#define FIFO_PEEK_OFFSET(FIFO_NAME, OFFSET)                        \
  _fifo_##FIFO_NAME##_buffer[(_fifo_##FIFO_NAME##_read + OFFSET) % \
                             _fifo_##FIFO_NAME##_length]

/**
 * @brief 获取FIFO缓冲区中最后一个数据
 *
 * @param FIFO_NAME 缓冲区名
 * @return typeof(_fifo_##FIFO_NAME##_buffer[0]) 最后一个数据
 */
#define FIFO_PEEK_LAST(FIFO_NAME)                               \
  _fifo_##FIFO_NAME##_buffer[(_fifo_##FIFO_NAME##_write +       \
                              _fifo_##FIFO_NAME##_length - 1) % \
                             _fifo_##FIFO_NAME##_length]

/**
 * @brief 向FIFO缓冲区中添加数据数组
 *
 * @param FIFO_NAME 缓冲区名
 * @param BUFFER 数据数组
 * @param LENGTH 数据数组长度
 */
#define FIFO_PUSH_BUFFER(FIFO_NAME, BUFFER, LENGTH) \
  for (uint16_t i = 0; i < LENGTH; i++) {           \
    FIFO_PUSH(FIFO_NAME, BUFFER[i]);                \
  }

/**
 * @brief 从FIFO缓冲区中弹出数据数组
 *
 * @param FIFO_NAME 缓冲区名
 * @param BUFFER 数据数组
 * @param LENGTH 数据数组长度
 */
#define FIFO_POP_BUFFER(FIFO_NAME, BUFFER, LENGTH) \
  for (uint16_t i = 0; i < LENGTH; i++) {          \
    BUFFER[i] = FIFO_POP(FIFO_NAME);               \
  }

/**
 * @brief 获取FIFO缓冲区中指定长度的数据数组
 *
 * @param FIFO_NAME 缓冲区名
 * @param BUFFER 数据数组
 * @param LENGTH 数据数组长度
 */
#define FIFO_PEEK_BUFFER(FIFO_NAME, BUFFER, LENGTH) \
  for (uint16_t i = 0; i < LENGTH; i++) {           \
    BUFFER[i] = FIFO_PEEK_OFFSET(FIFO_NAME, i);     \
  }

#endif  // __FIFO_H__

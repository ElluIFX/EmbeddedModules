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

#define FIFO_INIT(FIFO_NAME, FIFO_TYPE, FIFO_LENGTH) \
  FIFO_TYPE _fifo_##FIFO_NAME##_buffer[FIFO_LENGTH]; \
  uint16_t _fifo_##FIFO_NAME##_write = 0;            \
  uint16_t _fifo_##FIFO_NAME##_read = 0;             \
  const uint16_t _fifo_##FIFO_NAME##_length = FIFO_LENGTH;

#define FIFO_EXTERN(FIFO_NAME, FIFO_TYPE, FIFO_LENGTH)      \
  extern FIFO_TYPE _fifo_##FIFO_NAME##_buffer[FIFO_LENGTH]; \
  extern uint16_t _fifo_##FIFO_NAME##_write;                \
  extern uint16_t _fifo_##FIFO_NAME##_read;                 \
  extern const uint16_t _fifo_##FIFO_NAME##_length;

#define FIFO_IS_EMPTY(FIFO_NAME) \
  (_fifo_##FIFO_NAME##_write == _fifo_##FIFO_NAME##_read)

#define FIFO_IS_FULL(FIFO_NAME)                                    \
  ((_fifo_##FIFO_NAME##_write + 1) % _fifo_##FIFO_NAME##_length == \
   _fifo_##FIFO_NAME##_read)

#define FIFO_LENGTH(FIFO_NAME) _fifo_##FIFO_NAME##_length

#define FIFO_FREE_SPACE(FIFO_NAME) \
  (_fifo_##FIFO_NAME##_length - FIFO_DATA_LENGTH(FIFO_NAME) - 1)

#define FIFO_DATA_LENGTH(FIFO_NAME)                               \
  (_fifo_##FIFO_NAME##_write >= _fifo_##FIFO_NAME##_read          \
       ? _fifo_##FIFO_NAME##_write - _fifo_##FIFO_NAME##_read     \
       : _fifo_##FIFO_NAME##_length + _fifo_##FIFO_NAME##_write - \
             _fifo_##FIFO_NAME##_read)

#define FIFO_CLEAR(FIFO_NAME) \
  _fifo_##FIFO_NAME##_write = _fifo_##FIFO_NAME##_read = 0

#define FIFO_PUSH(FIFO_NAME, DATA)                              \
  _fifo_##FIFO_NAME##_buffer[_fifo_##FIFO_NAME##_write] = DATA; \
  _fifo_##FIFO_NAME##_write =                                   \
      (_fifo_##FIFO_NAME##_write + 1) % _fifo_##FIFO_NAME##_length

#define FIFO_POP(FIFO_NAME)                             \
  _fifo_##FIFO_NAME##_buffer[_fifo_##FIFO_NAME##_read]; \
  _fifo_##FIFO_NAME##_read =                            \
      (_fifo_##FIFO_NAME##_read + 1) % _fifo_##FIFO_NAME##_length

#define FIFO_PEEK(FIFO_NAME) \
  _fifo_##FIFO_NAME##_buffer[_fifo_##FIFO_NAME##_read]

#define FIFO_PEEK_OFFSET(FIFO_NAME, OFFSET)                        \
  _fifo_##FIFO_NAME##_buffer[(_fifo_##FIFO_NAME##_read + OFFSET) % \
                             _fifo_##FIFO_NAME##_length]

#define FIFO_PEEK_LAST(FIFO_NAME)                               \
  _fifo_##FIFO_NAME##_buffer[(_fifo_##FIFO_NAME##_write +       \
                              _fifo_##FIFO_NAME##_length - 1) % \
                             _fifo_##FIFO_NAME##_length]

#define FIFO_PUSH_BUFFER(FIFO_NAME, BUFFER, LENGTH) \
  for (uint16_t i = 0; i < LENGTH; i++) {           \
    FIFO_PUSH(FIFO_NAME, BUFFER[i]);                \
  }

#define FIFO_POP_BUFFER(FIFO_NAME, BUFFER, LENGTH) \
  for (uint16_t i = 0; i < LENGTH; i++) {          \
    BUFFER[i] = FIFO_POP(FIFO_NAME);               \
  }

#define FIFO_PEEK_BUFFER(FIFO_NAME, BUFFER, LENGTH) \
  for (uint16_t i = 0; i < LENGTH; i++) {           \
    BUFFER[i] = FIFO_PEEK_OFFSET(FIFO_NAME, i);     \
  }

#endif  // __FIFO_H__

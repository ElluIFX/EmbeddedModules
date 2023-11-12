/**
 * @file fifo.c
 * @brief 简单的FIFO缓冲区实现
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-11-07
 *
 * THINK DIFFERENTLY
 */
#include "fifo.h"

#include <string.h>

#define _INLINE __attribute__((always_inline)) inline

void Fifo_Init_Static(fifo_t *fifo, uint8_t *static_buffer_p, uint16_t size) {
  fifo->buf = static_buffer_p;
  fifo->size = size;
  fifo->wr = 0;
  fifo->rd = 0;
}

uint8_t Fifo_Init_Dynamic(fifo_t *fifo, uint16_t size) {
  m_alloc(fifo->buf, size + 1);
  if (fifo->buf == NULL) {
    return 1;
  }
  fifo->size = size + 1;
  fifo->wr = 0;
  fifo->rd = 0;
  return 0;
}

void Fifo_Destory_Dynamic(fifo_t *fifo) {
  m_free(fifo->buf);
  fifo->buf = NULL;
  fifo->size = 0;
  fifo->wr = 0;
  fifo->rd = 0;
}

_INLINE uint16_t Fifo_GetSize(fifo_t *fifo) { return fifo->size - 1; }

_INLINE uint16_t Fifo_GetUsed(fifo_t *fifo) {
  if (fifo->wr >= fifo->rd) {
    return fifo->wr - fifo->rd;
  } else {
    return fifo->size - fifo->rd + fifo->wr;
  }
}

_INLINE uint16_t Fifo_GetFree(fifo_t *fifo) {
  return fifo->size - Fifo_GetUsed(fifo) - 1;
}

void Fifo_ClearFill(fifo_t *fifo, const uint8_t fill_data) {
  memset(fifo->buf, fill_data, fifo->size);
  fifo->wr = 0;
  fifo->rd = 0;
}

void Fifo_Clear(fifo_t *fifo) {
  fifo->wr = 0;
  fifo->rd = 0;
}

uint16_t Fifo_Put(fifo_t *fifo, uint8_t *data, uint16_t len) {
  uint16_t i = 0;
  uint16_t free_size = Fifo_GetFree(fifo);
  uint16_t wr_t = fifo->wr;
  if (len > free_size) {
    len = free_size;
  }
  fifo->wr = (fifo->wr + len) % fifo->size;
  for (i = 0; i < len; i++) {
    fifo->buf[wr_t] = data[i];
    wr_t = (wr_t + 1) % fifo->size;
  }
  return len;
}

uint16_t Fifo_Get(fifo_t *fifo, uint8_t *data, uint16_t len) {
  uint16_t i = 0;
  uint16_t used_size = Fifo_GetUsed(fifo);
  uint16_t rd_t = fifo->rd;
  if (len > used_size) {
    len = used_size;
  }
  fifo->rd = (fifo->rd + len) % fifo->size;
  if (data == NULL) return len;
  for (i = 0; i < len; i++) {
    data[i] = fifo->buf[rd_t];
    rd_t = (rd_t + 1) % fifo->size;
  }
  return len;
}

_INLINE uint8_t Fifo_PutByte(fifo_t *fifo, uint8_t data) {
  if (Fifo_GetFree(fifo) == 0) {
    return 1;
  }
  fifo->buf[fifo->wr] = data;
  fifo->wr = (fifo->wr + 1) % fifo->size;
  return 0;
}

_INLINE uint8_t Fifo_GetByte(fifo_t *fifo) {
  if (Fifo_GetUsed(fifo) == 0) {
    return 0;
  }
  uint8_t data = fifo->buf[fifo->rd];
  fifo->rd = (fifo->rd + 1) % fifo->size;
  return data;
}

_INLINE uint8_t Fifo_PeekByte(fifo_t *fifo, uint16_t offset) {
  if (offset >= Fifo_GetUsed(fifo)) {
    return 0;
  }
  return fifo->buf[(fifo->rd + offset) % fifo->size];
}

uint16_t Fifo_Peek(fifo_t *fifo, uint16_t offset, uint8_t *data, uint16_t len) {
  uint16_t i = 0;
  uint16_t used_size = Fifo_GetUsed(fifo);
  if (offset >= used_size) {
    return 0;
  }
  if (len + offset > used_size) {
    len = used_size - offset;
  }
  uint16_t rd_t = (fifo->rd + offset) % fifo->size;
  for (i = 0; i < len; i++) {
    data[i] = fifo->buf[rd_t];
    rd_t = (rd_t + 1) % fifo->size;
  }
  return len;
}

uint8_t *Fifo_GetWrPtr(fifo_t *fifo, int16_t offset) {
  int16_t temp = (int16_t)fifo->wr + offset;
  while (temp < 0) {
    temp += fifo->size;
  }
  return &fifo->buf[temp % fifo->size];
}

uint8_t *Fifo_GetRdPtr(fifo_t *fifo, int16_t offset) {
  int16_t temp = (int16_t)fifo->rd + offset;
  while (temp < 0) {
    temp += fifo->size;
  }
  return &fifo->buf[temp % fifo->size];
}

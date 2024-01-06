/**
 * @file lfifo.c
 * @brief 原子化的环形FIFO缓冲区
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-11-07
 *
 * THINK DIFFERENTLY
 */
#include "lfifo.h"

#if FIFO_DISABLE_ATOMIC
#define FIFO_INIT(var, val) (var) = (val)
#define FIFO_LOAD(var, type) (var)
#define FIFO_STORE(var, val, type) (var) = (val)
#define _FIFO_MEMORDER_ACQ 0
#define _FIFO_MEMORDER_REL 0
#else
#define FIFO_INIT(var, val) atomic_init(&(var), (val))
#define FIFO_LOAD(var, type) atomic_load_explicit(&(var), (type))
#define FIFO_STORE(var, val, type) atomic_store_explicit(&(var), (val), (type))
#define _FIFO_MEMORDER_ACQ __ATOMIC_ACQUIRE
#define _FIFO_MEMORDER_REL __ATOMIC_RELEASE
#define _FIFO_MEMORDER_RELEX __ATOMIC_RELAXED
#endif  // FIFO_DISABLE_ATOMIC

#define _INLINE __attribute__((always_inline)) inline

int LFifo_Init(lfifo_t *fifo, fifo_size_t size) {
  fifo->buf = m_alloc(size + 1);
  if (fifo->buf == NULL) {
    return -1;
  }
  fifo->size = size + 1;
  FIFO_INIT(fifo->wr, 0);
  FIFO_INIT(fifo->rd, 0);
  return 0;
}

void LFifo_Destory(lfifo_t *fifo) {
  m_free(fifo->buf);
  fifo->buf = NULL;
  fifo->size = 0;
  FIFO_INIT(fifo->wr, 0);
  FIFO_INIT(fifo->rd, 0);
}

void LFifo_AssignBuf(lfifo_t *fifo, uint8_t *buffer, fifo_size_t size) {
  fifo->buf = buffer;
  fifo->size = size;
  FIFO_INIT(fifo->wr, 0);
  FIFO_INIT(fifo->rd, 0);
}

_INLINE fifo_size_t LFifo_GetSize(lfifo_t *fifo) {
  if (fifo->size == 0) return 0;
  return fifo->size - 1;
}

_INLINE fifo_size_t LFifo_GetUsed(lfifo_t *fifo) {
  if (fifo->size == 0) return 0;
  if (fifo->wr >= fifo->rd) {
    return fifo->wr - fifo->rd;
  } else {
    return fifo->size - fifo->rd + fifo->wr;
  }
}

_INLINE fifo_size_t LFifo_GetFree(lfifo_t *fifo) {
  if (fifo->size == 0) return 0;
  return fifo->size - LFifo_GetUsed(fifo) - 1;
}

_INLINE bool LFifo_IsEmpty(lfifo_t *fifo) { return (fifo->wr == fifo->rd); }

void LFifo_ClearFill(lfifo_t *fifo, const uint8_t fill_data) {
  memset(fifo->buf, fill_data, fifo->size);
  fifo->wr = 0;
  fifo->rd = 0;
}

void LFifo_Clear(lfifo_t *fifo) {
  fifo->wr = 0;
  fifo->rd = 0;
}

fifo_size_t LFifo_Write(lfifo_t *fifo, uint8_t *data, fifo_size_t len) {
  fifo_size_t free_size = LFifo_GetFree(fifo);
  if (len > free_size) len = free_size;
  fifo_size_t wr_t = FIFO_LOAD(fifo->wr, _FIFO_MEMORDER_ACQ);
  fifo_size_t tocpy = len;
  if (len > fifo->size - wr_t) tocpy = fifo->size - wr_t;
  if (data != NULL) FIFO_MEMCPY(&fifo->buf[wr_t], data, tocpy);
  wr_t += tocpy;
  data += tocpy;
  tocpy = len - tocpy;
  if (tocpy) {
    if (data != NULL) FIFO_MEMCPY(fifo->buf, data, tocpy);
    wr_t = tocpy;
  }
  wr_t %= fifo->size;
  FIFO_STORE(fifo->wr, wr_t, _FIFO_MEMORDER_REL);
  return len;
}

fifo_size_t LFifo_Read(lfifo_t *fifo, uint8_t *data, fifo_size_t len) {
  fifo_size_t used_size = LFifo_GetUsed(fifo);
  if (len > used_size) len = used_size;
  if (data == NULL) {  // discard data
    fifo->rd = (fifo->rd + len) % fifo->size;
    return len;
  }
  fifo_size_t rd_t = FIFO_LOAD(fifo->rd, _FIFO_MEMORDER_ACQ);
  fifo_size_t tocpy = len;
  if (len > fifo->size - rd_t) tocpy = fifo->size - rd_t;
  if (data != NULL) FIFO_MEMCPY(data, &fifo->buf[rd_t], tocpy);
  rd_t += tocpy;
  data += tocpy;
  tocpy = len - tocpy;
  if (tocpy) {
    if (data != NULL) FIFO_MEMCPY(data, fifo->buf, tocpy);
    rd_t = tocpy;
  }
  rd_t %= fifo->size;
  FIFO_STORE(fifo->rd, rd_t, _FIFO_MEMORDER_REL);
  return len;
}

fifo_size_t LFifo_Peek(lfifo_t *fifo, fifo_size_t offset, uint8_t *data,
                       fifo_size_t len) {
  fifo_size_t used_size = LFifo_GetUsed(fifo);
  if (offset >= used_size) return 0;
  if (len + offset > used_size) len = used_size - offset;
  fifo_size_t rd_t = (fifo->rd + offset) % fifo->size;
  fifo_size_t tocpy = len;
  if (len > fifo->size - rd_t) tocpy = fifo->size - rd_t;
  FIFO_MEMCPY(data, &fifo->buf[rd_t], tocpy);
  data += tocpy;
  tocpy = len - tocpy;
  if (tocpy) FIFO_MEMCPY(data, fifo->buf, tocpy);
  return len;
}

_INLINE int LFifo_WriteByte(lfifo_t *fifo, uint8_t data) {
  if (fifo->wr == (fifo->rd + fifo->size - 1) % fifo->size) return -1;
  fifo_size_t wr_t = FIFO_LOAD(fifo->wr, _FIFO_MEMORDER_ACQ);
  fifo->buf[wr_t] = data;
  wr_t = (wr_t + 1) % fifo->size;
  FIFO_STORE(fifo->wr, wr_t, _FIFO_MEMORDER_REL);
  return 0;
}

_INLINE int LFifo_ReadByte(lfifo_t *fifo) {
  if (fifo->wr == fifo->rd) return -1;
  fifo_size_t rd_t = FIFO_LOAD(fifo->rd, _FIFO_MEMORDER_ACQ);
  uint8_t data = fifo->buf[rd_t];
  rd_t = (rd_t + 1) % fifo->size;
  FIFO_STORE(fifo->rd, rd_t, _FIFO_MEMORDER_REL);
  return data;
}

_INLINE int LFifo_PeekByte(lfifo_t *fifo, fifo_size_t offset) {
  if (offset >= LFifo_GetUsed(fifo)) {
    return -1;
  }
  return fifo->buf[(fifo->rd + offset) % fifo->size];
}

uint8_t *LFifo_GetWritePtr(lfifo_t *fifo, fifo_offset_t offset) {
  fifo_offset_t temp = (fifo_offset_t)fifo->wr + offset;
  while (temp < 0) {
    temp += fifo->size;
  }
  return &fifo->buf[temp % fifo->size];
}

uint8_t *LFifo_GetReadPtr(lfifo_t *fifo, fifo_offset_t offset) {
  fifo_offset_t temp = (fifo_offset_t)fifo->rd + offset;
  while (temp < 0) {
    temp += fifo->size;
  }
  return &fifo->buf[temp % fifo->size];
}

fifo_offset_t LFifo_Find(lfifo_t *fifo, uint8_t *data, fifo_size_t len,
                         fifo_size_t r_offset) {
  fifo_size_t used, rd;
  uint8_t found;
  if (len == 0 || data == NULL || LFifo_GetUsed(fifo) == 0) {
    return -1;
  }
  used = LFifo_GetUsed(fifo);
  /* Verify initial conditions */
  if (used < (len + r_offset)) {
    return -1;
  }
  for (fifo_size_t skip_x = r_offset; skip_x <= used - len; ++skip_x) {
    found = 1;
    /* Prepare the starting point for reading */
    rd = (fifo->rd + skip_x) % fifo->size;
    /* Search in the buffer */
    for (fifo_size_t i = 0; i < len; ++i) {
      if (fifo->buf[rd] != data[i]) {
        found = 0;
        break;
      }
      if (++rd >= fifo->size) rd = 0;
    }
    if (found) return skip_x;
  }
  return -1;
}

uint8_t *LFifo_AcquireLinearWrite(lfifo_t *fifo, fifo_size_t *len) {
  if (LFifo_GetFree(fifo) == 0) {
    *len = 0;
    return NULL;
  }
  fifo_size_t wr_t = FIFO_LOAD(fifo->wr, _FIFO_MEMORDER_RELEX);
  fifo_size_t rd_t = FIFO_LOAD(fifo->rd, _FIFO_MEMORDER_RELEX);
  if (wr_t >= rd_t) {
    *len = fifo->size - wr_t;
  } else {
    *len = rd_t - wr_t - 1;
  }
  return &fifo->buf[wr_t];
}

void LFifo_ReleaseLinearWrite(lfifo_t *fifo, fifo_size_t len) {
  if (len == 0) return;
  fifo_size_t wr_t = FIFO_LOAD(fifo->wr, _FIFO_MEMORDER_ACQ);
  wr_t += len;
  wr_t %= fifo->size;
  FIFO_STORE(fifo->wr, wr_t, _FIFO_MEMORDER_REL);
}

uint8_t *LFifo_AcquireLinearRead(lfifo_t *fifo, fifo_size_t *len) {
  fifo_size_t used = LFifo_GetUsed(fifo);
  if (used == 0) {
    *len = 0;
    return NULL;
  }
  fifo_size_t wr_t = FIFO_LOAD(fifo->wr, _FIFO_MEMORDER_RELEX);
  fifo_size_t rd_t = FIFO_LOAD(fifo->rd, _FIFO_MEMORDER_RELEX);
  if (wr_t >= rd_t) {
    *len = wr_t - rd_t;
  } else {
    *len = fifo->size - rd_t;
  }
  return &fifo->buf[rd_t];
}

void LFifo_ReleaseLinearRead(lfifo_t *fifo, fifo_size_t len) {
  if (len == 0) return;
  fifo_size_t rd_t = FIFO_LOAD(fifo->rd, _FIFO_MEMORDER_ACQ);
  rd_t += len;
  rd_t %= fifo->size;
  FIFO_STORE(fifo->rd, rd_t, _FIFO_MEMORDER_REL);
}

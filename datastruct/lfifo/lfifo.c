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

#include <string.h>  // memcpy

#define _INLINE __attribute__((always_inline)) inline

int LFifo_Init(lfifo_t* fifo, mod_size_t size) {
    fifo->buf = m_alloc(size + 1);
    if (fifo->buf == NULL) {
        return -1;
    }
    fifo->size = size + 1;
    MOD_ATOMIC_INIT(fifo->wr, 0);
    MOD_ATOMIC_INIT(fifo->rd, 0);
    return 0;
}

void LFifo_Destory(lfifo_t* fifo) {
    m_free(fifo->buf);
    fifo->buf = NULL;
    fifo->size = 0;
    MOD_ATOMIC_INIT(fifo->wr, 0);
    MOD_ATOMIC_INIT(fifo->rd, 0);
}

void LFifo_AssignBuf(lfifo_t* fifo, uint8_t* buffer, mod_size_t size) {
    fifo->buf = buffer;
    fifo->size = size;
    MOD_ATOMIC_INIT(fifo->wr, 0);
    MOD_ATOMIC_INIT(fifo->rd, 0);
}

_INLINE mod_size_t LFifo_GetSize(lfifo_t* fifo) {
    if (fifo->size == 0)
        return 0;
    return fifo->size - 1;
}

_INLINE mod_size_t LFifo_GetUsed(lfifo_t* fifo) {
    if (fifo->size == 0)
        return 0;
    if (fifo->wr >= fifo->rd) {
        return fifo->wr - fifo->rd;
    } else {
        return fifo->size - fifo->rd + fifo->wr;
    }
}

_INLINE mod_size_t LFifo_GetFree(lfifo_t* fifo) {
    if (fifo->size == 0)
        return 0;
    return fifo->size - LFifo_GetUsed(fifo) - 1;
}

_INLINE bool LFifo_IsEmpty(lfifo_t* fifo) {
    return (fifo->wr == fifo->rd);
}

_INLINE bool LFifo_IsFull(lfifo_t* fifo) {
    return ((fifo->wr + 1) % fifo->size == fifo->rd);
}

void LFifo_ClearFill(lfifo_t* fifo, const uint8_t fill_data) {
    memset(fifo->buf, fill_data, fifo->size);
    fifo->wr = 0;
    fifo->rd = 0;
}

void LFifo_Clear(lfifo_t* fifo) {
    fifo->wr = 0;
    fifo->rd = 0;
}

mod_size_t LFifo_Write(lfifo_t* fifo, uint8_t* data, mod_size_t len) {
    mod_size_t free_size = LFifo_GetFree(fifo);
    if (!free_size)
        return 0;
    if (len > free_size)
        len = free_size;
    mod_size_t wr_t = MOD_ATOMIC_LOAD(fifo->wr, MOD_ATOMIC_ORDER_ACQUIRE);
    mod_size_t tocpy = len;
    if (len > fifo->size - wr_t)
        tocpy = fifo->size - wr_t;
    if (data != NULL)
        memcpy(&fifo->buf[wr_t], data, tocpy);
    wr_t += tocpy;
    data += tocpy;
    tocpy = len - tocpy;
    if (tocpy) {
        if (data != NULL)
            memcpy(fifo->buf, data, tocpy);
        wr_t = tocpy;
    }
    wr_t %= fifo->size;
    MOD_ATOMIC_STORE(fifo->wr, wr_t, MOD_ATOMIC_ORDER_RELEASE);
    return len;
}

mod_size_t LFifo_Read(lfifo_t* fifo, uint8_t* data, mod_size_t len) {
    mod_size_t used_size = LFifo_GetUsed(fifo);
    if (!used_size)
        return 0;
    if (len > used_size)
        len = used_size;
    if (data == NULL) {  // discard data
        fifo->rd = (fifo->rd + len) % fifo->size;
        return len;
    }
    mod_size_t rd_t = MOD_ATOMIC_LOAD(fifo->rd, MOD_ATOMIC_ORDER_ACQUIRE);
    mod_size_t tocpy = len;
    if (len > fifo->size - rd_t)
        tocpy = fifo->size - rd_t;
    if (data != NULL)
        memcpy(data, &fifo->buf[rd_t], tocpy);
    rd_t += tocpy;
    data += tocpy;
    tocpy = len - tocpy;
    if (tocpy) {
        if (data != NULL)
            memcpy(data, fifo->buf, tocpy);
        rd_t = tocpy;
    }
    rd_t %= fifo->size;
    MOD_ATOMIC_STORE(fifo->rd, rd_t, MOD_ATOMIC_ORDER_RELEASE);
    return len;
}

mod_size_t LFifo_Peek(lfifo_t* fifo, mod_size_t offset, uint8_t* data,
                      mod_size_t len) {
    mod_size_t used_size = LFifo_GetUsed(fifo);
    if (offset >= used_size)
        return 0;
    if (len + offset > used_size)
        len = used_size - offset;
    mod_size_t rd_t = (fifo->rd + offset) % fifo->size;
    mod_size_t tocpy = len;
    if (len > fifo->size - rd_t)
        tocpy = fifo->size - rd_t;
    memcpy(data, &fifo->buf[rd_t], tocpy);
    data += tocpy;
    tocpy = len - tocpy;
    if (tocpy)
        memcpy(data, fifo->buf, tocpy);
    return len;
}

_INLINE int LFifo_WriteByte(lfifo_t* fifo, uint8_t data) {
    if (fifo->wr == (fifo->rd + fifo->size - 1) % fifo->size)
        return -1;
    mod_size_t wr_t = MOD_ATOMIC_LOAD(fifo->wr, MOD_ATOMIC_ORDER_ACQUIRE);
    fifo->buf[wr_t] = data;
    wr_t = (wr_t + 1) % fifo->size;
    MOD_ATOMIC_STORE(fifo->wr, wr_t, MOD_ATOMIC_ORDER_RELEASE);
    return 0;
}

_INLINE int LFifo_ReadByte(lfifo_t* fifo) {
    if (fifo->wr == fifo->rd)
        return -1;
    mod_size_t rd_t = MOD_ATOMIC_LOAD(fifo->rd, MOD_ATOMIC_ORDER_ACQUIRE);
    uint8_t data = fifo->buf[rd_t];
    rd_t = (rd_t + 1) % fifo->size;
    MOD_ATOMIC_STORE(fifo->rd, rd_t, MOD_ATOMIC_ORDER_RELEASE);
    return data;
}

_INLINE int LFifo_PeekByte(lfifo_t* fifo, mod_size_t offset) {
    if (offset >= LFifo_GetUsed(fifo)) {
        return -1;
    }
    return fifo->buf[(fifo->rd + offset) % fifo->size];
}

uint8_t* LFifo_GetWritePtr(lfifo_t* fifo, mod_offset_t offset) {
    mod_offset_t temp = (mod_offset_t)fifo->wr + offset;
    while (temp < 0) {
        temp += fifo->size;
    }
    return &fifo->buf[temp % fifo->size];
}

uint8_t* LFifo_GetReadPtr(lfifo_t* fifo, mod_offset_t offset) {
    mod_offset_t temp = (mod_offset_t)fifo->rd + offset;
    while (temp < 0) {
        temp += fifo->size;
    }
    return &fifo->buf[temp % fifo->size];
}

mod_offset_t LFifo_Find(lfifo_t* fifo, uint8_t* data, mod_size_t len,
                        mod_size_t r_offset) {
    mod_size_t used, rd;
    uint8_t found;
    if (len == 0 || data == NULL || LFifo_GetUsed(fifo) == 0) {
        return -1;
    }
    used = LFifo_GetUsed(fifo);
    /* Verify initial conditions */
    if (used < (len + r_offset)) {
        return -1;
    }
    for (mod_size_t skip_x = r_offset; skip_x <= used - len; ++skip_x) {
        found = 1;
        /* Prepare the starting point for reading */
        rd = (fifo->rd + skip_x) % fifo->size;
        /* Search in the buffer */
        for (mod_size_t i = 0; i < len; ++i) {
            if (fifo->buf[rd] != data[i]) {
                found = 0;
                break;
            }
            if (++rd >= fifo->size)
                rd = 0;
        }
        if (found)
            return skip_x;
    }
    return -1;
}

uint8_t* LFifo_AcquireLinearWrite(lfifo_t* fifo, mod_size_t* len) {
    if (LFifo_GetFree(fifo) == 0) {
        *len = 0;
        return NULL;
    }
    mod_size_t wr_t = MOD_ATOMIC_LOAD(fifo->wr, MOD_ATOMIC_ORDER_RELAXED);
    mod_size_t rd_t = MOD_ATOMIC_LOAD(fifo->rd, MOD_ATOMIC_ORDER_RELAXED);
    if (wr_t >= rd_t) {
        *len = fifo->size - wr_t;
    } else {
        *len = rd_t - wr_t - 1;
    }
    return &fifo->buf[wr_t];
}

void LFifo_ReleaseLinearWrite(lfifo_t* fifo, mod_size_t len) {
    if (len == 0)
        return;
    mod_size_t wr_t = MOD_ATOMIC_LOAD(fifo->wr, MOD_ATOMIC_ORDER_ACQUIRE);
    wr_t += len;
    wr_t %= fifo->size;
    MOD_ATOMIC_STORE(fifo->wr, wr_t, MOD_ATOMIC_ORDER_RELEASE);
}

uint8_t* LFifo_AcquireLinearRead(lfifo_t* fifo, mod_size_t* len) {
    mod_size_t used = LFifo_GetUsed(fifo);
    if (used == 0) {
        *len = 0;
        return NULL;
    }
    mod_size_t wr_t = MOD_ATOMIC_LOAD(fifo->wr, MOD_ATOMIC_ORDER_RELAXED);
    mod_size_t rd_t = MOD_ATOMIC_LOAD(fifo->rd, MOD_ATOMIC_ORDER_RELAXED);
    if (wr_t >= rd_t) {
        *len = wr_t - rd_t;
    } else {
        *len = fifo->size - rd_t;
    }
    return &fifo->buf[rd_t];
}

void LFifo_ReleaseLinearRead(lfifo_t* fifo, mod_size_t len) {
    if (len == 0)
        return;
    mod_size_t rd_t = MOD_ATOMIC_LOAD(fifo->rd, MOD_ATOMIC_ORDER_ACQUIRE);
    rd_t += len;
    rd_t %= fifo->size;
    MOD_ATOMIC_STORE(fifo->rd, rd_t, MOD_ATOMIC_ORDER_RELEASE);
}

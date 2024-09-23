#ifndef __KLITE_FIFO_H
#define __KLITE_FIFO_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t* buf;
    kl_size_t size;
    kl_size_t wp;
    kl_size_t rp;
} fifo_t;

static void kl_fifo_init(void* fifo, void* buf, kl_size_t size) {
    ((fifo_t*)fifo)->buf = buf;
    ((fifo_t*)fifo)->size = size;
    ((fifo_t*)fifo)->rp = 0;
    ((fifo_t*)fifo)->wp = 0;
}

static inline kl_size_t kl_fifo_read(void* fifo, void* buf, kl_size_t size) {
    kl_size_t i;
    for (i = 0; i < size; i++) {
        if (((fifo_t*)fifo)->rp == ((fifo_t*)fifo)->wp) {
            break;
        }
        if (buf) {
            ((uint8_t*)buf)[i] = ((fifo_t*)fifo)->buf[((fifo_t*)fifo)->rp++];
        }
        if (((fifo_t*)fifo)->rp == ((fifo_t*)fifo)->size) {
            ((fifo_t*)fifo)->rp = 0;
        }
    }
    return i;
}

static inline kl_size_t kl_fifo_peek(void* fifo, void* buf, kl_size_t size) {
    kl_size_t i;
    kl_size_t rp = ((fifo_t*)fifo)->rp;
    for (i = 0; i < size; i++) {
        if (rp == ((fifo_t*)fifo)->wp) {
            break;
        }
        ((uint8_t*)buf)[i] = ((fifo_t*)fifo)->buf[rp++];
        if (rp == ((fifo_t*)fifo)->size) {
            rp = 0;
        }
    }
    return i;
}

static inline kl_size_t kl_fifo_write(void* fifo, void* buf, kl_size_t size) {
    kl_size_t i;
    kl_size_t pos;
    for (i = 0; i < size; i++) {
        pos = ((fifo_t*)fifo)->wp + 1;
        if (pos == ((fifo_t*)fifo)->size) {
            pos = 0;
        }
        if (pos == ((fifo_t*)fifo)->rp) {
            break;
        }
        ((fifo_t*)fifo)->buf[((fifo_t*)fifo)->wp] = ((uint8_t*)buf)[i];
        ((fifo_t*)fifo)->wp = pos;
    }
    return i;
}

static inline void kl_fifo_clear(void* fifo) {
    ((fifo_t*)fifo)->wp = 0;
    ((fifo_t*)fifo)->rp = 0;
}

static inline kl_size_t kl_fifo_get_free(void* fifo) {
    if (((fifo_t*)fifo)->rp > ((fifo_t*)fifo)->wp) {
        return ((fifo_t*)fifo)->rp - ((fifo_t*)fifo)->wp - 1;
    } else {
        return ((fifo_t*)fifo)->rp + ((fifo_t*)fifo)->size -
               ((fifo_t*)fifo)->wp;
    }
}

static kl_size_t kl_fifo_get_used(fifo_t* fifo) {
    if (((fifo_t*)fifo)->wp >= ((fifo_t*)fifo)->rp) {
        return ((fifo_t*)fifo)->wp - ((fifo_t*)fifo)->rp;
    } else {
        return ((fifo_t*)fifo)->wp + ((fifo_t*)fifo)->size -
               ((fifo_t*)fifo)->rp;
    }
}

#endif

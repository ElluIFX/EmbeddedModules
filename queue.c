/**
 * @file queue.c
 * @brief 实现环形队列
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-01-07
 *
 * THINK DIFFERENTLY
 */

#include "queue.h"

#undef this
#define this (*ptObj)

#if QUEUE_INT_SAFE
#define PROTECT SAFE_ATOM_CODE
#else
#define PROTECT
#endif

queue_t *queue_init(queue_t *ptObj, void *pBuffer, uint16_t hwItemSize) {
  if (pBuffer == NULL || hwItemSize == 0 || ptObj == NULL) {
    return NULL;
  }

  PROTECT {
    this.pchBuffer = pBuffer;
    this.hwSize = hwItemSize;
    this.hwHead = 0;
    this.hwTail = 0;
    this.hwLength = 0;
    this.hwPeek = this.hwHead;
    this.hwPeekLength = 0;
  }
  return ptObj;
}

bool queue_in_byte(queue_t *ptObj, uint8_t chByte) {
  if (ptObj == NULL) {
    return false;
  }

  if (this.hwHead == this.hwTail && 0 != this.hwLength) {
    return false;
  }

  PROTECT {
    this.pchBuffer[this.hwTail++] = chByte;

    if (this.hwTail >= this.hwSize) {
      this.hwTail = 0;
    }

    this.hwLength++;
    this.hwPeekLength++;
  }
  return true;
}

int16_t queue_in(queue_t *ptObj, void *pchByte, uint16_t hwLength) {
  if (ptObj == NULL) {
    return -1;
  }

  if (this.hwHead == this.hwTail && 0 != this.hwLength) {
    return 0;
  }

  PROTECT {
    if (hwLength > (this.hwSize - this.hwLength)) {
      hwLength = this.hwSize - this.hwLength;
    }

    do {
      if (hwLength < (this.hwSize - this.hwTail)) {
        memcpy(&this.pchBuffer[this.hwTail], pchByte, hwLength);
        this.hwTail += hwLength;
        break;
      }

      memcpy(&this.pchBuffer[this.hwTail], &pchByte[0],
             this.hwSize - this.hwTail);
      memcpy(&this.pchBuffer[0], &pchByte[this.hwSize - this.hwTail],
             hwLength - (this.hwSize - this.hwTail));
      this.hwTail = hwLength - (this.hwSize - this.hwTail);
    } while (0);

    this.hwLength += hwLength;
    this.hwPeekLength += hwLength;
  }
  return hwLength;
}

bool queue_out_byte(queue_t *ptObj, uint8_t *pchByte) {
  if (pchByte == NULL || ptObj == NULL) {
    return false;
  }

  if (this.hwHead == this.hwTail && 0 == this.hwLength) {
    return false;
  }

  PROTECT {
    *pchByte = this.pchBuffer[this.hwHead++];

    if (this.hwHead >= this.hwSize) {
      this.hwHead = 0;
    }

    this.hwLength--;
    this.hwPeek = this.hwHead;
    this.hwPeekLength = this.hwLength;
  }
  return true;
}

int16_t queue_out(queue_t *ptObj, void *pchByte, uint16_t hwLength) {
  if (pchByte == NULL || ptObj == NULL) {
    return -1;
  }

  if (this.hwHead == this.hwTail && 0 == this.hwLength) {
    return 0;
  }

  PROTECT {
    if (hwLength > this.hwLength) {
      hwLength = this.hwLength;
    }

    do {
      if (hwLength < (this.hwSize - this.hwHead)) {
        memcpy(pchByte, &this.pchBuffer[this.hwHead], hwLength);
        this.hwHead += hwLength;
        break;
      }

      memcpy(&pchByte[0], &this.pchBuffer[this.hwHead],
             this.hwSize - this.hwHead);
      memcpy(&pchByte[this.hwSize - this.hwHead], &this.pchBuffer[0],
             hwLength - (this.hwSize - this.hwHead));
      this.hwHead = hwLength - (this.hwSize - this.hwHead);
    } while (0);

    this.hwLength -= hwLength;
    this.hwPeek = this.hwHead;
    this.hwPeekLength = this.hwLength;
  }
  return hwLength;
}

bool queue_check_empty(queue_t *ptObj) {
  if (ptObj == NULL) {
    return false;
  }

  if (this.hwHead == this.hwTail && 0 == this.hwLength) {
    return true;
  }

  return false;
}

int16_t queue_get_count(queue_t *ptObj) {
  if (ptObj == NULL) {
    return -1;
  }

  return (this.hwLength);
}

int16_t queue_get_available(queue_t *ptObj) {
  if (ptObj == NULL) {
    return -1;
  }

  return (this.hwSize - this.hwLength);
}

bool queue_peek_check_empty(queue_t *ptObj) {
  if (ptObj == NULL) {
    return false;
  }

  if (this.hwPeek == this.hwTail && 0 == this.hwPeekLength) {
    return true;
  }

  return false;
}

bool queue_peek_byte(queue_t *ptObj, uint8_t *pchByte) {
  if (ptObj == NULL || pchByte == NULL) {
    return false;
  }

  if (this.hwPeek == this.hwTail && 0 == this.hwPeekLength) {
    return false;
  }

  PROTECT {
    *pchByte = this.pchBuffer[this.hwPeek++];

    if (this.hwPeek >= this.hwSize) {
      this.hwPeek = 0;
    }

    this.hwPeekLength--;
  }
  return true;
}

int16_t queue_peek(queue_t *ptObj, void *pchByte, uint16_t hwLength) {
  if (ptObj == NULL || pchByte == NULL) {
    return -1;
  }

  if (this.hwPeek == this.hwTail && 0 == this.hwPeekLength) {
    return 0;
  }

  PROTECT {
    if (hwLength > this.hwPeekLength) {
      hwLength = this.hwPeekLength;
    }

    do {
      if (hwLength < (this.hwSize - this.hwPeek)) {
        memcpy(pchByte, &this.pchBuffer[this.hwPeek], hwLength);
        this.hwPeek += hwLength;
        break;
      }

      memcpy(&pchByte[0], &this.pchBuffer[this.hwPeek],
             this.hwSize - this.hwPeek);
      memcpy(&pchByte[this.hwSize - this.hwPeek], &this.pchBuffer[0],
             hwLength - (this.hwSize - this.hwPeek));
      this.hwPeek = hwLength - (this.hwSize - this.hwPeek);
    } while (0);

    this.hwPeekLength -= hwLength;
  }
  return hwLength;
}

bool queue_reset_peek_pos(queue_t *ptObj) {
  if (ptObj == NULL) {
    return false;
  }

  PROTECT {
    this.hwPeek = this.hwHead;
    this.hwPeekLength = this.hwLength;
  }
  return true;
}

bool queue_pop_peeked(queue_t *ptObj) {
  if (ptObj == NULL) {
    return false;
  }

  PROTECT {
    this.hwHead = this.hwPeek;
    this.hwLength = this.hwPeekLength;
  }
  return true;
}

uint16_t queue_get_peek_pos(queue_t *ptObj) {
  uint16_t hwCount;

  if (ptObj == NULL) {
    return false;
  }

  PROTECT {
    if (this.hwPeek >= this.hwHead) {
      hwCount = this.hwPeek - this.hwHead;
    } else {
      hwCount = this.hwSize - this.hwHead + this.hwPeek;
    }
  }
  return hwCount;
}

bool queue_set_peek_pos(queue_t *ptObj, uint16_t hwCount) {
  if (ptObj == NULL) {
    return false;
  }

  PROTECT {
    if (this.hwHead + hwCount < this.hwSize) {
      this.hwPeek = this.hwHead + hwCount;
    } else {
      this.hwPeek = hwCount - (this.hwSize - this.hwHead);
    }

    this.hwPeekLength = this.hwPeekLength - hwCount;
  }
  return true;
}

#include "log.h"
#define DBG_PRINT LOG_RAW

void queue_debug(queue_t *ptObj) {
  if (ptObj == NULL) {
    return;
  }
  DBG_PRINT("\r\n[DEBUG] len:%d head:%d tail:%d\r\n", this.hwLength,
            this.hwHead, this.hwTail);
  char line[24 + 4 + 16 + 1] = {0};
  memset(line, ' ', 24 + 4 + 16);  // space
  // print full line
  uint8_t tmp = 0;
  uint16_t i = 0;
  for (; i < this.hwLength / 8; i++) {
    for (uint16_t j = 0; j < 8; j++) {
      tmp = this.pchBuffer[this.hwHead + i * 8 + j];
      sprintf(line + j * 3, "%02X", tmp);
      line[j * 3 + 2] = ' ';  // remove '\0'
      if (tmp > 31 && tmp < 127) {
        line[24 + 4 + j * 2] = tmp;  // ASCII
      } else {
        line[24 + 4 + j * 2] = '.';
      }
    }
    DBG_PRINT("%3d: %s\r\n", i * 8, line);
  }
  // print last line
  if (this.hwLength % 8) {
    memset(line, ' ', 24 + 4 + 8);  // space
    for (uint16_t j = 0; j < 8; j++) {
      if (j < this.hwLength % 8) {
        tmp = this.pchBuffer[this.hwHead + (this.hwLength / 8) * 8 + j];
        sprintf(line + j * 3, "%02X", tmp);
        line[j * 3 + 2] = ' ';  // remove '\0'
        if (tmp > 31 && tmp < 127) {
          line[24 + 4 + j * 2] = tmp;  // ASCII
        } else {
          line[24 + 4 + j * 2] = '.';
        }
      } else {
        line[j * 3] = '_';
        line[j * 3 + 1] = '_';
        line[24 + 4 + j * 2] = '.';
      }
    }
    DBG_PRINT("%3d: %s\r\n\r\n", i * 8, line);
  } else {
    DBG_PRINT("\r\n");
  }
}

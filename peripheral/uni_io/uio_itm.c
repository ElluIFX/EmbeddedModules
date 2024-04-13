/**
 * @file uio_itm.c
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#include "uio_itm.h"

#if UIO_CFG_ENABLE_ITM

#include <stdio.h>
#include <string.h>

#include "lwprintf.h"

volatile int32_t ITM_RxBuffer = ITM_RXBUFFER_EMPTY;

void itm_send_char(uint8_t port, uint8_t ch) {
  if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) && /* ITM enabled */
      ((ITM->TER & (1 << port)) != 0UL))          /* ITM Port #0 enabled */
  {
    while (ITM->PORT[port].u32 == 0UL) {
      __NOP();
    }
    ITM->PORT[port].u8 = ch;
  }
}

int32_t itm_recv_char(void) {
  int32_t ch = -1;
  if (ITM_RxBuffer != ITM_RXBUFFER_EMPTY) {
    ch = ITM_RxBuffer;
    ITM_RxBuffer = ITM_RXBUFFER_EMPTY;
  }
  return (ch);
}

static int itm_lwprintf_fn(int ch, lwprintf_t *lwobj) {
  if (ch == '\0') return 0;
  itm_send_char((uint8_t)(size_t)lwobj->arg, ch);
  return ch;
}

int itm_printf(uint8_t port, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  lwprintf_t lwp_pub;
  lwp_pub.out_fn = itm_lwprintf_fn;
  lwp_pub.arg = (void *)(size_t)port;
  int sendLen = lwprintf_vprintf_ex(&lwp_pub, fmt, ap);
  va_end(ap);
  return sendLen;
}

int itm_write(uint8_t port, uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    itm_send_char(port, data[i]);
  }
  return 0;
}

size_t itm_read(uint8_t *data, size_t len, m_time_t timeout_ms) {
  m_time_t time = m_time_ms();
  size_t i = 0;
  int32_t tmp;
  do {
    tmp = itm_recv_char();
    if (tmp != -1) {
      data[i++] = (uint8_t)tmp;
      if (i >= len) break;
      time = m_time_ms();
    }
  } while (m_time_ms() - time < timeout_ms);
  return i;
}

#endif  // UIO_CFG_ENABLE_ITM

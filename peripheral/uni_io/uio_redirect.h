/**
 * @file uio_redirect.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#ifndef __UIO_REDIRECT_H__
#define __UIO_REDIRECT_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "modules.h"

#if UIO_CFG_PRINTF_REDIRECT
#include "uio_cdc.h"
#include "uio_itm.h"
#include "uio_uart.h"

#undef printf
#if UIO_CFG_PRINTF_DISABLE
#define printf(...) ((void)0)
#define printf_flush() ((void)0)
#elif UIO_CFG_PRINTF_USE_RTT
#include "SEGGER_RTT.h"
#define printf(...) SEGGER_RTT_printf(0, ##__VA_ARGS__)
#define printf_flush() ((void)0)
#define printf_block(...) SEGGER_RTT_printf(0, ##__VA_ARGS__)
#define println(fmt, ...) \
    SEGGER_RTT_printf(0, fmt UIO_CFG_PRINTF_ENDL, ##__VA_ARGS__)
#elif UIO_CFG_PRINTF_USE_CDC && UIO_CFG_ENABLE_CDC
#define printf(...) cdc_printf(__VA_ARGS__)
#define printf_flush() cdc_printf_flush()
#define printf_block(...) cdc_printf(__VA_ARGS__)
#define println(fmt, ...) cdc_printf(fmt UIO_CFG_PRINTF_ENDL, ##__VA_ARGS__)
#elif UIO_CFG_PRINTF_USE_ITM
#define printf(...) itm_printf(0, ##__VA_ARGS__)
#define printf_flush() ((void)0)
#define printf_block(...) itm_printf(0, ##__VA_ARGS__)
#define println(fmt, ...) itm_printf(0, fmt UIO_CFG_PRINTF_ENDL, ##__VA_ARGS__)
#else
#define printf(fmt, ...) \
    uart_printf(&UIO_CFG_PRINTF_UART_PORT, fmt, ##__VA_ARGS__)
#define printf_flush() uart_flush(&UIO_CFG_PRINTF_UART_PORT)
#define printf_block(...) \
    uart_printf_block(&UIO_CFG_PRINTF_UART_PORT, ##__VA_ARGS__)
#define println(fmt, ...) printf(fmt UIO_CFG_PRINTF_ENDL, ##__VA_ARGS__)
#endif  // UIO_CFG_PRINTF_USE_*

#if UIO_CFG_PRINTF_REDIRECT_PUTX
static inline int __putchar(int ch) {
#if UIO_CFG_PRINTF_USE_RTT
    SEGGER_RTT_Write(0, &ch, 1);
#elif UIO_CFG_PRINTF_USE_CDC && UIO_CFG_ENABLE_CDC
    cdc_write(&ch, 1);
#elif UIO_CFG_PRINTF_USE_ITM
    itm_send_char(0, ch);
#else
    uart_write(&UIO_CFG_PRINTF_UART_PORT, (uint8_t*)&ch, 1);
#endif  // UIO_CFG_PRINTF_USE_*
    return ch;
}

static inline int __puts(const char* s) {
#if UIO_CFG_PRINTF_USE_RTT
    SEGGER_RTT_Write(0, s, strlen(s));
#elif UIO_CFG_PRINTF_USE_CDC && UIO_CFG_ENABLE_CDC
    cdc_write((uint8_t*)s, strlen(s));
#elif UIO_CFG_PRINTF_USE_ITM
    while (*s) {
        itm_send_char(0, *s++);
    }
#else
    size_t strlen(const char* s);
    uart_write(&UIO_CFG_PRINTF_UART_PORT, (uint8_t*)s, strlen(s));
#endif  // UIO_CFG_PRINTF_USE_*
    return 0;
}

static inline int __getchar(void) {
#if UIO_CFG_PRINTF_USE_RTT
    char ch;
    SEGGER_RTT_Read(0, &ch, 1);
    return ch;
#elif UIO_CFG_PRINTF_USE_CDC && UIO_CFG_ENABLE_CDC
    return -1;  // not supported
#elif UIO_CFG_PRINTF_USE_ITM
    return itm_recv_char();
#else
    return -1;  // not supported
#endif  // UIO_CFG_PRINTF_USE_*
}

#undef putchar
#undef puts
#undef getchar
#define putchar __putchar
#define puts __puts
#define getchar __getchar
#endif  // UIO_CFG_PRINTF_REDIRECT_PUTX

#endif  // UIO_CFG_PRINTF_REDIRECT

#ifdef __cplusplus
}
#endif

#endif  // __UIO_REDIRECT_H__

/**
 * @file uio_vofa.c
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-04-12
 *
 * THINK DIFFERENTLY
 */

#include "uio_vofa.h"

#if UIO_CFG_ENABLE_VOFA

#include <string.h>

static float VOFA_Buffer[UIO_CFG_VOFA_BUFFER_SIZE + 1];
static uint8_t vofa_index = 0;
static const uint32_t vofa_endbit = 0x7F800000;

__attribute__((always_inline, flatten)) void vofa_add(float value) {
    if (vofa_index < UIO_CFG_VOFA_BUFFER_SIZE)
        VOFA_Buffer[vofa_index++] = value;
}

void vofa_add_array(float* value, uint8_t len) {
    if (vofa_index + len >= UIO_CFG_VOFA_BUFFER_SIZE)
        return;
    memcpy(&VOFA_Buffer[vofa_index], value, len * sizeof(float));
    vofa_index += len;
}

void vofa_clear(void) {
    vofa_index = 0;
}

void vofa_send_uart(UART_HandleTypeDef* huart) {
    if (vofa_index == 0)
        return;
    vofa_add_array((float*)&vofa_endbit, 1);
    uart_write(huart, (uint8_t*)VOFA_Buffer, vofa_index * sizeof(float));
    vofa_index = 0;
}

void vofa_send_uart_fast(UART_HandleTypeDef* huart) {
    if (vofa_index == 0)
        return;
    memcpy(&VOFA_Buffer[vofa_index], &vofa_endbit, sizeof(float));
    uart_write_fast(huart, (uint8_t*)VOFA_Buffer, ++vofa_index * sizeof(float));
    vofa_index = 0;
}

#if UIO_CFG_ENABLE_CDC
#include "uio_cdc.h"

void vofa_send_cdc(void) {
    if (vofa_index == 0)
        return;
    vofa_add_array((float*)&vofa_endbit, 1);
    cdc_write((uint8_t*)VOFA_Buffer, vofa_index * sizeof(float));
    vofa_index = 0;
}
#endif  // UIO_CFG_ENABLE_CDC
#endif  // UIO_CFG_ENABLE_VOFA

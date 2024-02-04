#pragma once

#include <stdbool.h>
#include <stdint.h>


void cdc_acm_data_recv_callback(uint8_t *buf, uint32_t len);

void cdc_acm_data_send_cplt_callback(void);

// ret: 0 - OK, 1 - disconnected, 2 - busy, 3 - error
uint8_t cdc_acm_data_send(uint8_t *buf, uint32_t len);

// NULL - error, not NULL - OK
uint8_t *cdc_acm_data_send_raw_acquire(uint32_t *len);

// ret: 0 - OK, 1 - disconnected, 2 - busy, 3 - error
uint8_t cdc_acm_data_send_raw_commit(uint32_t len);

bool cdc_acm_connected(void);

bool cdc_acm_idle(void);

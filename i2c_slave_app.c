#include <log.h>
#include <stdio.h>
#include <string.h>

#include "i2c_slave.h"

static uint8_t mem[256] = {0};
static uint8_t op_offset = 0;
static uint8_t op_addr = 0x00;

void Slave_I2C_RecvDone_Callback(uint16_t offset, uint16_t len) {
  LOG_D("RX: 0x%02X %d [% *K]", offset, len, len, mem + offset);
  if (offset == 0x42 && len >= 1) {
    Slave_I2C_SetAddress(mem[0x42]);
  }
}

void Slave_I2C_PreSend_Callback(uint16_t offset) {
  LOG_D("QR: 0x%02X", offset);
}

void Slave_I2C_SendDone_Callback(uint16_t offset, uint16_t len) {
  LOG_D("TX: 0x%02X %d [% *K]", offset, len, len, mem + offset);
}

enum {
  STATE_IDLE = 0,
  STATE_READY,
  STATE_TX,
  STATE_RX,
} i2c_state = STATE_IDLE;

void Slave_I2C_Init_Callback(void) { i2c_state = STATE_IDLE; }

void Slave_I2C_TransmitEnd_Callback(void) {
  if (i2c_state == STATE_IDLE) return;
  Slave_I2C_SetITEnable(0);
  if (i2c_state == STATE_RX) {
    Slave_I2C_RecvDone_Callback(op_addr, op_offset - op_addr);
  } else if (i2c_state == STATE_READY) {
    Slave_I2C_PreSend_Callback(op_addr);
  } else if (i2c_state == STATE_TX) {
    Slave_I2C_SendDone_Callback(op_addr, op_offset - op_addr - 1);
  }
  if (i2c_state != STATE_READY) {  // 重置状态
    i2c_state = STATE_IDLE;
    op_offset = 0;
    op_addr = 0;
  }
  Slave_I2C_SetITEnable(1);
}

void Slave_I2C_TransmitBegin_Callback(void) {}

void Slave_I2C_TransmitIn_Callback(uint8_t data) {
  if (i2c_state == STATE_IDLE) {
    op_offset = data;
    op_addr = data;
    i2c_state = STATE_READY;
    return;
  }
  i2c_state = STATE_RX;
  mem[op_offset++ % sizeof(mem)] = data;
}

void Slave_I2C_TransmitOut_Callback(uint8_t* data) {
  i2c_state = STATE_TX;
  *data = mem[op_offset++ % sizeof(mem)];
}

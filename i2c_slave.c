
#include "i2c_slave.h"

#include <log.h>
#include <stdio.h>
#include <string.h>

static uint8_t slave_i2c_addr = 0x00;
static uint8_t slave_i2c_mem[256] = {0};
static uint8_t op_offset = 0;
static uint8_t op_addr = 0x00;

enum {
  STATE_IDLE = 0,
  STATE_READY,
  STATE_TX,
  STATE_RX,
} i2c_state = STATE_IDLE;

__weak void Slave_I2C_RecvDone_Callback(uint16_t offset, uint8_t* mem,
                                        uint16_t len) {
  LOG_LIMIT(25, "RX: 0x%02X %d", offset, len, len, mem + offset);
}
__weak void Slave_I2C_PreSend_Callback(uint16_t offset, uint8_t* mem) {
  LOG_LIMIT(25, "QR: 0x%02X", offset);
}
__weak void Slave_I2C_SendDone_Callback(uint16_t offset, uint8_t* mem,
                                        uint16_t len) {
  LOG_LIMIT(25, "TX: 0x%02X %d", offset, len, len, mem + offset);
}
uint8_t* Slave_I2C_GetMem(void) { return slave_i2c_mem; }

static void Slave_Enable_IT(void) {
  LL_I2C_Enable(_SLAVE_I2C_INSTANCE);
  // LL_I2C_DisableClockStretching(_SLAVE_I2C_INSTANCE);
  // LL_I2C_EnableClockStretching(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_ADDR(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_TX(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_RX(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_NACK(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_ERR(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_STOP(_SLAVE_I2C_INSTANCE);
}

static void Slave_Disable_IT(void) {
  LL_I2C_DisableIT_ADDR(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_TX(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_RX(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_NACK(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_ERR(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_STOP(_SLAVE_I2C_INSTANCE);
}

void Slave_I2C_Error_IRQHandler(void) {
  Slave_Disable_IT();
  Slave_Enable_IT();
  LL_I2C_AcknowledgeNextData(_SLAVE_I2C_INSTANCE, LL_I2C_NACK);
}

void Slave_I2C_Init(const uint8_t addr) {
  slave_i2c_addr = (addr << 1);
  i2c_state = STATE_IDLE;
  Slave_Disable_IT();
  Slave_Enable_IT();
}

static void Slave_Check_Callback(void) {  // 检查是否执行回调函数
  if (i2c_state == STATE_IDLE) return;
  Slave_Disable_IT();  // 关闭中断
  if (i2c_state == STATE_RX) {
    Slave_I2C_RecvDone_Callback(op_addr, (uint8_t*)slave_i2c_mem,
                                op_offset - op_addr);
  } else if (i2c_state == STATE_READY) {
    Slave_I2C_PreSend_Callback(op_addr, (uint8_t*)slave_i2c_mem);
  } else if (i2c_state == STATE_TX) {
    Slave_I2C_SendDone_Callback(op_addr, (uint8_t*)slave_i2c_mem,
                                op_offset - op_addr - 1);
  }
  if (i2c_state != STATE_READY) {  // 重置状态
    i2c_state = STATE_IDLE;
    op_offset = 0;
    op_addr = 0;
  }
  Slave_Enable_IT();  // 打开中断
}

static void Slave_Reception_Callback(void) {  // 接收数据完成
  uint8_t data = LL_I2C_ReceiveData8(_SLAVE_I2C_INSTANCE);
  if (i2c_state == STATE_IDLE) {
    op_offset = data;
    op_addr = data;
    i2c_state = STATE_READY;
    return;
  }
  i2c_state = STATE_RX;
  slave_i2c_mem[op_offset] = data;
  op_offset = (op_offset + 1) % sizeof(slave_i2c_mem);
}

static void Slave_Ready_To_Transmit_Callback(void) {  // 准备发送数据
  i2c_state = STATE_TX;
  LL_I2C_TransmitData8(_SLAVE_I2C_INSTANCE, slave_i2c_mem[op_offset]);
  op_offset = (op_offset + 1) % sizeof(slave_i2c_mem);
}

void Slave_I2C_IRQHandler(void) {
  /* Check ADDR flag value in ISR register */
  if (LL_I2C_IsActiveFlag_ADDR(_SLAVE_I2C_INSTANCE)) {
    /* Verify the Address Match with the OWN Slave address */
    if (LL_I2C_GetAddressMatchCode(_SLAVE_I2C_INSTANCE) == slave_i2c_addr) {
      Slave_Check_Callback();
      /* Verify the transfer direction, a write direction, Slave enters receiver
       * mode */
      if (LL_I2C_GetTransferDirection(_SLAVE_I2C_INSTANCE) ==
          LL_I2C_DIRECTION_WRITE) {
        /* Clear ADDR flag value in ISR register */
        LL_I2C_ClearFlag_ADDR(_SLAVE_I2C_INSTANCE);
        /* Enable Receive Interrupt */
        LL_I2C_EnableIT_RX(_SLAVE_I2C_INSTANCE);
      }
      /* Verify the transfer direction, a read direction, Slave enters
         transmitter mode */
      else if (LL_I2C_GetTransferDirection(_SLAVE_I2C_INSTANCE) ==
               LL_I2C_DIRECTION_READ) {
        /* Clear ADDR flag value in ISR register */
        LL_I2C_ClearFlag_ADDR(_SLAVE_I2C_INSTANCE);
        /* Enable Transmit Interrupt */
        LL_I2C_EnableIT_TX(_SLAVE_I2C_INSTANCE);
      } else {
        /* Clear ADDR flag value in ISR register */
        LL_I2C_ClearFlag_ADDR(_SLAVE_I2C_INSTANCE);
        /* Call Error function */
        Slave_I2C_Error_IRQHandler();
      }
    } else {
      /* Clear ADDR flag value in ISR register */
      LL_I2C_ClearFlag_ADDR(_SLAVE_I2C_INSTANCE);
      /* Call Error function */
      Slave_I2C_Error_IRQHandler();
    }
  }
  /* Check NACK flag value in ISR register */
  else if (LL_I2C_IsActiveFlag_NACK(_SLAVE_I2C_INSTANCE)) {
    /* End of Transfer */
    LL_I2C_ClearFlag_NACK(_SLAVE_I2C_INSTANCE);
  }
  /* Check TXIS flag value in ISR register */
  else if (LL_I2C_IsActiveFlag_TXIS(_SLAVE_I2C_INSTANCE)) {
    /* Call function Slave Ready to Transmit Callback */
    Slave_Ready_To_Transmit_Callback();
  }
  /* Check RXNE flag value in ISR register */
  else if (LL_I2C_IsActiveFlag_RXNE(_SLAVE_I2C_INSTANCE)) {
    /* Call function Slave Reception Callback */
    Slave_Reception_Callback();
  }
  /* Check STOP flag value in ISR register */
  else if (LL_I2C_IsActiveFlag_STOP(_SLAVE_I2C_INSTANCE)) {
    /* End of Transfer */
    LL_I2C_ClearFlag_STOP(_SLAVE_I2C_INSTANCE);
    /* Check TXE flag value in ISR register */
    if (!LL_I2C_IsActiveFlag_TXE(_SLAVE_I2C_INSTANCE)) {
      /* Flush the TXDR register */
      LL_I2C_ClearFlag_TXE(_SLAVE_I2C_INSTANCE);
    }
    /* Call function Slave Complete Callback */
    Slave_Check_Callback();
  }
  /* Check TXE flag value in ISR register */
  else if (!LL_I2C_IsActiveFlag_TXE(_SLAVE_I2C_INSTANCE)) {
    /* Do nothing */
    /* This Flag will be set by hardware when the TXDR register is empty */
    /* If needed, use LL_I2C_ClearFlag_TXE() interface to flush the TXDR
     * register  */
  } else {
    /* Call Error function */
    Slave_I2C_Error_IRQHandler();
  }
}

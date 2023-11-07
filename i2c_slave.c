
#include "i2c_slave.h"

__weak void Slave_I2C_Init_Callback(void) {}
__weak void Slave_I2C_TransmitBegin_Callback(void) {}
__weak void Slave_I2C_TransmitEnd_Callback(void) {}
__weak void Slave_I2C_TransmitIn_Callback(uint8_t data) {}
__weak void Slave_I2C_TransmitOut_Callback(uint8_t* data) {}

static inline void Slave_Enable_IT(void) {
  LL_I2C_Enable(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_ADDR(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_TX(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_RX(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_NACK(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_ERR(_SLAVE_I2C_INSTANCE);
  LL_I2C_EnableIT_STOP(_SLAVE_I2C_INSTANCE);
}

static inline void Slave_Disable_IT(void) {
  LL_I2C_DisableIT_ADDR(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_TX(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_RX(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_NACK(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_ERR(_SLAVE_I2C_INSTANCE);
  LL_I2C_DisableIT_STOP(_SLAVE_I2C_INSTANCE);
}

static uint8_t slave_addr = 0x00;

void Slave_I2C_Init(void) {
  slave_addr = READ_BIT(_SLAVE_I2C_INSTANCE->OAR1, I2C_OAR1_OA1);
  Slave_Disable_IT();
  Slave_Enable_IT();
  Slave_I2C_Init_Callback();
}

void Slave_I2C_SetITEnable(uint8_t state) {
  if (state) {
    Slave_Enable_IT();
  } else {
    Slave_Disable_IT();
  }
}

void Slave_I2C_SetAddress(uint8_t addr) {
  if (addr < 0x08 || addr > 0x77 || addr == (slave_addr >> 1)) return;
  slave_addr = (addr << 1);
  Slave_I2C_SetEnable(0);
  LL_I2C_DisableOwnAddress1(_SLAVE_I2C_INSTANCE);
  LL_I2C_SetOwnAddress1(_SLAVE_I2C_INSTANCE, slave_addr,
                        LL_I2C_OWNADDRESS1_7BIT);
  LL_I2C_EnableOwnAddress1(_SLAVE_I2C_INSTANCE);
  Slave_I2C_SetEnable(1);
}

void Slave_I2C_SetEnable(uint8_t state) {
  if (state) {
    LL_I2C_Enable(_SLAVE_I2C_INSTANCE);
    Slave_Enable_IT();
  } else {
    Slave_Disable_IT();
    LL_I2C_Disable(_SLAVE_I2C_INSTANCE);
  }
}

static inline void Slave_I2C_Transmit_Stop_Callback(void) {
  // Slave_Disable_IT();
  Slave_I2C_TransmitEnd_Callback();
  // Slave_Enable_IT();
}

static inline void Slave_I2C_Transmit_Start_Callback(void) {
  // Slave_Disable_IT();
  Slave_I2C_TransmitBegin_Callback();
  // Slave_Enable_IT();
}

static inline void Slave_Reception_Callback(void) {
  // Slave_Disable_IT();
  uint8_t data = LL_I2C_ReceiveData8(_SLAVE_I2C_INSTANCE);
  Slave_I2C_TransmitIn_Callback(data);
  // Slave_Enable_IT();
}

static inline void Slave_Ready_To_Transmit_Callback(void) {
  // Slave_Disable_IT();
  uint8_t data = 0x00;
  Slave_I2C_TransmitOut_Callback(&data);
  LL_I2C_TransmitData8(_SLAVE_I2C_INSTANCE, data);
  // Slave_Enable_IT();
}

void Slave_I2C_IRQHandler(void) {
  /* Check ADDR flag value in ISR register */
  if (LL_I2C_IsActiveFlag_ADDR(_SLAVE_I2C_INSTANCE)) {
    /* Clear ADDR flag value in ISR register */
    LL_I2C_ClearFlag_ADDR(_SLAVE_I2C_INSTANCE);
    /* Verify the Address Match with the OWN Slave address */
    if (LL_I2C_GetAddressMatchCode(_SLAVE_I2C_INSTANCE) == slave_addr) {
      Slave_I2C_Transmit_Start_Callback();
      /* Verify the transfer direction, a write direction, Slave enters receiver
       * mode */
      if (LL_I2C_GetTransferDirection(_SLAVE_I2C_INSTANCE) ==
          LL_I2C_DIRECTION_WRITE) {
        /* Enable Receive Interrupt */
        LL_I2C_EnableIT_RX(_SLAVE_I2C_INSTANCE);
      }
      /* Verify the transfer direction, a read direction, Slave enters
         transmitter mode */
      else if (LL_I2C_GetTransferDirection(_SLAVE_I2C_INSTANCE) ==
               LL_I2C_DIRECTION_READ) {
        /* Enable Transmit Interrupt */
        LL_I2C_EnableIT_TX(_SLAVE_I2C_INSTANCE);
      } else {
        /* Call Error function */
        Slave_I2C_Error_IRQHandler();
      }
    } else {
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
    Slave_I2C_Transmit_Stop_Callback();
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

void Slave_I2C_Error_IRQHandler(void) {
  Slave_Disable_IT();
  Slave_Enable_IT();
  LL_I2C_AcknowledgeNextData(_SLAVE_I2C_INSTANCE, LL_I2C_NACK);
}

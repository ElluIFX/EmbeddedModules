
#include "i2c_slave.h"
#include "i2c.h"

__weak void slave_i2c_transmit_begin_handler(I2C_TypeDef* I2Cx) {}

__weak void slave_i2c_transmit_end_handler(I2C_TypeDef* I2Cx) {}

__weak void slave_i2c_transmit_in_handler(I2C_TypeDef* I2Cx, uint8_t data) {}

__weak void slave_i2c_transmit_out_handler(I2C_TypeDef* I2Cx, uint8_t* data) {}

static inline void slave_enable_it(I2C_TypeDef* I2Cx) {
    LL_I2C_Enable(I2Cx);
    LL_I2C_EnableIT_ADDR(I2Cx);
    LL_I2C_EnableIT_TX(I2Cx);
    LL_I2C_EnableIT_RX(I2Cx);
    LL_I2C_EnableIT_NACK(I2Cx);
    LL_I2C_EnableIT_ERR(I2Cx);
    LL_I2C_EnableIT_STOP(I2Cx);
}

static inline void slave_disable_it(I2C_TypeDef* I2Cx) {
    LL_I2C_DisableIT_ADDR(I2Cx);
    LL_I2C_DisableIT_TX(I2Cx);
    LL_I2C_DisableIT_RX(I2Cx);
    LL_I2C_DisableIT_NACK(I2Cx);
    LL_I2C_DisableIT_ERR(I2Cx);
    LL_I2C_DisableIT_STOP(I2Cx);
}

#define GET_SLAVE_ADDR(I2Cx) (READ_BIT(I2Cx->OAR1, I2C_OAR1_OA1))

void slave_i2c_init(I2C_TypeDef* I2Cx) {
    slave_disable_it(I2Cx);
    slave_enable_it(I2Cx);
}

void slave_i2c_set_interrupt(I2C_TypeDef* I2Cx, uint8_t state) {
    if (state) {
        slave_enable_it(I2Cx);
    } else {
        slave_disable_it(I2Cx);
    }
}

void slave_i2c_set_address(I2C_TypeDef* I2Cx, uint8_t addr) {
    if (addr < 0x08 || addr > 0x77 || addr == (GET_SLAVE_ADDR(I2Cx) >> 1))
        return;
    slave_i2c_set_enable(I2Cx, 0);
    LL_I2C_DisableOwnAddress1(I2Cx);
    LL_I2C_SetOwnAddress1(I2Cx, (addr << 1), LL_I2C_OWNADDRESS1_7BIT);
    LL_I2C_EnableOwnAddress1(I2Cx);
    slave_i2c_set_enable(I2Cx, 1);
}

void slave_i2c_set_enable(I2C_TypeDef* I2Cx, uint8_t state) {
    if (state) {
        LL_I2C_Enable(I2Cx);
        slave_enable_it(I2Cx);
    } else {
        slave_disable_it(I2Cx);
        LL_I2C_Disable(I2Cx);
    }
}

static inline void transmit_stop_handler(I2C_TypeDef* I2Cx) {
    slave_i2c_transmit_end_handler(I2Cx);
}

static inline void transmit_start_handler(I2C_TypeDef* I2Cx) {
    slave_i2c_transmit_begin_handler(I2Cx);
}

static inline void reception_handler(I2C_TypeDef* I2Cx) {
    uint8_t data = LL_I2C_ReceiveData8(I2Cx);
    slave_i2c_transmit_in_handler(I2Cx, data);
}

static inline void ready_to_transmit_handler(I2C_TypeDef* I2Cx) {
    uint8_t data = 0x00;
    slave_i2c_transmit_out_handler(I2Cx, &data);
    LL_I2C_TransmitData8(I2Cx, data);
}

void slave_i2c_error_irq(I2C_TypeDef* I2Cx) {
    slave_disable_it(I2Cx);
    slave_enable_it(I2Cx);
    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
}

void slave_i2c_event_irq(I2C_TypeDef* I2Cx) {
    /* Check ADDR flag value in ISR register */
    if (LL_I2C_IsActiveFlag_ADDR(I2Cx)) {
        /* Clear ADDR flag value in ISR register */
        LL_I2C_ClearFlag_ADDR(I2Cx);
        /* Verify the Address Match with the OWN Slave address */
        if (LL_I2C_GetAddressMatchCode(I2Cx) == GET_SLAVE_ADDR(I2Cx)) {
            transmit_start_handler(I2Cx);
            /* Verify the transfer direction, a write direction, Slave enters receiver
       * mode */
            if (LL_I2C_GetTransferDirection(I2Cx) == LL_I2C_DIRECTION_WRITE) {
                /* Enable Receive Interrupt */
                LL_I2C_EnableIT_RX(I2Cx);
            }
            /* Verify the transfer direction, a read direction, Slave enters
         transmitter mode */
            else if (LL_I2C_GetTransferDirection(I2Cx) ==
                     LL_I2C_DIRECTION_READ) {
                /* Enable Transmit Interrupt */
                LL_I2C_EnableIT_TX(I2Cx);
            } else {
                /* Call Error function */
                slave_i2c_error_irq(I2Cx);
            }
        } else {
            /* Call Error function */
            slave_i2c_error_irq(I2Cx);
        }
    }
    /* Check NACK flag value in ISR register */
    else if (LL_I2C_IsActiveFlag_NACK(I2Cx)) {
        /* End of Transfer */
        LL_I2C_ClearFlag_NACK(I2Cx);
    }
    /* Check TXIS flag value in ISR register */
    else if (LL_I2C_IsActiveFlag_TXIS(I2Cx)) {
        /* Call function Slave Ready to Transmit Callback */
        ready_to_transmit_handler(I2Cx);
    }
    /* Check RXNE flag value in ISR register */
    else if (LL_I2C_IsActiveFlag_RXNE(I2Cx)) {
        /* Call function Slave Reception Callback */
        reception_handler(I2Cx);
    }
    /* Check STOP flag value in ISR register */
    else if (LL_I2C_IsActiveFlag_STOP(I2Cx)) {
        /* End of Transfer */
        LL_I2C_ClearFlag_STOP(I2Cx);
        /* Check TXE flag value in ISR register */
        if (!LL_I2C_IsActiveFlag_TXE(I2Cx)) {
            /* Flush the TXDR register */
            LL_I2C_ClearFlag_TXE(I2Cx);
        }
        /* Call function Slave Complete Callback */
        transmit_stop_handler(I2Cx);
    }
    /* Check TXE flag value in ISR register */
    else if (!LL_I2C_IsActiveFlag_TXE(I2Cx)) {
        /* Do nothing */
        /* This Flag will be set by hardware when the TXDR register is empty */
        /* If needed, use LL_I2C_ClearFlag_TXE() interface to flush the TXDR
     * register  */
    } else {
        /* Call Error function */
        slave_i2c_error_irq(I2Cx);
    }
}

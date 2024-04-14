/**
 * @file ll_i2c_poll.c
 * @brief LL I2C interface implementation. Polling mode
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-10-15
 *
 * THINK DIFFERENTLY
 */

#include "ll_i2c.h"

#if !LL_IIC_CFG_USE_IT && __has_include("i2c.h")

#include "i2c.h"
#define LOG_MODULE "ll-i2c"
#include "log.h"
#if 0 /* 1: enable trace log */
#define I2C_TRACE(fmt, ...) LOG_TRACE(fmt, ##__VA_ARGS__)
#else
#define I2C_TRACE(fmt, ...)
#endif

typedef enum {
    I2C_TRANSMITTER,
    I2C_RECEIVER,
    I2C_RECEIVER_RESTART,
} i2c_direction_t;

static inline void i2c_clear_error_flags(I2C_TypeDef* i2c) {
    LL_I2C_ClearFlag_ADDR(i2c);
    LL_I2C_ClearFlag_STOP(i2c);
    LL_I2C_ClearFlag_NACK(i2c);
    LL_I2C_ClearFlag_BERR(i2c);
    LL_I2C_ClearFlag_ARLO(i2c);
    LL_I2C_ClearFlag_OVR(i2c);
}

static inline bool i2c_check_error_conditions(I2C_TypeDef* i2c) {
    if (LL_I2C_IsActiveFlag_NACK(i2c)) {
        LL_I2C_ClearFlag_NACK(i2c);
        return false;
    }
    if (LL_I2C_IsActiveFlag_BERR(i2c) || LL_I2C_IsActiveFlag_ARLO(i2c) ||
        LL_I2C_IsActiveFlag_OVR(i2c)) {
        LL_I2C_ClearFlag_BERR(i2c);
        LL_I2C_ClearFlag_ARLO(i2c);
        LL_I2C_ClearFlag_OVR(i2c);
        return false;
    }
    return true;
}

static bool i2c_start_transfer(I2C_TypeDef* i2c, i2c_direction_t direction,
                               uint8_t addr, uint8_t msg_len, bool reload) {
    uint32_t Request;

    switch (direction) {
        case I2C_TRANSMITTER:
            Request = LL_I2C_GENERATE_START_WRITE;
            break;
        case I2C_RECEIVER:
            Request = LL_I2C_GENERATE_START_READ;
            break;
        case I2C_RECEIVER_RESTART:
            Request = LL_I2C_GENERATE_RESTART_7BIT_READ;
            break;
        default:
            return false;
    }
    if (!LL_I2C_IsEnabledReloadMode(i2c)) {
        LL_I2C_HandleTransfer(i2c, addr, LL_I2C_ADDRSLAVE_7BIT, msg_len,
                              LL_I2C_MODE_SOFTEND, Request);
        if (reload) {
            LL_I2C_EnableReloadMode(i2c);
        } else {
            LL_I2C_DisableReloadMode(i2c);
        }
    } else {
        LL_I2C_SetTransferSize(i2c, msg_len);
    }

    m_time_t start = m_time_ms();

    while (!LL_I2C_IsActiveFlag_TXIS(i2c) && !LL_I2C_IsActiveFlag_RXNE(i2c)) {
        if (!i2c_check_error_conditions(i2c)) {
            return false;
        }
        if (m_time_ms() - start > LL_I2C_CFG_POLL_TIMEOUT_MS) {
            return false;
        }
    }

    return true;
}

static bool i2c_send_byte(I2C_TypeDef* i2c, uint8_t byte) {
    LL_I2C_TransmitData8(i2c, byte);

    m_time_t start = m_time_ms();

    while (!LL_I2C_IsActiveFlag_TXIS(i2c) && !LL_I2C_IsActiveFlag_TC(i2c)) {
        if (LL_I2C_IsEnabledReloadMode(i2c) && LL_I2C_IsActiveFlag_TCR(i2c)) {
            break;
        }
        if (!i2c_check_error_conditions(i2c)) {
            return false;
        }
        if (m_time_ms() - start > LL_I2C_CFG_POLL_TIMEOUT_MS) {
            return false;
        }
    }

    return true;
}

static bool i2c_recv_byte(I2C_TypeDef* i2c, uint8_t* Byte) {
    if (!Byte)
        return false;

    m_time_t start = m_time_ms();

    while (!LL_I2C_IsActiveFlag_RXNE(i2c) && !LL_I2C_IsActiveFlag_TC(i2c)) {
        if (LL_I2C_IsEnabledReloadMode(i2c) && LL_I2C_IsActiveFlag_TCR(i2c)) {
            break;
        }
        if (!i2c_check_error_conditions(i2c)) {
            return false;
        }
        if (m_time_ms() - start > LL_I2C_CFG_POLL_TIMEOUT_MS) {
            return false;
        }
    }

    *Byte = LL_I2C_ReceiveData8(i2c);

    return true;
}

static void i2c_stop_transfer(I2C_TypeDef* i2c, bool reset) {
    /* Send STOP bit */
    LL_I2C_GenerateStopCondition(i2c);
    /* Disable Reload mode */
    if (LL_I2C_IsEnabledReloadMode(i2c)) {
        LL_I2C_DisableReloadMode(i2c);
    }
    if (reset) {
        if (LL_I2C_IsActiveFlag_NACK(i2c)) {
            LL_I2C_ClearFlag_NACK(i2c);
            return;
        }
        i2c_clear_error_flags(i2c);
        LL_I2C_Disable(i2c);
        LL_I2C_Enable(i2c);
    }
}

///////////// PORTING //////////////
#define STM32_I2C_MAX_SIZE 255

void ll_i2c_internal_init(I2C_TypeDef* i2c) {}  // no init needed

bool i2c_start_transfer_retry(I2C_TypeDef* i2c, i2c_direction_t direction,
                              uint8_t addr, uint8_t msg_len, bool reload,
                              uint8_t retry) {
    while (retry--) {
        if (i2c_start_transfer(i2c, direction, addr, msg_len, reload)) {
            return true;
        }
        i2c_stop_transfer(i2c, true);  // reset i2c
    }
    return false;
}

#define I2C_TRANS_RETRY 3

bool ll_i2c_internal_read(I2C_TypeDef* i2c, uint8_t addr, uint16_t reg,
                          uint8_t reg_len, uint8_t* data, uint32_t data_len) {
    uint8_t msg_len;
    i2c_clear_error_flags(i2c);

    if (reg_len) {
        if (!i2c_start_transfer_retry(i2c, I2C_TRANSMITTER, addr, reg_len,
                                      false, I2C_TRANS_RETRY)) {
            return false;
        }
        if (reg_len == 2 && !i2c_send_byte(i2c, reg >> 8)) {
            i2c_stop_transfer(i2c, true);
            return false;
        }
        if (!i2c_send_byte(i2c, reg & 0xFF)) {
            i2c_stop_transfer(i2c, true);
            return false;
        }
    }

    do {
        msg_len = data_len > STM32_I2C_MAX_SIZE ? STM32_I2C_MAX_SIZE : data_len;
        data_len -= msg_len;
        if (!i2c_start_transfer_retry(
                i2c, reg_len ? I2C_RECEIVER_RESTART : I2C_RECEIVER, addr,
                msg_len, data_len > 0, I2C_TRANS_RETRY)) {
            return false;
        }

        while (msg_len--) {
            if (!i2c_recv_byte(i2c, data++)) {
                i2c_stop_transfer(i2c, true);
                return false;
            }
        }
    } while (data_len);

    /* Send STOP after the last byte */
    i2c_stop_transfer(i2c, false);

    return true;
}

bool ll_i2c_internal_write(I2C_TypeDef* i2c, uint8_t addr, uint16_t WriteAddr,
                           uint8_t reg_len, uint8_t* data, uint32_t data_len) {
    uint8_t msg_len;
    i2c_clear_error_flags(i2c);

    if (!i2c_start_transfer_retry(i2c, I2C_TRANSMITTER, addr, reg_len, true,
                                  I2C_TRANS_RETRY)) {
        return false;
    }

    if (reg_len == 2 && !i2c_send_byte(i2c, WriteAddr >> 8)) {
        i2c_stop_transfer(i2c, true);
        return false;
    }
    if (reg_len >= 1 && !i2c_send_byte(i2c, WriteAddr & 0xFF)) {
        i2c_stop_transfer(i2c, true);
        return false;
    }

    do {
        msg_len = data_len > STM32_I2C_MAX_SIZE ? STM32_I2C_MAX_SIZE : data_len;
        data_len -= msg_len;
        if (!i2c_start_transfer_retry(i2c, I2C_TRANSMITTER, addr, msg_len,
                                      data_len > 0, I2C_TRANS_RETRY)) {
            return false;
        }

        while (msg_len--) {
            if (!i2c_send_byte(i2c, *data++)) {
                i2c_stop_transfer(i2c, true);
                return false;
            }
        }
    } while (data_len);

    /* Send STOP after the last byte */
    i2c_stop_transfer(i2c, false);

    return true;
}

bool ll_i2c_internal_transfer(I2C_TypeDef* i2c, uint8_t addr, ll_i2c_msg_t* msg,
                              uint32_t msg_len) {
    uint32_t i;
    uint8_t act_len;
    uint32_t data_len;
    uint8_t* data;
    bool stop;

    i2c_clear_error_flags(i2c);

    for (i = 0; i < msg_len; i++) {
        data = msg[i].data;
        data_len = msg[i].len;
        // 最后一个消息必须发送STOP
        stop = (i == (msg_len - 1)) ? true : msg[i].stop;

        do {
            act_len =
                data_len > STM32_I2C_MAX_SIZE ? STM32_I2C_MAX_SIZE : data_len;
            data_len -= act_len;
            if (!i2c_start_transfer(i2c,
                                    msg[i].wr ? I2C_TRANSMITTER : I2C_RECEIVER,
                                    addr, act_len, data_len > 0)) {
                goto error;
            }
            while (act_len--) {
                if (msg[i].wr) {
                    if (!i2c_send_byte(i2c, *data++)) {
                        goto error;
                    }
                } else {
                    if (!i2c_recv_byte(i2c, data++)) {
                        goto error;
                    }
                }
            }
        } while (data_len);

        if (stop) {
            i2c_stop_transfer(i2c, false);
        }
    }
    return true;

error:
    i2c_stop_transfer(i2c, true);
    return false;
}

bool ll_i2c_internal_check_addr(I2C_TypeDef* i2c, uint8_t addr) {
    uint8_t temp;
    return ll_i2c_internal_read(i2c, addr, 0, 0, &temp, 1);
}

#endif /* !LL_IIC_CFG_USE_IT */

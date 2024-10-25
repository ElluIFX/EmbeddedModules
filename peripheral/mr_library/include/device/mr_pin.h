/*
 * @copyright (c) 2023-2024, MR Development Team
 *
 * @license SPDX-License-Identifier: Apache-2.0
 *
 * @date 2023-11-08    MacRsh       First version
 */

#ifndef _MR_PIN_H_
#define _MR_PIN_H_

#include "mr_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef MR_USING_PIN

/**
 * @addtogroup PIN
 * @{
 */

/**
 * @brief PIN mode.
 */
#define MR_PIN_MODE_NONE (0)       /**< No mode */
#define MR_PIN_MODE_OUTPUT (1)     /**< Output push-pull mode */
#define MR_PIN_MODE_OUTPUT_OD (2)  /**< Output open-drain mode */
#define MR_PIN_MODE_INPUT (3)      /**< Input mode */
#define MR_PIN_MODE_INPUT_DOWN (4) /**< Input pull-down mode */
#define MR_PIN_MODE_INPUT_UP (5)   /**< Input pull-up mode */

/**
 * @brief PIN mode-interrupt.
 */
#define MR_PIN_MODE_IRQ_RISING (6)  /**< Interrupt rising edge mode */
#define MR_PIN_MODE_IRQ_FALLING (7) /**< Interrupt falling edge mode */
#define MR_PIN_MODE_IRQ_EDGE (8)    /**< Interrupt edge mode */
#define MR_PIN_MODE_IRQ_LOW (9)     /**< Interrupt low level mode */
#define MR_PIN_MODE_IRQ_HIGH (10)   /**< Interrupt high level mode */

/**
 * @brief PIN configuration structure.
 */
struct mr_pin_config {
    int mode; /**< Mode */
};

/**
 * @brief PIN control command.
 */
#define MR_IOC_PIN_SET_NUMBER MR_IOC_SPOS /**< Set pin number command */
#define MR_IOC_PIN_SET_MODE MR_IOC_SCFG   /**< Set pin mode command */
#define MR_IOC_PIN_SET_EXTI_CALL \
    MR_IOC_SRCB /**< Set pin exti callback command */

#define MR_IOC_PIN_GET_NUMBER MR_IOC_GPOS /**< Get pin number command */
#define MR_IOC_PIN_GET_MODE MR_IOC_GCFG   /**< Get pin mode command */
#define MR_IOC_PIN_GET_EXTI_CALL \
    MR_IOC_GRCB /**< Get pin exti callback command */

/**
 * @brief PIN data type.
 */
typedef uint8_t mr_pin_data_t; /**< PIN read/write data type */

/**
 * @brief PIN ISR events.
 */
#define MR_ISR_PIN_EXTI_INT (MR_ISR_RD | (0x01)) /**< Exti interrupt event */

/**
 * @brief PIN structure.
 */
struct mr_pin {
    struct mr_dev dev; /**< Device */

    uint32_t pins[32]; /**< Pins */
};

/**
 * @brief PIN operations structure.
 */
struct mr_pin_ops {
    int (*configure)(struct mr_pin* pin, int number, int mode);
    int (*read)(struct mr_pin* pin, int number, uint8_t* value);
    int (*write)(struct mr_pin* pin, int number, uint8_t value);
};

int mr_pin_register(struct mr_pin* pin, const char* path, struct mr_drv* drv);
/** @} */

#endif /* MR_USING_PIN */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MR_PIN_H_ */

/*
    ST doesn't provide a good documentation.
    So use the open examples to make large ST's driver alive on tiny
   microcontollers.

    For some ideas many thanks to https://www.artfulbytes.com/vl53l0x-post
*/

#ifndef __VL53L0X__
#define __VL53L0X__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* Any functions of the lib returns one of these values */
typedef enum { VL53L0X_OK, VL53L0X_FAIL } vl53l0x_ret_t;

typedef enum {
    VL53L0X_GPIO_FUNC_OFF = 0, /* No Interrupt  */
    VL53L0X_GPIO_FUNC_THRESHOLD_CROSSED_LOW =
        1, /* Level Low (value < thresh_low)  */
    VL53L0X_GPIO_FUNC_THRESHOLD_CROSSED_HIGH =
        2, /* Level High (value > thresh_high) */
    VL53L0X_GPIO_FUNC_THRESHOLD_CROSSED_OUT =
        3, /* Out Of Window (value < thresh_low OR value > thresh_high)  */
    VL53L0X_GPIO_FUNC_NEW_MEASURE_READY = 4, /* New Sample Ready  */
} vl53l0x_gpio_func_t;

/* Low Level functions which must be implemented by user */
typedef struct {
    /* Millisecond program delay */
    void (*delay_ms)(uint32_t ms);

    /* I2C communication low level functions */
    void (*i2c_write_reg)(uint8_t addr, uint8_t reg, uint8_t value);
    void (*i2c_write_reg_16bit)(uint8_t addr, uint8_t reg, uint16_t value);
    void (*i2c_write_reg_32bit)(uint8_t addr, uint8_t reg, uint32_t value);
    void (*i2c_write_reg_multi)(uint8_t addr, uint8_t reg, uint8_t* src_buf,
                                size_t count);
    uint8_t (*i2c_read_reg)(uint8_t addr, uint8_t reg);
    uint16_t (*i2c_read_reg_16bit)(uint8_t addr, uint8_t reg);
    uint32_t (*i2c_read_reg_32bit)(uint8_t addr, uint8_t reg);
    void (*i2c_read_reg_multi)(uint8_t addr, uint8_t reg, uint8_t* dst_buf,
                               size_t count);

    /* Control power pin. Don't implement if don't use this pin */
    void (*xshut_set)(void);
    void (*xshut_reset)(void);
} vl53l0x_ll_t;

typedef struct {
    /*
   * Hardware dependent functions.
   * Users have to implement its in their application
   **/
    vl53l0x_ll_t* ll;
    uint8_t addr; /* I2C address of the sensor */

    /* GPIO function. Default is VL53L0X_GPIO_FUNC_NEW_MEASURE_READY */
    vl53l0x_gpio_func_t gpio_func;
} vl53l0x_dev_t;

/*
        Init function. Call it one times during start.
        Default init is: interrupt enabled when
   VL53L0X_GPIO_FUNC_NEW_MEASURE_READY.
*/
vl53l0x_ret_t vl53l0x_init(vl53l0x_dev_t* dev);

/* Power control. Do nothing if xshut_set() and xshut_reset() weren't
implemented. See vl53l0x_ll_t s structure */
void vl53l0x_shutdown(vl53l0x_dev_t* dev);
void vl53l0x_power_up(vl53l0x_dev_t* dev);

void vl53l0x_soft_reset(vl53l0x_dev_t* dev);

/* Enable\disable generation low level on GPIO1 pin on sensor chip */
vl53l0x_ret_t vl53l0x_activate_gpio_interrupt(vl53l0x_dev_t* dev,
                                              vl53l0x_gpio_func_t int_type);
vl53l0x_ret_t vl53l0x_deactivate_gpio_interrupt(
    vl53l0x_dev_t* dev); /* Disable any IRQ type */
vl53l0x_ret_t vl53l0x_clear_flag_gpio_interrupt(
    vl53l0x_dev_t* dev); /* DO NOT call this func in a real ISR */

/* Do one measurement and place it to range variable in mm */
vl53l0x_ret_t vl53l0x_read_in_oneshot_mode(vl53l0x_dev_t* dev, uint16_t* range);

/* Continuous mode */
vl53l0x_ret_t vl53l0x_start_continuous_measurements(vl53l0x_dev_t* dev);
vl53l0x_ret_t vl53l0x_start_single_measurements(vl53l0x_dev_t* dev);
vl53l0x_ret_t vl53l0x_stop_measurement(vl53l0x_dev_t* dev);

/*
 * Returns range in millimeters via 'range' parameter
 * Enable GPIO interrupt and use this to get range after INT will occur.
 * Setup your pin like external interrupt, falling edge mode. (low level is
 *active).
 *
 * DO NOT call this func in a real ISR
 * You will receive INT from the GPIO after each measurement in continuous mode
 *(~40ms)
 **/
vl53l0x_ret_t vl53l0x_get_range_mm(vl53l0x_dev_t* dev, uint16_t* range);

#ifdef __cplusplus
}
#endif

#endif /* __VL53L0X__ */

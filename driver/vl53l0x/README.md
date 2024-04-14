# VL53L0x ranging sensor library

This implementation was born because of these:
![](st-forum-comment.png "")

Read more here:  https://community.st.com/s/question/0D50X00009XkYG8SAN/vl53l0x-register-map


# My features:
 - NOT WIN32 ORIENTED
 - Interrupt setup support
 - Temprature calibration support. It is done inside vl53l0x_init() function.
 - Hardware independent. Just implement these low level functions:
```
typedef struct {
	/* Millisecond program delay */
	void (*delay_ms)(uint32_t ms);

	/* I2C communication low level functions */
	void (*i2c_write_reg)(uint8_t reg, uint8_t value);
	void (*i2c_write_reg_16bit)(uint8_t reg, uint16_t value);
	void (*i2c_write_reg_32bit)(uint8_t reg, uint32_t value);
	void (*i2c_write_reg_multi)(uint8_t reg, uint8_t *src_buf, size_t count);
	uint8_t (*i2c_read_reg)(uint8_t reg);
	uint16_t (*i2c_read_reg_16bit)(uint8_t reg);
	uint32_t (*i2c_read_reg_32bit)(uint8_t reg);
	void (*i2c_read_reg_multi)(uint8_t reg, uint8_t *dst_buf, size_t count);

	/* Control power pin. Don't implement if don't use this pin */
	void (*xshut_set)(void);
	void (*xshut_reset)(void);
} vl53l0x_ll_t;
```
And then use the API.

# Example: single mode, no interrupt
```
int main()
{
	vl53l0x_ll_t vl53l0x_ll;
	vl53l0x_dev_t vl53l0x_dev;
	uint16_t range;

	vl53l0x_ll.delay_ms = vl53l0x_delay; /* You have to implement this function */
	vl53l0x_ll.i2c_write_reg = vl53l0x_i2c_write_reg; /* You have to implement this function */
	vl53l0x_ll.i2c_write_reg_16bit = vl53l0x_i2c_write_reg_16bit; /* You have to implement this function */
	vl53l0x_ll.i2c_write_reg_32bit = vl53l0x_i2c_write_reg_32bit; /* You have to implement this function */
	vl53l0x_ll.i2c_read_reg = vl53l0x_i2c_read_reg; /* You have to implement this function */
	vl53l0x_ll.i2c_read_reg_16bit = vl53l0x_i2c_read_reg_16bit; /* You have to implement this function */
	vl53l0x_ll.i2c_read_reg_32bit = vl53l0x_i2c_read_reg_32bit; /* You have to implement this function */
	vl53l0x_ll.i2c_write_reg_multi = vl53l0x_i2c_write_reg_multi; /* You have to implement this function */
	vl53l0x_ll.i2c_read_reg_multi = vl53l0x_i2c_read_reg_multi; /* You have to implement this function */
	vl53l0x_ll.xshut_reset = vl53l0x_xshut_reset; /* You have to implement this function (optionally) */
	vl53l0x_ll.xshut_set = vl53l0x_xshut_set; /* You have to implement this function (optionally) */

	vl53l0x_dev.ll = &vl53l0x_ll;

	vl53l0x_init(&vl53l0x_dev);
	vl53l0x_deactivate_gpio_interrupt(&vl53l0x_dev); /* Interrupts enabled by default */

	while (1) {
		vl53l0x_read_in_oneshot_mode(&vl53l0x_dev, &range);
		printf("Range = %d [mm]", range);
	}
}
```

# Example: continuous mode, interrupt enabled
```
static int ranging_sensor_int_flag = 0;

int main()
{
	vl53l0x_ll_t vl53l0x_ll;
	vl53l0x_dev_t vl53l0x_dev;
	vl53l0x_range range;

	/* enable gpio IRQ for your CPU */

	vl53l0x_ll.delay_ms = vl53l0x_delay; /* You have to implement this function */
	vl53l0x_ll.i2c_write_reg = vl53l0x_i2c_write_reg; /* You have to implement this function */
	vl53l0x_ll.i2c_write_reg_16bit = vl53l0x_i2c_write_reg_16bit; /* You have to implement this function */
	vl53l0x_ll.i2c_write_reg_32bit = vl53l0x_i2c_write_reg_32bit; /* You have to implement this function */
	vl53l0x_ll.i2c_read_reg = vl53l0x_i2c_read_reg; /* You have to implement this function */
	vl53l0x_ll.i2c_read_reg_16bit = vl53l0x_i2c_read_reg_16bit; /* You have to implement this function */
	vl53l0x_ll.i2c_read_reg_32bit = vl53l0x_i2c_read_reg_32bit; /* You have to implement this function */
	vl53l0x_ll.i2c_write_reg_multi = vl53l0x_i2c_write_reg_multi; /* You have to implement this function */
	vl53l0x_ll.i2c_read_reg_multi = vl53l0x_i2c_read_reg_multi; /* You have to implement this function */
	vl53l0x_ll.xshut_reset = vl53l0x_xshut_reset; /* You have to implement this function (optionally) */
	vl53l0x_ll.xshut_set = vl53l0x_xshut_set; /* You have to implement this function (optionally) */

	vl53l0x_dev.ll = &vl53l0x_ll;

	vl53l0x_init(&vl53l0x_dev);
	vl53l0x_start_continuous_measurements(&vl53l0x_dev);

	while (1) {
		if (ranging_sensor_int_flag) {
			vl53l0x_get_range_mm_continuous(&vl53l0x_dev, &range);
			vl53l0x_clear_flag_gpio_interrupt(&vl53l0x_dev);
			printf("Range = %d [mm]", range.range_mm);
			ranging_sensor_int_flag = 0;
		}
	}
}

void gpio_interrupt_handler_callback(void)
{
	ranging_sensor_int_flag = 1;
}
```
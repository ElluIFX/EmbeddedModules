#ifndef _SW_IIC_H_
#define _SW_IIC_H_
#ifdef __cplusplus
extern "C" {
#endif

/* includes */
#include "modules.h"

typedef struct {
    GPIO_TypeDef* sclPort;
    uint16_t sclPin;
    GPIO_TypeDef* sdaPort;
    uint16_t sdaPin;
    uint8_t waitTime;
    uint8_t waitTimeLong;
} sw_i2c_t;

extern void sw_i2c_init(sw_i2c_t* dev);
extern void sw_i2c_write_byte(sw_i2c_t* dev, uint8_t data);
extern uint8_t sw_i2c_read_byte(sw_i2c_t* dev);
extern uint8_t sw_i2c_read(sw_i2c_t* dev, uint8_t addr, uint8_t reg,
                           uint8_t* pdata, uint8_t rcnt);
extern uint8_t sw_i2c_read_16addr(sw_i2c_t* dev, uint8_t addr, uint16_t reg,
                                  uint8_t* pdata, uint8_t rcnt);
extern uint8_t sw_i2c_write(sw_i2c_t* dev, uint8_t addr, uint8_t reg,
                            uint8_t* pdata, uint8_t rcnt);
extern uint8_t sw_i2c_write_16addr(sw_i2c_t* dev, uint8_t addr, uint16_t reg,
                                   uint8_t* pdata, uint8_t rcnt);
extern uint8_t sw_i2c_check_addr(sw_i2c_t* dev, uint8_t addr);
extern void sw_i2c_bus_scan(sw_i2c_t* dev, uint8_t* addr_list,
                            uint8_t* addr_cnt);
#ifdef __cplusplus
}
#endif
#endif /* __I2C_SW_H */

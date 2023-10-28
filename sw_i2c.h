#ifndef _SW_IIC_H_
#define _SW_IIC_H_

/* includes */
#include "modules.h"

typedef struct {
  GPIO_TypeDef* sclPort;
  uint16_t sclPin;
  GPIO_TypeDef* sdaPort;
  uint16_t sdaPin;
} sw_iic_t;

#define SW_IIC_WAIT_TIME 15       // default 25us
#define SW_IIC_WAIT_TIME_LONG 25  // default 25us

/* functions */
extern void SW_IIC_Init(sw_iic_t* dev);
extern void SW_IIC_Write_Data(sw_iic_t* dev, uint8_t data);
extern uint8_t SW_IIC_Read_Data(sw_iic_t* dev);
extern uint8_t SW_IIC_Read_8addr(sw_iic_t* dev, uint8_t SlaveAddr,
                                 uint8_t RegAddr, uint8_t* pdata, uint8_t rcnt);
extern uint8_t SW_IIC_Read_16addr(sw_iic_t* dev, uint8_t SlaveAddr,
                                  uint16_t RegAddr, uint8_t* pdata,
                                  uint8_t rcnt);
extern uint8_t SW_IIC_Write_8addr(sw_iic_t* dev, uint8_t SlaveAddr,
                                  uint8_t RegAddr, uint8_t* pdata,
                                  uint8_t rcnt);
extern uint8_t SW_IIC_Write_16addr(sw_iic_t* dev, uint8_t SlaveAddr,
                                   uint16_t RegAddr, uint8_t* pdata,
                                   uint8_t rcnt);
extern uint8_t SW_IIC_Check_SlaveAddr(sw_iic_t* dev, uint8_t SlaveAddr);
#endif /* __I2C_SW_H */

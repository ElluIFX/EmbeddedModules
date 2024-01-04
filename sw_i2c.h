#ifndef _SW_IIC_H_
#define _SW_IIC_H_
#ifdef __cplusplus
extern "C" {
#endif

/* includes */
#include "modules.h"

typedef struct {
    GPIO_TypeDef *sclPort;
    uint16_t sclPin;
    GPIO_TypeDef *sdaPort;
    uint16_t sdaPin;
    uint8_t waitTime;
    uint8_t waitTimeLong;
} sw_iic_t;

extern void SW_IIC_Init(sw_iic_t *dev);
extern void SW_IIC_WriteData(sw_iic_t *dev, uint8_t data);
extern uint8_t SW_IIC_ReadData(sw_iic_t *dev);
extern uint8_t SW_IIC_Read8addr(sw_iic_t *dev, uint8_t SlaveAddr,
                                uint8_t RegAddr, uint8_t *pdata, uint8_t rcnt);
extern uint8_t SW_IIC_Read16addr(sw_iic_t *dev, uint8_t SlaveAddr,
                                 uint16_t RegAddr, uint8_t *pdata,
                                 uint8_t rcnt);
extern uint8_t SW_IIC_Write8addr(sw_iic_t *dev, uint8_t SlaveAddr,
                                 uint8_t RegAddr, uint8_t *pdata,
                                 uint8_t rcnt);
extern uint8_t SW_IIC_Write16addr(sw_iic_t *dev, uint8_t SlaveAddr,
                                  uint16_t RegAddr, uint8_t *pdata,
                                  uint8_t rcnt);
extern uint8_t SW_IIC_CheckSlaveAddr(sw_iic_t *dev, uint8_t SlaveAddr);
extern void SW_IIC_BusScan(sw_iic_t *dev, uint8_t *addr_list,
                           uint8_t *addr_cnt);
#ifdef __cplusplus
}
#endif
#endif /* __I2C_SW_H */

/**
 * @file ll_i2c.h
 * @brief LL I2C interface definition
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-10-15
 *
 * THINK DIFFERENTLY
 */

#ifndef __LL_IIC_H__
#define __LL_IIC_H__

#include "modules.h"

#define _LL_IIC_USE_IT 0             // 使用中断(freeRTOS)
#define _LL_IIC_INSTANCE I2C2        // 接管 I2C 实例
#define _LL_IIC_CONVERT_SLAVEADDR 0  // 转换从机地址(自动<<1)

#define _LL_IIC_SPEED_FREQ 400000  // I2C 频率
#define _LL_IIC_BYTE_TIMEOUT_US ((10 ^ 6) / (_LL_IIC_SPEED_FREQ / 9) + 1)
#define _LL_IIC_BYTE_TIMEOUT_MS (_LL_IIC_BYTE_TIMEOUT_US / 1000 + 1)

extern void LL_IIC_Init(void);
extern void LL_IIC_Write_Data(uint8_t data);
extern uint8_t LL_IIC_Read_Data(void);
extern uint8_t LL_IIC_Read_8addr(uint8_t SlaveAddr, uint8_t RegAddr,
                                 uint8_t *pdata, uint8_t rcnt);
extern uint8_t LL_IIC_Read_16addr(uint8_t SlaveAddr, uint16_t RegAddr,
                                  uint8_t *pdata, uint8_t rcnt);
extern uint8_t LL_IIC_Write_8addr(uint8_t SlaveAddr, uint8_t RegAddr,
                                  uint8_t *pdata, uint8_t rcnt);
extern uint8_t LL_IIC_Write_16addr(uint8_t SlaveAddr, uint16_t RegAddr,
                                   uint8_t *pdata, uint8_t rcnt);
extern uint8_t LL_IIC_Check_SlaveAddr(uint8_t SlaveAddr);

#if _LL_IIC_USE_IT  // 放到 stm32xxx_it.c 里对应的中断函数中
extern void LL_IIC_IRQHandler(void);
extern void LL_IIC_EV_IRQHandler(void);
extern void LL_IIC_ER_IRQHandler(void);
#endif /* _LL_IIC_USE_IT */
#endif /* __LL_IIC_H__ */

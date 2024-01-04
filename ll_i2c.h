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

#ifdef __cplusplus
extern "C" {
#endif
#include "modules.h"

#define _LL_IIC_CONVERT_SLAVEADDR 0 // 转换从机地址(自动<<1)

#define _LL_IIC_SPEED_FREQ      400000 // I2C 频率
#define _LL_IIC_BYTE_TIMEOUT_US ((10 ^ 6) / (_LL_IIC_SPEED_FREQ / 9) + 1)
#define _LL_IIC_BYTE_TIMEOUT_MS (_LL_IIC_BYTE_TIMEOUT_US / 1000 + 1)

/**
 * @brief I2C 直写数据
 * @param  I2Cx   I2C 句柄
 * @param  data   数据
 */
extern bool LL_IIC_WriteData(I2C_TypeDef *I2Cx, uint8_t SlaveAddr,
                             uint8_t *pdata, uint8_t rcnt);

/**
 * @brief I2C 直读数据
 * @param  I2Cx   I2C 句柄
 * @return uint8_t 读取的数据
 */
extern bool LL_IIC_ReadData(I2C_TypeDef *I2Cx, uint8_t SlaveAddr,
                            uint8_t *pdata, uint8_t rcnt);

/**
 * @brief I2C 读取数据到缓冲区, 8位地址
 * @param  I2Cx   I2C 句柄
 * @param  SlaveAddr 从机地址
 * @param  RegAddr   寄存器地址
 * @param  pdata     数据指针
 * @param  rcnt      数据长度
 * @return bool      是否成功
 */
extern bool LL_IIC_Read8addr(I2C_TypeDef *I2Cx, uint8_t SlaveAddr,
                             uint8_t RegAddr, uint8_t *pdata, uint8_t rcnt);

/**
 * @brief I2C 读取数据到缓冲区, 16位地址
 * @param  I2Cx   I2C 句柄
 * @param  SlaveAddr 从机地址
 * @param  RegAddr   寄存器地址
 * @param  pdata     数据指针
 * @param  rcnt      数据长度
 * @retval bool      是否成功
 */
extern bool LL_IIC_Read16addr(I2C_TypeDef *I2Cx, uint8_t SlaveAddr,
                              uint16_t RegAddr, uint8_t *pdata, uint8_t rcnt);

/**
 * @brief I2C 写入缓冲区数据, 8位地址
 * @param  I2Cx   I2C 句柄
 * @param  SlaveAddr 从机地址
 * @param  RegAddr   寄存器地址
 * @param  pdata     数据指针
 * @param  rcnt      数据长度
 * @return bool      是否成功
 */
extern bool LL_IIC_Write8addr(I2C_TypeDef *I2Cx, uint8_t SlaveAddr,
                              uint8_t RegAddr, uint8_t *pdata, uint8_t rcnt);

/**
 * @brief I2C 写入缓冲区数据, 16位地址
 * @param  I2Cx   I2C 句柄
 * @param  SlaveAddr 从机地址
 * @param  RegAddr   寄存器地址
 * @param  pdata     数据指针
 * @param  rcnt      数据长度
 * @return bool      是否成功
 */
extern bool LL_IIC_Write16addr(I2C_TypeDef *I2Cx, uint8_t SlaveAddr,
                               uint16_t RegAddr, uint8_t *pdata, uint8_t rcnt);

/**
 * @brief I2C 检查从机是否存在
 * @param  I2Cx  I2C 句柄
 * @param  SlaveAddr 从机地址
 * @retval bool      是否存在
 */
extern bool LL_IIC_CheckSlaveAddr(I2C_TypeDef *I2Cx, uint8_t SlaveAddr);

/**
 * @brief I2C 总线扫描
 * @param  I2Cx  I2C 句柄
 * @param  addr_list 从机地址列表指针(NULL则不保存)
 * @param  addr_cnt  从机地址数量指针(NULL则不保存)
 */
extern void LL_IIC_BusScan(I2C_TypeDef *I2Cx, uint8_t *addr_list,
                           uint8_t *addr_cnt);
#ifdef __cplusplus
}
#endif
#endif /* __LL_IIC_H__ */

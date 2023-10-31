#ifndef I2C_SLAVE_H_
#define I2C_SLAVE_H_
#include "modules.h"
#define _SLAVE_I2C_INSTANCE I2C1

/**
 * @brief I2C从机初始化
 * @param  addr 从机地址
 */
extern void Slave_I2C_Init(const uint8_t addr);

/**
 * @brief I2C从机LL事件中断回调函数
 * @note 在I2Cx_EV_IRQHandler中调用(stm32xxxx_it.c)
 */
extern void Slave_I2C_IRQHandler(void);

/**
 * @brief I2C从机LL错误中断回调函数
 * @note 在I2Cx_ER_IRQHandler中调用(stm32xxxx_it.c)
 */
extern void Slave_I2C_Error_IRQHandler(void);

/**
 * @brief I2C从机接收完成回调函数(用户自定义)
 * @param  offset     主站设定的寄存器偏移地址
 * @param  data       寄存器缓冲区指针
 * @param  len        接收数据长度
 * @note  用户应从mem[offset]开始读取数据
 */
extern void Slave_I2C_RecvDone_Callback(uint16_t offset, uint8_t* data,
                                        uint16_t len);

/**
 * @brief I2C从机发送准备回调函数(用户自定义)
 * @param  offset     主站请求的寄存器偏移地址
 * @param  mem        寄存器缓冲区指针
 * @note  从机将从mem[offset]开始上传数据
 */
extern void Slave_I2C_PreSend_Callback(uint16_t offset, uint8_t* mem);

/**
 * @brief I2C从机发送完成回调函数(用户自定义)
 * @param  offset     主站请求的寄存器偏移地址
 * @param  data       寄存器缓冲区指针
 * @param  len        发送数据长度
 */
extern void Slave_I2C_SendDone_Callback(uint16_t offset, uint8_t* data,
                                        uint16_t len);

/**
 * @brief 获取从机寄存器内存指针
 * @retval 从机寄存器内存指针
 */
extern uint8_t* Slave_I2C_GetMem(void);
#endif

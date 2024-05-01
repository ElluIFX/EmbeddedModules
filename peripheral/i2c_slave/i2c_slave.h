#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

#include <stdbool.h>

/************* 用户函数 **************/

/**
 * @brief I2C从机硬件初始化
 * @param  addr 从机地址
 */
extern void slave_i2c_init(I2C_TypeDef* I2Cx);

/**
 * @brief I2C从机设置从机地址
 * @param  addr 从机地址
 */
extern void slave_i2c_set_address(I2C_TypeDef* I2Cx, uint8_t addr);

/**
 * @brief I2C从机设置从机使能
 * @param  state 1:使能 0:关闭
 */
extern void slave_i2c_set_enable(I2C_TypeDef* I2Cx, bool state);

/**
 * @brief I2C从机设置中断状态
 * @param  state 1:使能中断 0:关闭中断
 */
extern void slave_i2c_set_interrupt(I2C_TypeDef* I2Cx, bool state);

/************* 中断回调函数 *************/
// note: 对于部分不区分EV/ER的MCU，可以只调用slave_i2c_event_irq

/**
 * @brief I2C从机LL事件中断回调函数
 * @note 在I2Cx_EV_IRQHandler中调用(stm32xxxx_it.c)
 */
extern void slave_i2c_event_irq(I2C_TypeDef* I2Cx);

/**
 * @brief I2C从机LL错误中断回调函数
 * @note 在I2Cx_ER_IRQHandler中调用(stm32xxxx_it.c)
 */
extern void slave_i2c_error_irq(I2C_TypeDef* I2Cx);

/************* 数据处理回调函数 *************/
// note: 以下回调函数由用户实现，组成完整的I2C从机传输逻辑

/**
 * @brief I2C从机传输开始回调函数(由用户实现)
 * @note 对应主机发送START信号
 */
extern void slave_i2c_transmit_begin_handler(I2C_TypeDef* I2Cx);

/**
 * @brief I2C从机传输结束回调函数(由用户实现)
 * @note 对应主机发送STOP信号(重启数据传输时不会触发)
 */
extern void slave_i2c_transmit_end_handler(I2C_TypeDef* I2Cx);

/**
 * @brief I2C从机接收数据回调函数(由用户实现)
 * @param[in]  data 刚刚接收到的数据
 * @retval     true:允许继续接收 false:停止接收(发送NACK)
 */
extern bool slave_i2c_transmit_in_handler(I2C_TypeDef* I2Cx, uint8_t in_data);

/**
 * @brief I2C从机准备发送数据回调函数(由用户实现)
 * @param[out]  data 即将发送的数据
 * @retval      true:允许继续发送 false:停止发送(发送NACK, 本次数据不会发送)
 */
extern bool slave_i2c_transmit_out_handler(I2C_TypeDef* I2Cx,
                                           uint8_t* out_data);

#ifdef __cplusplus
}
#endif
#endif /* __I2C_SLAVE_H__ */

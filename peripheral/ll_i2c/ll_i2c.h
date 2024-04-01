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

#if !KCONFIG_AVAILABLE
#define LL_IIC_CFG_CONVERT_7BIT_ADDR 0  // 转换从机地址(自动<<1)
#define LL_IIC_CFG_USE_IT 0             // 使用中断
#define LL_I2C_CFG_SEM_TIMEOUT_MS 1000  // 信号量超时时间
#define LL_I2C_CFG_POLL_TIMEOUT_MS 5    // 轮询响应超时时间

#endif

typedef struct {
  uint8_t *data;     // 数据指针
  uint32_t len;      // 数据长度
  uint8_t wr : 1;    // 写入/读取
  uint8_t stop : 1;  // 是否产生STOP
} ll_i2c_msg_t;

/**
 * @brief I2C 初始化
 * @param  i2c       I2Cx 句柄
 */
extern void ll_i2c_init(I2C_TypeDef *i2c);

/**
 * @brief I2C 多消息事务
 * @param  i2c       I2Cx 句柄
 * @param  addr      从机地址
 * @param  msg       消息数组指针
 * @param  msg_len   消息数组个数
 * @retval bool      是否成功
 */
extern bool ll_i2c_transfer(I2C_TypeDef *i2c, uint8_t addr, ll_i2c_msg_t *msg,
                            uint32_t msg_len);

/**
 * @brief I2C 直写数据
 * @param  i2c        I2Cx 句柄
 * @param  addr       从机地址
 * @param  data       数据指针
 * @param  data_len   数据长度
 * @return bool       是否成功
 */
extern bool ll_i2c_write_raw(I2C_TypeDef *i2c, uint8_t addr, uint8_t *data,
                             uint32_t data_len);

/**
 * @brief I2C 直读数据
 * @param  i2c        I2Cx 句柄
 * @param  addr       从机地址
 * @param  data       数据指针
 * @param  data_len   数据长度
 * @return bool       是否成功
 */
extern bool ll_i2c_read_raw(I2C_TypeDef *i2c, uint8_t addr, uint8_t *data,
                            uint32_t data_len);

/**
 * @brief I2C 读取数据到缓冲区, 8位地址
 * @param  i2c       I2Cx 句柄
 * @param  addr      从机地址
 * @param  reg       寄存器地址
 * @param  data      数据指针
 * @param  data_len  数据长度
 * @return bool      是否成功
 */
extern bool ll_i2c_read(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg,
                        uint8_t *data, uint32_t data_len);

/**
 * @brief I2C 读取数据到缓冲区, 16位地址
 * @param  i2c       I2Cx 句柄
 * @param  addr      从机地址
 * @param  reg       寄存器地址
 * @param  data      数据指针
 * @param  data_len  数据长度
 * @retval bool      是否成功
 */
extern bool ll_i2c_read_16addr(I2C_TypeDef *i2c, uint8_t addr, uint16_t reg,
                               uint8_t *data, uint32_t data_len);

/**
 * @brief I2C 写入缓冲区数据, 8位地址
 * @param  i2c       I2Cx 句柄
 * @param  addr      从机地址
 * @param  reg       寄存器地址
 * @param  data      数据指针
 * @param  data_len  数据长度
 * @return bool      是否成功
 */
extern bool ll_i2c_write(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg,
                         uint8_t *data, uint32_t data_len);

/**
 * @brief I2C 写入缓冲区数据, 16位地址
 * @param  i2c       I2Cx 句柄
 * @param  addr      从机地址
 * @param  reg       寄存器地址
 * @param  data      数据指针
 * @param  data_len  数据长度
 * @return bool      是否成功
 */
extern bool ll_i2c_write_16addr(I2C_TypeDef *i2c, uint8_t addr, uint16_t reg,
                                uint8_t *data, uint32_t data_len);

/**
 * @brief I2C 检查从机是否存在
 * @param  i2c       I2Cx 句柄
 * @param  addr      从机地址
 * @retval bool      是否存在
 */
extern bool ll_i2c_check_addr(I2C_TypeDef *i2c, uint8_t addr);

/**
 * @brief I2C 总线扫描
 * @param  i2c       I2Cx 句柄
 * @param  addr_list 从机地址列表指针(NULL则不保存)
 * @param  addr_cnt  从机地址数量指针(NULL则不保存)
 */
extern void ll_i2c_bus_scan(I2C_TypeDef *i2c, uint8_t *addr_list,
                            uint8_t *addr_cnt);
/**
 * @brief I2C 打印寄存器
 *
 * @param  i2c      I2Cx 句柄
 * @param  addr     从机地址
 * @param  start    起始寄存器地址
 * @param  stop     结束寄存器地址
 */
extern void ll_i2c_dump(I2C_TypeDef *i2c, uint8_t addr, uint8_t start,
                        uint8_t stop);

/**
 * @brief I2C 事件中断服务函数
 * @param  i2c 中断源      I2Cx 句柄
 */
extern void ll_i2c_event_irq(I2C_TypeDef *i2c);

/**
 * @brief I2C 错误中断服务函数
 * @param  i2c 中断源      I2Cx 句柄
 */
extern int ll_i2c_error_irq(I2C_TypeDef *i2c);

/**
 * @brief I2C 组合中断服务函数(ev+er)
 * @param  i2c 中断源      I2Cx 句柄
 */
extern void ll_i2c_combine_irq(I2C_TypeDef *i2c);

#ifdef __cplusplus
}
#endif
#endif /* __LL_IIC_H__ */

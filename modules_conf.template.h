/**
 * @file modules_conf.h (modules_conf.template.h)
 * @brief 配置Modules文件夹下所有模块的设置
 * @note  将此文件复制一份并重命名为modules_conf.h
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-04-29
 *
 * THINK DIFFERENTLY
 */

#ifndef __MODULES_CONF_H
#define __MODULES_CONF_H
/*********************************************************************/
/****************************** 全局设置 ******************************/
// 动态内存分配方法(m_alloc/m_free/m_realloc):
#define _MOD_HEAP_MATHOD 0  // 0:stdlib 1:lwmem 2:klite 3:freertos
// 时间获取方法(m_tick/m_time_*)
#define _MOD_TIME_MATHOD 1  // 0:HAL 1:perf_counter
// 延时方法(m_delay_*)
#define _MOD_DELAY_MATHOD 1  // 0:HAL 1:perf_counter 2:klite 3:freertos
// 是否使用操作系统(MOD_MUTEX_*)
#define _MOD_USE_OS 0  // 0:none 1:klite 2:freertos

/******************************调度器设置******************************/
#define _SCH_ENABLE_TASK 1       // 支持任务
#define _SCH_ENABLE_EVENT 1      // 支持事件
#define _SCH_ENABLE_COROUTINE 1  // 支持宏协程
#define _SCH_ENABLE_CALLLATER 1  // 支持延时调用
#define _SCH_ENABLE_SOFTINT 1    // 支持软中断

#define _SCH_COMP_RANGE_US 1000  // 任务调度自动补偿范围(us)

#define _SCH_DEBUG_REPORT 1  // 输出调度器统计信息(调试模式/低性能)
#define _SCH_DEBUG_PERIOD 5  // 调试报告打印周期(s)(超过10s的值可能导致溢出)
#define _SCH_DEBUG_MAXLINE 5  // 调试报告最大行数

#define _SCH_ENABLE_TERMINAL 1  // 是否启用终端命令集(依赖embedded-cli)

/****************************** 日志设置 ******************************/
// 调试日志设置
#define _LOG_ENABLE 1            // 调试日志总开关
#define _LOG_ENABLE_TIMESTAMP 1  // 调试日志是否添加时间戳
#define _LOG_ENABLE_COLOR 1      // 调试日志是否按等级添加颜色
#define _LOG_ENABLE_FUNC_LINE 0  // 调试日志是否添加函数名和行号
#define _LOG_ENABLE_ASSERT 1     // 是否开启ASSERT
// 调试日志等级
#define _LOG_ENABLE_DEBUG 1  // 是否输出DEBUG日志
#define _LOG_ENABLE_PASS 1   // 是否输出PASS日志
#define _LOG_ENABLE_INFO 1   // 是否输出INFO日志
#define _LOG_ENABLE_WARN 1   // 是否输出WARN日志
#define _LOG_ENABLE_ERROR 1  // 是否输出ERROR日志
#define _LOG_ENABLE_FATAL 1  // 是否输出FATAL日志

/****************************** 串口设置 ******************************/
// 组件设置
#define _UART_ENABLE_DMA_RX 1      // 是否开启串口DMA接收功能
#define _UART_ENABLE_FIFO_TX 1     // 是否开启串口FIFO发送功能
#define _UART_DCACHE_COMPATIBLE 0  // (H7/F7) DCache兼容模式
#define _UART_REWRITE_HANLDER 1  // 是否重写HAL库中的串口中断处理函数
// USB CDC设置
#define _UART_ENABLE_CDC 0      // 是否开启CDC虚拟串口支持
#define _UART_CDC_USE_CUBEMX 1  // 是否使用CUBEMX生成的CDC代码
#define _UART_CDC_USE_CHERRY 0  // 是否使用CherryUSB的CDC代码
// 串口收发设置
#define _UART_TX_USE_DMA 1  // 对于支持DMA的串口是否使用DMA发送
#define _UART_TX_USE_IT 1   // 对不支持DMA的串口是否使用中断发送
#define _UART_TX_TIMEOUT 5  // 串口发送等待超时时间(ms)/0阻塞/<0放弃发送
#define _UART_CDC_TIMEOUT 5  // CDC发送等待超时时间(ms)/<=0放弃发送
#define _UART_FIFO_TIMEOUT 5  // FIFO发送等待超时时间(ms)/0阻塞/<0放弃发送
// printf重定向设置
#define _PRINTF_BLOCK 0           // 是否屏蔽所有printf
#define _PRINTF_REDIRECT 1        // 是否重定向printf
#define _PRINTF_REDIRECT_PUTX 1   // 是否重定向putchar/puts
#define _PRINTF_UART_PORT huart1  // printf重定向到串口
#define _PRINTF_USE_RTT 0         // printf重定向到RTT
#define _PRINTF_USE_CDC 0         // printf重定向到CDC
#define _PRINTF_USE_ITM 0         // printf重定向到ITM
// VOFA+
#define _VOFA_ENABLE 1        // 是否开启VOFA相关函数
#define _VOFA_BUFFER_SIZE 32  // VOFA缓冲区大小

/****************************** LED设置 ******************************/
#define _LED_USE_PWM 0                // 是否使用PWM控制RGB灯
#define _LED_R_HTIM htim8             // 红灯PWM定时器
#define _LED_G_HTIM htim8             // 绿灯PWM定时器
#define _LED_B_HTIM htim8             // 蓝灯PWM定时器
#define _LED_R_CHANNEL TIM_CHANNEL_1  // 红灯PWM通道
#define _LED_G_CHANNEL TIM_CHANNEL_2  // 绿灯PWM通道
#define _LED_B_CHANNEL TIM_CHANNEL_3  // 蓝灯PWM通道
#define _LED_R_PULSE 400              // 红灯最大比较值
#define _LED_G_PULSE 650              // 绿灯最大比较值
#define _LED_B_PULSE 800              // 蓝灯最大比较值
#define _LED_PWMN_OUTPUT 1            // 互补输出

/****************************** IIC设置 ******************************/
#define _BOARD_I2C_USE_SW_IIC 0       // 是否使用软件IIC
#define _BOARD_I2C_USE_LL_I2C 0       // 是否使用LL库IIC
#define _BOARD_I2C_USE_HAL_I2C 0      // 是否使用HAL库IIC
#define _BOARD_I2C_SW_SCL_PORT GPIOB  // 软件IIC SCL引脚
#define _BOARD_I2C_SW_SCL_PIN GPIO_PIN_6
#define _BOARD_I2C_SW_SDA_PORT GPIOB  // 软件IIC SDA引脚
#define _BOARD_I2C_SW_SDA_PIN GPIO_PIN_7
#define _BOARD_I2C_LL_INSTANCE I2C1    // LL库IIC实例
#define _BOARD_I2C_HAL_INSTANCE hi2c1  // HAL库IIC实例

/*********************************************************************/
/**************************** 设置结束 *******************************/
#endif  // __MODULES_CONF_H

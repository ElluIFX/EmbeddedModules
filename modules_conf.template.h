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
// > 0:stdlib 1:lwmem 2:klite 3:freertos 4:heap4 5:rtthread
#define MOD_CFG_HEAP_MATHOD 0

// 时间获取方法(m_tick/m_time_*)
// > 0:HAL 1:perf_counter
#define MOD_CFG_TIME_MATHOD 1

// 延时方法(m_delay_*)
// > 0:HAL 1:perf_counter 2:klite 3:freertos 4:rtthread
#define MOD_CFG_DELAY_MATHOD 1

// 是否使用操作系统(MOD_MUTEX_*)
// > 0:none 1:klite 2:freertos 3:rtthread
#define MOD_CFG_USE_OS 0

/******************************调度器设置******************************/
#define SCH_CFG_ENABLE_TASK 1       // 支持任务
#define SCH_CFG_ENABLE_EVENT 1      // 支持事件
#define SCH_CFG_ENABLE_COROUTINE 1  // 支持宏协程
#define SCH_CFG_ENABLE_CALLLATER 1  // 支持延时调用
#define SCH_CFG_ENABLE_SOFTINT 1    // 支持软中断

#define SCH_CFG_COMP_RANGE_US 1000  // 任务调度自动补偿范围(us)
#define SCH_CFG_STATIC_NAME 1       // 是否使用静态标识名
#define SCH_CFG_STATIC_NAME_LEN 16  // 静态标识名长度

#define SCH_CFG_DEBUG_REPORT 1  // 输出调度器统计信息(调试模式/低性能)
#define SCH_CFG_DEBUG_PERIOD 5  // 调试报告打印周期(s)(超过10s的值可能导致溢出)
#define SCH_CFG_DEBUG_MAXLINE 10  // 调试报告最大行数

#define SCH_CFG_ENABLE_TERMINAL 1  // 是否启用终端命令集(依赖embedded-cli)

/****************************** 日志设置 ******************************/
// 调试日志设置
#define LOG_CFG_ENABLE 1            // 调试日志总开关
#define LOG_CFG_ENABLE_TIMESTAMP 1  // 调试日志是否添加时间戳
#define LOG_CFG_ENABLE_COLOR 1      // 调试日志是否按等级添加颜色
#define LOG_CFG_ENABLE_FUNC_LINE 0  // 调试日志是否添加函数名和行号
#define LOG_CFG_ENABLE_ASSERT 1     // 是否开启ASSERT
// 调试日志等级
#define LOG_CFG_ENABLE_DEBUG 1  // 是否输出DEBUG日志
#define LOG_CFG_ENABLE_PASS 1   // 是否输出PASS日志
#define LOG_CFG_ENABLE_INFO 1   // 是否输出INFO日志
#define LOG_CFG_ENABLE_WARN 1   // 是否输出WARN日志
#define LOG_CFG_ENABLE_ERROR 1  // 是否输出ERROR日志
#define LOG_CFG_ENABLE_FATAL 1  // 是否输出FATAL日志

/****************************** 串口设置 ******************************/
// 组件设置
#define UART_CFG_ENABLE_DMA_RX 1      // 是否开启串口DMA接收功能
#define UART_CFG_ENABLE_FIFO_TX 1     // 是否开启串口FIFO发送功能
#define UART_CFG_DCACHE_COMPATIBLE 0  // (H7/F7) DCache兼容模式
#define UART_CFG_REWRITE_HANLDER 1  // 是否重写HAL库中的串口中断处理函数
// USB CDC设置
#define UART_CFG_ENABLE_CDC 0      // 是否开启CDC虚拟串口支持
#define UART_CFG_CDC_USE_CUBEMX 1  // 是否使用CUBEMX生成的CDC代码
#define UART_CFG_CDC_USE_CHERRY 0  // 是否使用CherryUSB的CDC代码
// 串口收发设置
#define UART_CFG_TX_USE_DMA 1  // 对于支持DMA的串口是否使用DMA发送
#define UART_CFG_TX_USE_IT 1  // 对不支持DMA的串口是否使用中断发送
#define UART_CFG_TX_TIMEOUT 5  // 串口发送等待超时时间(ms)/0阻塞/<0放弃发送
#define UART_CFG_CDC_TIMEOUT 5  // CDC发送等待超时时间(ms)/<=0放弃发送
#define UART_CFG_FIFO_TIMEOUT 5  // FIFO发送等待超时时间(ms)/0阻塞/<0放弃发送
// printf重定向设置
#define UART_CFG_PRINTF_BLOCK 0           // 是否屏蔽所有printf
#define UART_CFG_PRINTF_REDIRECT 1        // 是否重定向printf
#define UART_CFG_PRINTF_REDIRECT_PUTX 1   // 是否重定向putchar/puts
#define UART_CFG_PRINTF_UART_PORT huart1  // printf重定向到串口
#define UART_CFG_PRINTF_USE_RTT 0         // printf重定向到RTT
#define UART_CFG_PRINTF_USE_CDC 0         // printf重定向到CDC
#define UART_CFG_PRINTF_USE_ITM 0         // printf重定向到ITM
// VOFA+
#define VOFA_CFG_ENABLE 1        // 是否开启VOFA相关函数
#define VOFA_CFG_BUFFER_SIZE 32  // VOFA缓冲区大小

/****************************** LED设置 ******************************/
#define LED_CFG_USE_PWM 0                // 是否使用PWM控制RGB灯
#define LED_CFG_R_HTIM htim8             // 红灯PWM定时器
#define LED_CFG_G_HTIM htim8             // 绿灯PWM定时器
#define LED_CFG_B_HTIM htim8             // 蓝灯PWM定时器
#define LED_CFG_R_CHANNEL TIM_CHANNEL_1  // 红灯PWM通道
#define LED_CFG_G_CHANNEL TIM_CHANNEL_2  // 绿灯PWM通道
#define LED_CFG_B_CHANNEL TIM_CHANNEL_3  // 蓝灯PWM通道
#define LED_CFG_R_PULSE 400              // 红灯最大比较值
#define LED_CFG_G_PULSE 650              // 绿灯最大比较值
#define LED_CFG_B_PULSE 800              // 蓝灯最大比较值
#define LED_CFG_PWMN_OUTPUT 1            // 互补输出

/****************************** IIC设置 ******************************/
#define BOARD_I2C_CFG_USE_SW_IIC 0       // 是否使用软件IIC
#define BOARD_I2C_CFG_USE_LL_I2C 0       // 是否使用LL库IIC
#define BOARD_I2C_CFG_USE_HAL_I2C 0      // 是否使用HAL库IIC
#define BOARD_I2C_CFG_SW_SCL_PORT GPIOB  // 软件IIC SCL引脚
#define BOARD_I2C_CFG_SW_SCL_PIN GPIO_PIN_6
#define BOARD_I2C_CFG_SW_SDA_PORT GPIOB  // 软件IIC SDA引脚
#define BOARD_I2C_CFG_SW_SDA_PIN GPIO_PIN_7
#define BOARD_I2C_CFG_LL_INSTANCE I2C1    // LL库IIC实例
#define BOARD_I2C_CFG_HAL_INSTANCE hi2c1  // HAL库IIC实例

/*********************************************************************/
/**************************** 设置结束 *******************************/
#endif  // __MODULES_CONF_H

该Modules文件夹下各文件的作用:

> modules.h 用于定义各个模块的时基函数, 必须编译
> macro.h 辅助宏文件模块, 必须编译

> perf_counter.c/h systick_wrapper_ual.s 代替systick提供高精度ns级时基和性能测试模块, 强烈推荐编译

> uart_pack.c/h HAL串口通讯封装模块, 推荐编译
> log.h 调试日志模块, 推荐编译, 依赖uart_pack
> scheduler.c/h 任务调度器模块, 推荐编译
> scheduler_lite.c/h 轻量级任务调度器模块, 和scheduler二选一

> key.c/h 完善的多按键模块, 可选编译
> led.c/h 简单LED点灯模块, 可选编译
> fifo.h 通用FIFO(先进先出环形缓冲区)创建辅助模块, 可选编译
> queue.h 队列创建辅助模块, 可选编译
> pid.c/h 高级PID控制器模块, 可选编译
> motor.c/h 闭环直流电机控制模块, 可选编译, 依赖pid
> stepper.c/h 简单步进电机控制模块, 可选编译
> sw_i2c.c/h 软件I2C模块, 可选编译
> modbus.c/h modbus通讯协议模块, 可选编译
> cstring.c/h 类C++的字符串模块, 替代string.h, 可选编译

四个文件夹的作用:

> /benchmark MCU性能测试模块, 图一乐
> /dalloc 动态内存分配模块, 解决malloc/free的内存碎片问题, 用到的时候再说
> /nr_micro_shell 一个简单的命令行shell模块, 方便测试函数, 实际上基本用不到
> /RTT Segger RTT模块, 用于不需要串口的调试日志输出, 感兴趣自己学, 基本用不到

所有不编译的文件最好是不要复制到工程里, 以免造成不必要的麻烦

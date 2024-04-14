# 嵌入式STM32软件模块集

采用统一上层头文件来统一动态内存/时间/延时函数，并实现可视化参数配置

> [!IMPORTANT]
> 主要是给stm32项目用的，部分模块依赖hal库

## 模块仓库文件结构

> [!NOTE]
> 标*的为自己写的模块，其他修改自开源库，原repo已列出(标x为非开源库)

<details open>
  <summary>根目录文件</summary>

| 根目录文件 | 功能 |
|-|-|
| [.clang-format](./.clang-format) | clang代码格式化配置文件 |
| [modules.h](./modules.h) | 模块统一头文件 |
| [tool.py](./tool.py) | 模块配置工具 |
| [~modules_conf.template.h~](./modules_conf.template.h) | ~模块统一配置文件模板~ ***(deprecated)*** |

</details>

<details>
  <summary>算法模块</summary>

| [Algorithm](./algorithm) | 算法 | repo | 备注 |
|-|-|:-:|-|
| [cmsis_dsp](./algorithm/cmsis_dsp) | CMSIS-DSP(Src) | [link](https://github.com/ARM-software/CMSIS-DSP) | 源码形式 |
| [libcrc](./algorithm/libcrc) | CRC计算库 | [link](https://github.com/whik/crc-lib-c) | |
| [pid](./algorithm/pid) | 通用PID控制器 |*| |
| [quaternion](./algorithm/quaternion) | 四元数和IMU姿态估计 | [link](https://github.com/rbv188/IMU-algorithm) | 未测试 |
| [tiny_regex](./algorithm/tiny_regex)|  简易正则解析器 | [link](https://github.com/zeta-zero/tiny-regex-c) | 无捕获组 |

</details>

<details>
  <summary>通信模块</summary>

| [Communication](./communication) | 通信 | repo | 备注 |
|-|-|:-:|-|
| [CherryUSB](./communication/cherryusb) | Cherry USB | [link](https://github.com/cherry-embedded/CherryUSB) | |
| [lwpkt](./communication/lwpkt) | 轻量级数据包 | [link](https://github.com/MaJerle/lwpkt) | |
| [minmea](./utility/minmea) | GPS NMEA解析器 | [link](https://github.com/ata4/minema) | |
| [modbus](./communication/modbus) | Modbus协议 | [link](https://github.com/wql7013/ModBus) | |
| [TinyFrame](./communication/tinyframe) | 另一个轻量级数据包 | [link](https://github.com/MightyPork/TinyFrame) | |
| [xymodem](./communication/xymodem) | X/YMODEM协议 | [link](https://github.com/LONGZR007/IAP-STM32) | |

</details>

<details>
  <summary>数据结构模块</summary>

| [DataStruct](./datastruct) | 数据结构 | repo | 备注 |
|-|-|:-:|-|
| [btree](./datastruct/btree) | B树 | [link](https://github.com/tidwall/btree.c) | |
| [cstring](./datastruct/cstring) | C字符串 | [link](https://github.com/cloudwu/cstring) | |
| [ctl](./datastruct/ctl) | 类型安全C模板容器库 | [link](https://github.com/rurban/ctl) | |
| [dlist](./datastruct/dlist) | 双向链表 | [link](https://github.com/clibs/list) | |
| [fifofast](./datastruct/fifofast) | 纯头文件快速FIFO | [link](https://github.com/nqtronix/fifofast) | |
| [hashmap](./datastruct/hashmap) | 哈希表 | [link](https://github.com/tidwall/hashmap.c) | |
| [json](./datastruct/json) | JSON解析 | [link](https://github.com/tidwall/json.c) | |
| [lfbb](./datastruct/lfbb) | 二分无锁缓冲区 | [link](https://github.com/DNedic/lfbb) | |
| [lfifo](./datastruct/lfifo) | 通用环形缓冲区 |*| 比lwrb更高效 |
| [linux_list](./datastruct/linux_list) | Linux-like链表 | [link](https://github.com/sysprog21/linux-list) | |
| [lwrb](./datastruct/lwrb) | 轻量环形缓冲区 | [link](https://github.com/MaJerle/lwrb) | |
| [pqueue](./datastruct/pqueue) | 优先队列 | [link](https://github.com/tidwall/pqueue.c) | |
| [sds](./datastruct/sds) | 简单动态字符串 | [link](https://github.com/antirez/sds) | |
| [struct2json](./datastruct/struct2json) | C结构体与JSON快速互转库 | [link](https://github.com/armink/struct2json) | |
| [udict](./datastruct/udict) | 通用哈希字典 |*| 基于uthash |
| [ulist](./datastruct/ulist) | 通用内存连续列表 |*| |
| [uthash](./datastruct/uthash) |基于宏的可嵌入哈希表 | [link](https://github.com/troydhanson/uthash) | |

</details>

<details>
  <summary>调试模块</summary>

| [Debug](./debug) | 调试 | repo | 备注 |
|-|-|:-:|-|
| [benchmark](./debug/benchmark) | CoreMark基准测试 | [link](https://github.com/eembc/coremark) | |
| [cm_backtrace](./debug/cm_backtrace) | hardfault堆栈回溯 | [link](https://github.com/armink/CmBacktrace) | |
| [RTT](./debug/rtt) | Segger-RTT 调试模块 | [link](https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/) | |
| [log](./debug/log) | 纯头文件日志库 |*| |
| [minctest](./debug/minctest) | 简易单元测试 | [link](https://github.com/codeplea/minctest) | |

</details>

<details>
  <summary>驱动模块</summary>

| [Driver](./driver) | 驱动 | repo | 备注 |
|-|-|:-:|-|
| [bq25890](./driver/bq25890) | BQ2589x充电芯片 | [link](https://github.com/SumantKhalate/BQ25895) | |
| [ee24](./peripheral/ee24) | 24xx EEPROM库 | [link](https://github.com/nimaltd/ee24) | |
| [key](./peripheral/key) | 通用按键驱动 |*| 支持多种事件 |
| [motor](./peripheral/motor) | 直流电机闭环驱动 | * | |
| [paj7620u2](./driver/paj7620u2) | PAJ7620U2手势识别 | * | |
| [sc7a20](./driver/sc7a20) | SC7A20加速度计 |*| |
| [sh2](./driver/sh2) | SH2 Sensorhub协议 | [link](https://github.com/ceva-dsp/sh2) | |
| [spif](./peripheral/spif) | SPI Flash通用驱动 | [link](https://github.com/nimaltd/spif) | |
| [stepper](./peripheral/stepper) | 步进电机驱动 |*| |
| [vl53l0x](./driver/vl53l0x) | VL53L0X激光测距 | [link](https://github.com/anisyanka/vl53l0x) | 非官方库 |
| [ws2812_spi](./peripheral/ws2812_spi) | WS2812灯带DMA-SPI驱动 |*| |

</details>

<details>
  <summary>图形模块</summary>

| [Graphics](./graphics) | 图形 | repo | 备注 |
|-|-|:-:|-|
| [easy_ui](./graphics/easy_ui) | 单色屏UI库 | [link](https://github.com/ErBWs/Easy-UI) | 大幅魔改 |
| [hagl](./graphics/hagl) | HAL图形库 | [link](https://github.com/tuupola/hagl) | |
| [lvgl](./graphics/lvgl) | LittlevGL图形库 | [link](https://github.com/lvgl/lvgl) | |
| [lvgl_gaussian_blur](./graphics/lvgl_gaussian_blur) | LVGL高斯模糊效果 | [link](https://gitee.com/MIHI1/lvgl_gaussian_blur) | cpp->c |
| [lvgl-pm](./graphics/lvgl-pm) | LVGL页面管理器 | [link](https://github.com/LanFly/lvgl-pm) | |
| [u8g2](./graphics/u8g2) | U8g2图形库 | [link](https://github.com/olikraus/u8g2) | |
| [ugui](./graphics/ugui) | uGUI图形库 | [link](https://github.com/achimdoebler/UGUI) | |
| [virtual_lcd](./graphics/virtual_lcd) | 虚拟LCD |*| 包含上位机 |

</details>

<details>
  <summary>神经网络模块</summary>

| [NN](./nn) | 神经网络 | repo | 备注 |
|-|-|:-:|-|
| [genann](./nn/genann) | 简单前馈神经网络 | [link](https://github.com/codeplea/genann) | |

| [Peripheral](./peripheral) | 外设 | repo | 备注 |
|-|-|:-:|-|
| [board_i2c](./peripheral/board_i2c) | 板级I2C包装层 |*| |
| [board_led](./peripheral/board_led) | 板级LED包装层 |*| |
| [ee](./peripheral/ee) | 内置flash读写库 | [link](https://github.com/nimaltd/ee) | |
| [i2c_salve](./peripheral/i2c_slave) | LL库I2C从机 |*| |
| [ll_i2c](./peripheral/ll_i2c) | LL库I2C | * | 包含中断/轮询 |
| [sw_i2c](./peripheral/sw_i2c) | 软件I2C | [link](https://github.com/liyanboy74/soft-i2c) | |
| [sw_spi](./peripheral/sw_spi) | 软件SPI | x | |
| [uni_io](./peripheral/uni_io) | 数据通信功能包 |*| |

</details>

<details>
  <summary>存储模块</summary>

| [Storage](./storage) | 存储 | repo | 备注 |
|-|-|:-:|-|
| [easyflash](./storage/easyflash) | 轻量级Flash数据库 | [link](https://github.com/armink/EasyFlash) | |
| [littlefs](./storage/littlefs) | LittleFS | [link](https://github.com/littlefs-project/littlefs) | |
| [MiniFlashDB](./storage/miniflashdb) | 轻量级Flash数据库 | [link](https://github.com/Jiu-xiao/MiniFlashDB) | 魔改 |

</details>

<details>
  <summary>系统模块</summary>

| [System](./system) | 系统 | repo | 备注 |
|-|-|:-:|-|
| [dalloc](./system/dalloc) | 动态指针管理内存分配器 | [link](https://github.com/SkyEng1neering/dalloc) | |
| [heap4](./system/heap4) | FreeRTOS堆4 | [link](https://www.freertos.org/a00111.html) | |
| [klite](./system/klite) | 基础实时内核 | [link](https://gitee.com/kerndev/klite) | 轻量高性能,推荐 |
| [lwmem](./system/lwmem) | 轻量级内存管理 | [link](https://github.com/MaJerle/lwmem) | 性能远不如heap4|
| [rtthread_nano](./system/rtthread_nano) | RT-Thread Nano | [link](https://github.com/RT-Thread/rtthread-nano) | |
| [s_task](./system/s_task) | 精简的协程实现 | [link](https://github.com/xhawk18/s_task) | 需要实现栈切换 |
| [scheduler](./system/scheduler) | 多功能任务调度器 |*| 内有使用说明 |
| [scheduler_lite](./system/scheduler_lite) | 轻量级任务调度器 |*| |

</details>

<details>
  <summary>工具模块</summary>

| [Utility](./utility) | 工具 | repo | 备注 |
|-|-|:-:|-|
| [cot_menu](./utility/cot_menu) | 轻量级菜单框架 | [link](https://gitee.com/cot_package/cot_menu) | 抽象菜单 |
| [embedded_cli](./utility/embedded_cli) | 嵌入式命令行 | [link](https://github.com/funbiscuit/embedded-cli) | 魔改 |
| [lwprintf](./utility/lwprintf) | 轻量级无缓冲区printf | [link](https://github.com/MaJerle/lwprintf) | |
| [perf_counter](./utility/perf_counter) | PerfCounter性能统计/时基库 | [link](https://github.com/GorgonMeducer/perf_counter) | 必备品 |
| [ryu](./utility/ryu) | 浮点数转字符串 | [link](https://github.com/tidwall/ryu) | |
| [term_table](./utility/term_table) | 动态终端表格工具 |*| 仅debug使用 |
| [TimeLib](./utility/TimeLib) | UNIX时间库 | [link](https://github.com/geekfactory/TimeLib) | |
| [xv](./utility/xv) | 类JavaScript的字符串解析器 | [link](https://github.com/tidwall/xv) | |
| [incbin.h](./utility/incbin) | 二进制文件嵌入 | [link](https://github.com/graphitemaster/incbin) | |
| [macro.h](./utility/macro.h) | 通用宏 |*| |

</details>

## 配置工具 [`tool.py`](./tool.py)

```bash
Ellu@ELLU  /home/ellu/git/EmbeddedModules   master ≣ +0 ~1 -0 !
❯ python ./tool.py

usage: tool.py [-m] [-n] [-g] [-u] [-k KCONFIG] [-c CONFIG] [-o OUTPUT_DIR]

optional arguments:
  -m, --menuconfig      Run menuconfig
  -n, --newmodule       Create a new module
  -g, --generate        Generate header file without menuconfig
  -u, --update          Pull the latest version of the tool from github
  -k KCONFIG, --kconfig KCONFIG
                        Specify the kconfig file, default is Kconfig
  -c CONFIG, --config CONFIG
                        Specify the menuconfig output file, default is .config
  -o OUTPUT_DIR, --output_dir OUTPUT_DIR
                        Specify the directory for the output files, or use MOD_OUTPUT_DIR env variable
```

使用Kconfig可视化配置并生成头文件:

```shell
python tool.py -m
```

创建新模块:

```shell
python tool.py -n
```

## TODO

- [x] 用kconfig替代modules_conf.template.h
- [ ] 为所有自己写的模块编写README

## LICENSE

MIT (For self-written modules only)

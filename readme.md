# [Ellu's Embedded Modules](https://github.com/ElluIFX/EmbeddedModules)

嵌入式STM32软件模块集

采用统一上层头文件来统一动态内存/时间/延时函数，并实现可视化参数配置

> [!IMPORTANT]
> 主要是给stm32项目用的，部分模块依赖hal库

## 模块仓库文件结构

> [!NOTE]
> 标 `*` 的为自己写的模块，标 `x` 为非开源第三方库，其他修改自开源库，来源地址已列出

<details open>
  <summary>根目录文件 /</summary>

| 根目录文件                       | 功能           |
| -------------------------------- | -------------- |
| [.clang-format](./.clang-format) | 格式化配置文件 |
| [modules.h](./modules.h)         | 模块统一头文件 |
| [tool.py](./tool.py)             | 模块配置工具   |

</details>

<details>
  <summary>算法模块 /algorithm</summary>

| [Algorithm](./algorithm)             | 算法                |                        src                        | 备注     | SHA     |
| ------------------------------------ | ------------------- | :-----------------------------------------------: | -------- | ------- |
| [cmsis_dsp](./algorithm/cmsis_dsp)   | CMSIS-DSP(Src)      | [link](https://github.com/ARM-software/CMSIS-DSP) | 源码形式 | 03fa0e5 |
| [libcrc](./algorithm/libcrc)         | CRC计算库           |     [link](https://github.com/whik/crc-lib-c)     |          | abe136a |
| [pid](./algorithm/pid)               | 通用PID控制器       |                         *                         |          |         |
| [quaternion](./algorithm/quaternion) | 四元数和IMU姿态估计 |  [link](https://github.com/rbv188/IMU-algorithm)  | 未测试   | bd77afd |

</details>

<details>
  <summary>通信模块 /communication</summary>

| [Communication](./communication)       | 通信               |                         src                          | 备注 | SHA     |
| -------------------------------------- | ------------------ | :--------------------------------------------------: | ---- | ------- |
| [CherryUSB](./communication/cherryusb) | Cherry USB         | [link](https://github.com/cherry-embedded/CherryUSB) |      | 9cb992b |
| [lwpkt](./communication/lwpkt)         | 轻量级数据包       |       [link](https://github.com/MaJerle/lwpkt)       |      | 6a82dab |
| [minmea](./communication/minmea)       | GPS NMEA解析器     |        [link](https://github.com/ata4/minema)        |      | 450ad08 |
| [modbus](./communication/modbus)       | Modbus协议         |      [link](https://github.com/wql7013/ModBus)       |      | 0745519 |
| [TinyFrame](./communication/tinyframe) | 另一个轻量级数据包 |   [link](https://github.com/MightyPork/TinyFrame)    |      | a29167a |
| [xymodem](./communication/xymodem)     | X/YMODEM协议       |    [link](https://github.com/LONGZR007/IAP-STM32)    |      | f7b988d |

</details>

<details>
  <summary>数据结构模块 /datastruct</summary>

| [DataStruct](./datastruct)              | 数据结构                |                       src                       | 备注         | SHA     |
| --------------------------------------- | ----------------------- | :---------------------------------------------: | ------------ | ------- |
| [btree](./datastruct/btree)             | B树                     |   [link](https://github.com/tidwall/btree.c)    |              | c0cfc4e |
| [cstring](./datastruct/cstring)         | C字符串                 |   [link](https://github.com/cloudwu/cstring)    |              | 88e021b |
| [ctl](./datastruct/ctl)                 | 类型安全C模板容器库     |      [link](https://github.com/rurban/ctl)      |              | d314c08 |
| [dlist](./datastruct/dlist)             | 双向链表                |      [link](https://github.com/clibs/list)      |              | 23faa20 |
| [fifofast](./datastruct/fifofast)       | 纯头文件快速FIFO        |  [link](https://github.com/nqtronix/fifofast)   |              | 196edda |
| [hashmap](./datastruct/hashmap)         | 哈希表                  |  [link](https://github.com/tidwall/hashmap.c)   |              | 1c13992 |
| [json](./datastruct/json)               | JSON解析                |    [link](https://github.com/tidwall/json.c)    |              | 3d0e877 |
| [lfbb](./datastruct/lfbb)               | 二分无锁缓冲区          |     [link](https://github.com/DNedic/lfbb)      |              | 8c24b34 |
| [lfifo](./datastruct/lfifo)             | 通用环形缓冲区          |                        *                        | 比lwrb更高效 |         |
| [linux_list](./datastruct/linux_list)   | Linux-like链表          | [link](https://github.com/sysprog21/linux-list) |              | 452262e |
| [lwrb](./datastruct/lwrb)               | 轻量环形缓冲区          |     [link](https://github.com/MaJerle/lwrb)     |              | b32c645 |
| [pqueue](./datastruct/pqueue)           | 优先队列                |   [link](https://github.com/tidwall/pqueue.c)   |              | 2bb5600 |
| [sds](./datastruct/sds)                 | 简单动态字符串          |     [link](https://github.com/antirez/sds)      |              | a9a03bb |
| [struct2json](./datastruct/struct2json) | C结构体与JSON快速互转库 |  [link](https://github.com/armink/struct2json)  |              | 4f1fdc9 |
| [udict](./datastruct/udict)             | 通用哈希字典            |                        *                        | 基于uthash   |         |
| [ulist](./datastruct/ulist)             | 通用内存连续列表        |                        *                        |              |         |
| [uthash](./datastruct/uthash)           | 基于宏的可嵌入哈希表    |  [link](https://github.com/troydhanson/uthash)  |              | eeba196 |

</details>

<details>
  <summary>调试模块 /debug</summary>

| [Debug](./debug)                     | 调试                |                      src                      | 备注 | SHA     |
| ------------------------------------ | ------------------- | :-------------------------------------------: | ---- | ------- |
| [benchmark](./debug/benchmark)       | CoreMark基准测试    |   [link](https://github.com/eembc/coremark)   |      | d5fad6b |
| [cm_backtrace](./debug/cm_backtrace) | hardfault堆栈回溯   | [link](https://github.com/armink/CmBacktrace) |      | 6013293 |
| [RTT](./debug/rtt)                   | Segger-RTT 调试模块 |      [link](https://wiki.segger.com/RTT)      |      |         |
| [log](./debug/log)                   | 纯头文件日志库      |                       *                       |      |         |
| [minctest](./debug/minctest)         | 简易单元测试        | [link](https://github.com/codeplea/minctest)  |      | 0ab5834 |

</details>

<details>
  <summary>驱动模块 /driver</summary>

| [Driver](./driver)                | 驱动                  |                       src                        | 备注         | SHA     |
| --------------------------------- | --------------------- | :----------------------------------------------: | ------------ | ------- |
| [bq25890](./driver/bq25890)       | BQ2589x充电芯片       | [link](https://github.com/SumantKhalate/BQ25895) |              | ade0e3c |
| [ee24](./driver/ee24)             | 24xx EEPROM库         |     [link](https://github.com/nimaltd/ee24)      |              | 92816a7 |
| [key](./driver/key)               | 通用按键驱动          |                        *                         | 支持多种事件 |         |
| [motor](./driver/motor)           | 直流电机闭环驱动      |                        *                         |              |         |
| [paj7620u2](./driver/paj7620u2)   | PAJ7620U2手势识别     |                        *                         |              |         |
| [sc7a20](./driver/sc7a20)         | SC7A20加速度计        |                        *                         |              |         |
| [sh2](./driver/sh2)               | SH2 Sensorhub协议     |     [link](https://github.com/ceva-dsp/sh2)      |              | b514b1e |
| [spif](./driver/spif)             | SPI Flash通用驱动     |     [link](https://github.com/nimaltd/spif)      |              | c0f3ba2 |
| [stepper](./driver/stepper)       | 步进电机驱动          |                        *                         |              |         |
| [vl53l0x](./driver/vl53l0x)       | VL53L0X激光测距       |   [link](https://github.com/anisyanka/vl53l0x)   | 非官方库     | 04891c2 |
| [ws2812_spi](./driver/ws2812_spi) | WS2812灯带DMA-SPI驱动 |                        *                         |              |         |

</details>

<details>
  <summary>图形模块 /graphics</summary>

| [Graphics](./graphics)                              | 图形             |                        src                         | 备注       | SHA     |
| --------------------------------------------------- | ---------------- | :------------------------------------------------: | ---------- | ------- |
| [easy_ui](./graphics/easy_ui)                       | 单色屏UI库       |      [link](https://github.com/ErBWs/Easy-UI)      | 大幅魔改   | 691bdb4 |
| [hagl](./graphics/hagl)                             | HAL图形库        |      [link](https://github.com/tuupola/hagl)       |            | 8281a8a |
| [lvgl](./graphics/lvgl)                             | LittlevGL图形库  |        [link](https://github.com/lvgl/lvgl)        |            | 3aac8cc |
| [lvgl_gaussian_blur](./graphics/lvgl_gaussian_blur) | LVGL高斯模糊效果 | [link](https://gitee.com/MIHI1/lvgl_gaussian_blur) | cpp->c     |         |
| [lvgl_pm](./graphics/lvgl_pm)                       | LVGL页面管理器   |     [link](https://github.com/LanFly/lvgl-pm)      |            | 825df21 |
| [u8g2](./graphics/u8g2)                             | U8g2图形库       |      [link](https://github.com/olikraus/u8g2)      |            | 3e86287 |
| [ugui](./graphics/ugui)                             | uGUI图形库       |    [link](https://github.com/achimdoebler/UGUI)    |            | ce0bccb |
| [virtual_lcd](./graphics/virtual_lcd)               | 虚拟LCD          |                         *                          | 包含上位机 |         |

</details>

<details>
  <summary>神经网络模块 /nn</summary>

| [NN](./nn)            | 神经网络         |                    src                     | 备注 | SHA     |
| --------------------- | ---------------- | :----------------------------------------: | ---- | ------- |
| [genann](./nn/genann) | 简单前馈神经网络 | [link](https://github.com/codeplea/genann) |      | 4f72209 |

</details>

<details>
  <summary>外设模块 /peripheral</summary>

| [Peripheral](./peripheral)            | 外设               |                      src                       | 备注          | SHA     |
| ------------------------------------- | ------------------ | :--------------------------------------------: | ------------- | ------- |
| [board_i2c](./peripheral/board_i2c)   | 板级I2C包装层      |                       *                        |               |         |
| [board_led](./peripheral/board_led)   | 板级LED包装层      |                       *                        |               |         |
| [ee](./peripheral/ee)                 | 内置flash读写库    |     [link](https://github.com/nimaltd/ee)      |               | 460d569 |
| [i2c_salve](./peripheral/i2c_slave)   | LL库I2C从机        |                       *                        |               |         |
| [ll_i2c](./peripheral/ll_i2c)         | LL库I2C            |                       *                        | 包含中断/轮询 |         |
| [mr_library](./peripheral/mr_library) | 轻量级设备读写接口 |  [link](https://gitee.com/MacRsh/mr-library)   |               |         |
| [sw_i2c](./peripheral/sw_i2c)         | 软件I2C            | [link](https://github.com/liyanboy74/soft-i2c) |               | c595a39 |
| [sw_spi](./peripheral/sw_spi)         | 软件SPI            |                       x                        |               |         |
| [uni_io](./peripheral/uni_io)         | 数据通信功能包     |                       *                        |               |         |

</details>

<details>
  <summary>存储模块 /storage</summary>

| [Storage](./storage)                 | 存储              |                         src                          | 备注 | SHA     |
| ------------------------------------ | ----------------- | :--------------------------------------------------: | ---- | ------- |
| [easyflash](./storage/easyflash)     | 轻量级Flash数据库 |     [link](https://github.com/armink/EasyFlash)      |      | a67fffc |
| [littlefs](./storage/littlefs)       | LittleFS          | [link](https://github.com/littlefs-project/littlefs) |      | d01280e |
| [MiniFlashDB](./storage/miniflashdb) | 轻量级Flash数据库 |   [link](https://github.com/Jiu-xiao/MiniFlashDB)    | 魔改 | 99bf7aa |

</details>

<details>
  <summary>系统模块 /system</summary>

| [System](./system)                        | 系统                   |                        src                         | 备注            | SHA     |
| ----------------------------------------- | ---------------------- | :------------------------------------------------: | --------------- | ------- |
| [dalloc](./system/dalloc)                 | 动态指针管理内存分配器 |  [link](https://github.com/SkyEng1neering/dalloc)  |                 | da14f0f |
| [heap4](./system/heap4)                   | FreeRTOS堆4            |    [link](https://www.freertos.org/a00111.html)    |                 |         |
| [klite](./system/klite)                   | 基础实时内核           |      [link](https://gitee.com/kerndev/klite)       | 轻量高性能,推荐 |         |
| [lwmem](./system/lwmem)                   | 轻量级内存管理         |      [link](https://github.com/MaJerle/lwmem)      | 性能远不如heap4 | d7a159c |
| [rtthread_nano](./system/rtthread_nano)   | RT-Thread Nano         | [link](https://github.com/RT-Thread/rtthread-nano) |                 | 9177e3e |
| [s_task](./system/s_task)                 | 精简的协程实现         |     [link](https://github.com/xhawk18/s_task)      | 需要实现栈切换  | 609835c |
| [scheduler](./system/scheduler)           | 多功能任务调度器       |                         *                          | 内有使用说明    |         |
| [scheduler_lite](./system/scheduler_lite) | 轻量级任务调度器       |                         *                          |                 |         |

</details>

<details>
  <summary>工具模块 /utility</summary>

| [Utility](./utility)                   | 工具                       |                          src                          | 备注        | SHA     |
| -------------------------------------- | -------------------------- | :---------------------------------------------------: | ----------- | ------- |
| [cot_menu](./utility/cot_menu)         | 轻量级菜单框架             |    [link](https://gitee.com/cot_package/cot_menu)     | 抽象菜单    |         |
| [embedded_cli](./utility/embedded_cli) | 嵌入式命令行               |  [link](https://github.com/funbiscuit/embedded-cli)   | 魔改        | ffa8014 |
| [lwprintf](./utility/lwprintf)         | 轻量级无缓冲区printf       |      [link](https://github.com/MaJerle/lwprintf)      |             | 18a1338 |
| [perf_counter](./utility/perf_counter) | PerfCounter性能统计/时基库 | [link](https://github.com/GorgonMeducer/perf_counter) | 必备品      | 0b17943 |
| [ryu](./utility/ryu)                   | 浮点数转字符串             |        [link](https://github.com/tidwall/ryu)         |             | 5056abc |
| [term_table](./utility/term_table)     | 动态终端表格工具           |                           *                           | 仅debug使用 |         |
| [TimeLib](./utility/TimeLib)           | UNIX时间库                 |    [link](https://github.com/geekfactory/TimeLib)     |             | 8bdf963 |
| [xv](./utility/xv)                     | 类JavaScript的字符串解析器 |         [link](https://github.com/tidwall/xv)         |             | b46851f |
| [tiny_regex](./utility/tiny_regex)     | 简易正则解析器             |   [link](https://github.com/zeta-zero/tiny-regex-c)   | 无捕获组    | 9d5f5d8 |
| [incbin.h](./utility/incbin)           | 二进制文件嵌入             |   [link](https://github.com/graphitemaster/incbin)    |             | 6e576ca |
| [macro.h](./utility/macro.h)           | 通用宏                     |                           *                           |             |         |

</details>

## 配置文件

- `Kconfig` - Kconfig配置文件, 用于配置代码的宏定义, 开关和设置各种功能, 遵循Linux内核的[Kconfig规范](https://github.com/torvalds/linux/blob/master/Documentation/kbuild/kconfig-language.rst)
- `Mconfig` - 基于Kconfig的配置, 描述生成项目的模块文件夹时所需复制的模块文件, 如不存在则复制完整文件夹

<details>
  <summary>Mconfig 语法</summary>

Mconfig文件实际上是一个python脚本, 继承完整的`tool.py`运行环境

其中有四个特殊变量和三个特殊函数:

- `CONFIG` - 从Kconfig配置结果中解析的配置项目, 访问不存在的项目将返回`False`
- `IGNORES` - 复制该模块的文件时忽略的项目, 使用glob匹配
- `DST_PATH` - 本模块文件夹的目标路径
- `SRC_PATH` - 本模块文件夹的源路径
- `def DEBUG(msg: str)` - 输出调试信息 (`--debug`)
- `def WARNING(msg: str)` - 输出警告信息
- `def ERROR(msg: str)` - 输出错误信息并退出

下面是一个简单的例子:

```python
if CONFIG.DISABLE_MODULE_A: # 也支持.get()方法来定义不存在时的默认返回值
    IGNORES += "module_a.*"
if CONFIG.DISABLE_SUB_MODULES:
    DEBUG(f"SUB_MODULES: {CONFIG.SUB_MODULES}")
    if "B" in CONFIG.SUB_MODULES:
        IGNORES += ["module_b1.*", "module_b2.*"]
    IGNORES += "module_c*.*"
IGNORES += "test_*_module.*"
```

</details>

## 配置工具 [`tool.py`](./tool.py)

```shell
Ellu@ELLU  /home/ellu/git/EmbeddedModules   master ≣ +0 ~0 -0
❯ python ./tool.py --help

usage: tool.py [-p PROJECT_DIR] [-m] [-s] [-n] [-u] [-d MODULE_DIRNAME] [--debug]

optional arguments:
  -h, --help            show this help message and exit
  -p PROJECT_DIR, --project-dir PROJECT_DIR
                        Specify the directory for working project, default is current directory
  -m, --menuconfig      Run menuconfig in project dir
  -g, --guiconfig       Run menuconfig with GUI
  -s, --sync            Sync latest module files without menuconfig
  -ns, --no-sync        Skip syncing latest module files after menuconfig
  -n, --newmodule       Create a new module
  -u, --update          Pull the latest version of this toolset from github
  -a, --analyze         Analyze module dependencies
  -c, --check           Check for updates of modules
  -d MODULE_DIRNAME, --module-dirname MODULE_DIRNAME
                        Specify the directory name for generated modules, default is 'Modules'
  -fc, --force-copy     Force copy files even if destination is newer
  -gm, --gen-makefile   Generate makefile for source files
  --debug               Enable debug output
```

- **在命令行中配置并更新项目模块文件夹**

  ```shell
  python tool.py -p /path/to/project -m
  ```

- **在GUI中配置并更新项目模块文件夹** (需要Python环境支持tkinter)

  ```shell
  python tool.py -p /path/to/project -g
  ```

- **使用已有配置从仓库同步最新模块文件**

  ```shell
  python tool.py -p /path/to/project -u -s
  ```

- **在仓库中创建新模块**

  ```shell
  python tool.py -n
  ```

- **列出所有模块的依赖关系** (可用于辅助编写Kconfig)

  ```shell
  python tool.py -a
  ```

- **检查所有模块的更新情况** (比较本文件记录的SHA和仓库中最新提交的SHA)

  ```shell
  python tool.py -c
  ```

- **冻结生成文件夹中的某个模块** (如针对项目修改了模块源码时)

  在生成文件夹 (如`Modules`) 的根目录创建一个`.Mfreeze`文件, 写入要冻结的模块文件夹名, 每行一个

- **有效的环境变量**

  - `MOD_PROJECT_DIR` - 指定项目目录 (命令参数优先)
  - `MOD_MODULE_DIRNAME` - 指定生成模块文件夹的目录名 (命令参数优先)

## TODO

- [x] 用kconfig替代modules_conf.template.h
- [x] 配置工具自动复制模块文件夹
- [x] 允许配置Mconfig来指定添加项目
- [ ] 为所有自己写的模块编写README

## LICENSE

MIT (For self-written modules only)

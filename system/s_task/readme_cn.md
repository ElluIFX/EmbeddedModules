# s_task - 跨平台的C语言协程多任务库

## 目录

- [s\_task - 跨平台的C语言协程多任务库](#s_task---跨平台的c语言协程多任务库)
  - [目录](#目录)
  - [特性](#特性)
  - [协程 vs 多线程](#协程-vs-多线程)
  - [示例](#示例)
    - [示例 1 - 创建简单任务](#示例-1---创建简单任务)
    - [示例 2 - （无需回调函数的）异步HTTP客户端程序](#示例-2---无需回调函数的异步http客户端程序)
    - [示例 3 - ardinuo下同时跑多个任务控制led闪烁](#示例-3---ardinuo下同时跑多个任务控制led闪烁)
  - [兼容性列表](#兼容性列表)
  - [编译](#编译)
    - [Posix - Linux / FreeBSD / MacOS / MingW(MSYS2)](#posix---linux--freebsd--macos--mingwmsys2)
    - [其他 - Windows / STM8 / Cortex-M / Arduino, 和更多单片机](#其他---windows--stm8--cortex-m--arduino-和更多单片机)
  - [如何在您的项目中使用s\_task？](#如何在您的项目中使用s_task)
  - [API](#api)
    - [Task （任务）](#task-任务)
    - [Chan （数据通道）](#chan-数据通道)
    - [Mutex （互斥量）](#mutex-互斥量)
    - [Event （事件）](#event-事件)
  - [嵌入式平台](#嵌入式平台)
    - [Chan for interrupt (中断和任务的数据通道，仅嵌入式平台支持，STM8/STM32/M051/Arduino)](#chan-for-interrupt-中断和任务的数据通道仅嵌入式平台支持stm8stm32m051arduino)
      - [任务里调用的 chan api](#任务里调用的-chan-api)
      - [中断里调用 chan api](#中断里调用-chan-api)
    - [Event for interrupt (中断里的事件，仅嵌入式平台支持，STM8/STM32/M051/Arduino)](#event-for-interrupt-中断里的事件仅嵌入式平台支持stm8stm32m051arduino)
      - [任务里调用的 event api](#任务里调用的-event-api)
      - [中断里调用的 event api](#中断里调用的-event-api)
  - [希望移植到新的平台？](#希望移植到新的平台)
  - [联系方式](#联系方式)
  - [其他协程库对比](#其他协程库对比)
  - [感谢](#感谢)

## 特性

- 全c和汇编实现，紧凑小巧又不失实用，并且不需要c++。
- 协程切换代码来自boost汇编，性能极好，稳定可靠，移植性好，几乎全平台支持。
- 和libuv（稍作修改）无缝融合，完美支持跨平台网络编程。
- 支持 __await__ , __async__ 关键词，含义和用法都其他语言的await/async相同 --
    没有调用 await 函数的地方，协程肯定不会被切换出去，可确保共享数据不会被其他协程改变。
    具备传染性，能调用 await 的函数，一定在一个 async 函数里。这个async 函数需要用 await 调用。
- 支持协程间的event变量、mutex锁、chan数据通道等，方便不同协程间同步数据和状态。这个方式比其他协程resume函数更好用和可控。
- 除支持windows, linux, macos这些常规环境外，更能为stm32等嵌入式小芯片环境提供多任务支持（注：小芯片环境下不支持libuv)。
- 在嵌入式小芯片下使用，s_task是个恰到好处的RTOS -- 没有动态内存分配，增加程序大小不到 1.5k, 不增加程序空间负担，支持任务和中断间通讯。

## 协程 vs 多线程

协程和多线程编程模式对比，协程的优势极其明显 --

- **协程不会陷入死锁的窘境。**

- **协程需要的代码量极小。**

  一般协程比多线程更少的代码量就能实现，这在资源捉襟见肘的嵌入式单片机中尤其重要。有时跑个多线程 RTOS 库，应用自己都没空间了。s_task协程只增加了不到 1.5K 的代码量，这对单片机极其友好。

- **协程主动让出CPU，没有也不需要 “抢占式多任务” 。**

  您没看错，人们已经开始反思，“抢占式多任务” 根本不是啥优势，而是__多线程最大的缺点__，更是__bug之源__。主动让出CPU的协程，减少bug的同时，更能带来更好的CPU利用率，更多的并发任务数。这也是近年来，不管C++, C#, nodejs, java, php各式语言，都开始引入协程的原因。

- **协程比任何的所谓 “实时操作系统RTOS” 更实时。**

- **协程有 __await__ 标注任务可能切换。**

  通过__await__标注，程序员明确的知道，在某个函数在运行的时候，CPU可能会被切换到其他任务。如此多任务间共享变量变得无比安全，这点是线程不能比的。

s_task协程库，更是打造了全平台兼容的协程支持环境，从高端linux服务器、windows/apple桌面，到安卓ndk，到stm32、arduino等各式嵌入式无操作系统环境都能支持，有些芯片的运行内存甚至可以低至1K大小。（[参考兼容性列表](#%e5%85%bc%e5%ae%b9%e6%80%a7)）

所有这些平台，全部共享一套同样接口的多任务[API](#api)。使用s_task，您将用最小的使用成本，获得最大的收益。

现在，暂时忘记多线程，开始您的 s_task 协程之旅！

## 示例

### [示例 1](examples/ex0_task.c) - 创建简单任务

```c
#include <stdio.h>
#include "s_task.h"

//定义协程任务需要的栈空间
void* g_stack_main[64 * 1024];
void* g_stack0[64 * 1024];
void* g_stack1[64 * 1024];

void sub_task(__async__, void* arg) {
    int i;
    int n = (int)(size_t)arg;
    for (i = 0; i < 5; ++i) {
        printf("task %d, delay seconds = %d, i = %d\n", n, n, i);
        s_task_msleep(__await__, n * 1000);  //等待一点时间
    }
}

void main_task(__async__, void* arg) {
    int i;

    //创建两个子任务
    s_task_create(g_stack0, sizeof(g_stack0), sub_task, (void*)1);
    s_task_create(g_stack1, sizeof(g_stack1), sub_task, (void*)2);

    for (i = 0; i < 4; ++i) {
        printf("task_main arg = %p, i = %d\n", arg, i);
        s_task_yield(__await__); //主动让出cpu
    }

    //等待子任务结束
    s_task_join(__await__, g_stack0);
    s_task_join(__await__, g_stack1);
}

int main(int argc, char* argv) {

    s_task_init_system();

    //创建一个任务
    s_task_create(g_stack_main, sizeof(g_stack_main), main_task, (void*)(size_t)argc);
    s_task_join(__await__, g_stack_main);
    printf("all task is over\n");
    return 0;
}
```

### [示例 2](examples/ex3_http_client.c) - （无需回调函数的）异步HTTP客户端程序

```c
void main_task(__async__, void *arg) {
    uv_loop_t* loop = (uv_loop_t*)arg;

    const char *HOST = "baidu.com";
    const unsigned short PORT = 80;

    //<1> 异步域名解析
    struct addrinfo* addr = s_uv_getaddrinfo(__await__,
        loop,
        HOST,
        NULL,
        NULL);
    if (addr == NULL) {
        fprintf(stderr, "can not resolve host %s\n", HOST);
        goto out0;
    }

    if (addr->ai_addr->sa_family == AF_INET) {
        struct sockaddr_in* sin = (struct sockaddr_in*)(addr->ai_addr);
        sin->sin_port = htons(PORT);
    }
    else if (addr->ai_addr->sa_family == AF_INET6) {
        struct sockaddr_in6* sin = (struct sockaddr_in6*)(addr->ai_addr);
        sin->sin6_port = htons(PORT);
    }

    //<2> 异步连接服务端
    uv_tcp_t tcp_client;
    int ret = uv_tcp_init(loop, &tcp_client);
    if (ret != 0)
        goto out1;
    ret = s_uv_tcp_connect(__await__, &tcp_client, addr->ai_addr);
    if (ret != 0)
        goto out2;

    //<3> 异步发送请求
    const char *request = "GET / HTTP/1.0\r\n\r\n";
    uv_stream_t* tcp_stream = (uv_stream_t*)&tcp_client;
    s_uv_write(__await__, tcp_stream, request, strlen(request));

    //<4> 异步读HTTP返回数据
    ssize_t nread;
    char buf[1024];
    while (true) {
        ret = s_uv_read(__await__, tcp_stream, buf, sizeof(buf), &nread);
        if (ret != 0) break;

        // 输出从HTTP服务器读到的数据
        fwrite(buf, 1, nread, stdout);
    }

    //<5> 关闭连接
out2:;
    s_uv_close(__await__, (uv_handle_t*)&tcp_client);
out1:;
    uv_freeaddrinfo(addr);
out0:;
}
```

### [示例 3](build/arduino/arduino.ino) - ardinuo下同时跑多个任务控制led闪烁

```c
#include <stdio.h>
#include "src/s_task/s_task.h"

//这个程序运行了三个任务
// 1) 任务 main_task -
//    等10秒并设置退出标志 g_exit，
//    在所有其他任务退出后，将LED设为常量。
// 2) 任务 sub_task_fast_blinking -
//    使 LED 快速闪烁
// 3) 任务 sub_task_set_low -
//    使上个任务中快速闪烁的LED，没快速闪烁3秒种，熄灭1秒种。

void setup() {
    // 初始化LED
    pinMode(LED_BUILTIN, OUTPUT);
}


//定义协程任务需要的栈空间
char g_stack0[384];
char g_stack1[384];
volatile bool g_is_low = false;
volatile bool g_exit = false;

void sub_task_fast_blinking(__async__, void* arg) {
    while(!g_exit) {
        if(!g_is_low)
            digitalWrite(LED_BUILTIN, HIGH); // 点亮LED

        s_task_msleep(__await__, 50);        // 等50毫秒
        digitalWrite(LED_BUILTIN, LOW);      // 熄灭LED
        s_task_msleep(__await__, 50);        // 等50毫秒
    }
}

void sub_task_set_low(__async__, void* arg) {
    while(!g_exit) {
        g_is_low = true;                     // 关闭LED快闪
        digitalWrite(LED_BUILTIN, LOW);      // 熄灭LED
        s_task_sleep(__await__, 1);          // 等1秒
        g_is_low = false;                    // 打开LED快闪
        s_task_sleep(__await__, 3);          // 等3秒
    }
}

void main_task(__async__, void* arg) {
    // 创建两个任务
    s_task_create(g_stack0, sizeof(g_stack0), sub_task_fast_blinking, NULL);
    s_task_create(g_stack1, sizeof(g_stack1), sub_task_set_low, NULL);

    // 等10秒
    s_task_sleep(__await__, 10);
    g_exit = true;

    // 等待两个任务结束
    s_task_join(__await__, g_stack0);
    s_task_join(__await__, g_stack1);
}

void loop() {

    s_task_init_system();
    main_task(__await__, NULL);

    // 使LED常亮
    digitalWrite(LED_BUILTIN, HIGH);
    while(1);
}
```

## 兼容性列表

"s_task" 可以作为一个单独的库使用，也可以配合libuv实现跨平台网络编程（编译时加上宏定义__USE_LIBUV__）。

| 平台                                     | coroutine协程      | libuv支持          |
|------------------------------------------|--------------------|--------------------|
| Windows                                  | :heavy_check_mark: | :heavy_check_mark: |
| Linux                                    | :heavy_check_mark: | :heavy_check_mark: |
| MacOS                                    | :heavy_check_mark: | :heavy_check_mark: |
| FreeBSD (12.1, x64)                      | :heavy_check_mark: | :heavy_check_mark: |
| Android                                  | :heavy_check_mark: | :heavy_check_mark: |
| MingW (<https://www.msys2.org/>)           | :heavy_check_mark: | :heavy_check_mark: |
| ARMv6-M (M051, 树莓派 Raspberry Pi Pico) | :heavy_check_mark: | :x:                |
| ARMv7-M (STM32F103, STM32F302)           | :heavy_check_mark: | :x:                |
| STM8 (STM8S103, STM8L051F3)              | :heavy_check_mark: | :x:                |
| riscv32 (GD32VF103)                      | :heavy_check_mark: | :x:                |
| Arduino UNO (AVR MEGA328P)               | :heavy_check_mark: | :x:                |
| Arduino DUE (ATSAM3X8E)                  | :heavy_check_mark: | :x:                |

   linux在以下硬件环境测试通过

- i686 (ubuntu-16.04)
- x86_64 (centos-8.1)
- arm (树莓派32位)
- aarch64 (① 树莓派64位, ② ubuntu 14.04 / centos7.6 运行于华为鲲鹏920)
- mipsel (openwrt ucLinux 3.10.14 for MT7628)
- mips64 (fedora for loongson 3A-4000 龙芯)
- ppc64 / ppc64le (centos-7.8.2003 altarch)
- riscv64 ([jslinux](https://bellard.org/jslinux/vm.html?cpu=riscv64&url=buildroot-riscv64.cfg&mem=256))

## 编译

### Posix - Linux / FreeBSD / MacOS / MingW(MSYS2)

    git clone https://github.com/xhawk18/s_task.git
    cd s_task/build/
    cmake .
    make

若您采用交叉编译器，请在上述运行 "cmake ." 指令时，加上参数 CMAKE_C_COMPILER 指定您所使用的交叉编译器，例如 --

    cmake . -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc

### 其他 - Windows / STM8 / Cortex-M / Arduino, 和更多单片机

| 平台                       | 项目                                  | 工具链                                              |
|----------------------------|---------------------------------------|-----------------------------------------------------|
| Windows                    | build\windows\s_task.sln              | visual studio 2019                                  |
| Android                    | build\android\cross_build_arm*.sh     | android ndk 20, API level 21 (在termux测试)         |
| STM8S103                   | build\stm8s103\Project.eww            | IAR workbench for STM8                              |
| STM8L051F3                 | build\stm8l05x\Project.eww            | IAR workbench for STM8                              |
| STM32F103                  | build\stm32f103\arcc\Project.uvproj   | Keil uVision5                                       |
| STM32F103                  | build\stm32f103\gcc\Project.uvproj    | arm-none-eabi-gcc                                   |
| STM32F302                  | build\stm32f302\Project.uvporj        | Keil uVision5                                       |
| M051                       | build\m051\Project.uvporj             | Keil uVision5                                       |
| 树莓派 Raspberry Pi Pico   | build\raspberrypi_pico\CMakeLists.txt | [pico-sdk](https://github.com/raspberrypi/pico-sdk) |
| GD32VF103                  | build\gd32vf103\                      | VSCode + [PlatformIO](https://platformio.org/)      |
| ATmega328P                 | build\atmega328p\atmega328p.atsln     | Atmel Studio 7.0                                    |
| Arduino UNO<br>Arduino DUE | build\arduino\arduino.ino             | Arduino IDE                                         |

## 如何在您的项目中使用s_task？

在 linux/unix 等环境里，可以先用cmake编译，编译完成后，将产生可以直接用于您的项目的链接库文件，您可以通过以下简单3步使用s_task --

- 将 libs_task.a 加入到您的项目
- #include "[s_task.h](include/s_task.h)"
- 编译时加上宏定义 USE_LIBUV

在 arduino 上使用，可以复制目录 "[include](include)" 和 "[src](src)" 下的所有*.h,*.c文件到您的项目的 src/s_task目录下。这里有个实际的目录结果可供参考："[build/arduino/](build/arduino/)"。

在 windows 或其他平台，请用 build 目录下的项目作为项目模板和参考。

## API

### Task （任务）

```c
/*
 * Return values --
 * For all functions marked by __async__ and hava an int return value, will
 *     return 0 on waiting successfully,
 *     return -1 on waiting cancalled by s_task_cancel_wait() called by other task.
 */

/* Function type for task entrance */
typedef void(*s_task_fn_t)(__async__, void *arg);

/* System initialization (without USE_LIBUV defined) */
void s_task_init_system();

/* System initialization (with USE_LIBUV defined)  */
void s_task_init_uv_system(uv_loop_t *loop);

/* Create a new task */
void s_task_create(void *stack, size_t stack_size, s_task_fn_t entry, void *arg);

/* Wait a task to exit */
int s_task_join(__async__, void *stack);

/* Sleep in milliseconds */
int s_task_msleep(__async__, uint32_t msec);

/* Sleep in seconds */
int s_task_sleep(__async__, uint32_t sec);

/* Yield current task */
void s_task_yield(__async__);

/* Cancel task waiting and make it running */
void s_task_cancel_wait(void* stack);
```

### Chan （数据通道）

```c
/*
 * macro: Declare the chan variable
 *    name: name of the chan
 *    TYPE: type of element in the chan
 *    count: max count of element buffer in the chan
 */
s_chan_declare(name,TYPE,count);

/*
 * macro: Initialize the chan (parameters same as what's in s_declare_chan).
 * To make a chan, we need to use "s_chan_declare" and then call "s_chan_init".
 */
s_chan_init(name,TYPE,count);

/*
 * Put element into chan
 *  return 0 on chan put successfully
 *  return -1 on chan cancelled
 */
int s_chan_put(__async__, s_chan_t *chan, const void *in_object);

/*
 * Put number of elements into chan
 *  return 0 on chan put successfully
 *  return -1 on chan cancelled
 */
int s_chan_put_n(__async__, s_chan_t *chan, const void *in_object, uint16_t number);

/*
 * Get element from chan
 *  return 0 on chan get successfully
 *  return -1 on chan cancelled
 */
int s_chan_get(__async__, s_chan_t *chan, void *out_object);

/*
 * Get number of elements from chan
 *  return 0 on chan get successfully
 *  return -1 on chan cancelled
 */
int s_chan_get_n(__async__, s_chan_t *chan, void *out_object, uint16_t number);
```

### Mutex （互斥量）

```c
/* Initialize a mutex */
void s_mutex_init(s_mutex_t *mutex);

/* Lock the mutex */
int s_mutex_lock(__async__, s_mutex_t *mutex);

/* Unlock the mutex */
void s_mutex_unlock(s_mutex_t *mutex);
```

### Event （事件）

```c
/* Initialize a wait event */
void s_event_init(s_event_t *event);

/* Wait event */
int s_event_wait(__async__, s_event_t *event);

/* Set event */
void s_event_set(s_event_t *event);

/* Wait event with timeout */
int s_event_wait_msec(__async__, s_event_t *event, uint32_t msec);

/* Wait event with timeout */
int s_event_wait_sec(__async__, s_event_t *event, uint32_t sec);
```

## 嵌入式平台

<details>
  <summary>嵌入式平台特殊API</summary>

### Chan for interrupt (中断和任务的数据通道，仅嵌入式平台支持，STM8/STM32/M051/Arduino)

#### 任务里调用的 chan api

```c
/* Task puts element into chan and waits interrupt to read the chan */
void s_chan_put__to_irq(__async__, s_chan_t *chan, const void *in_object);

/* Task puts number of elements into chan and waits interrupt to read the chan */
void s_chan_put_n__to_irq(__async__, s_chan_t *chan, const void *in_object, uint16_t number);

/* Task waits interrupt to write the chan and then gets element from chan */
void s_chan_get__from_irq(__async__, s_chan_t *chan, void *out_object);

/* Task waits interrupt to write the chan and then gets number of elements from chan */
void s_chan_get_n__from_irq(__async__, s_chan_t *chan, void *out_object, uint16_t number);
```

#### 中断里调用 chan api

```c
/*
 * Interrupt writes element into the chan,
 * return number of element was written into chan
 */
uint16_t s_chan_put__in_irq(s_chan_t *chan, const void *in_object);

/*
 * Interrupt writes number of elements into the chan,
 * return number of element was written into chan
 */
uint16_t s_chan_put_n__in_irq(s_chan_t *chan, const void *in_object, uint16_t number);

/*
 * Interrupt reads element from chan,
 * return number of element was read from chan
 */
uint16_t s_chan_get__in_irq(s_chan_t *chan, void *out_object);

/*
 * Interrupt reads number of elements from chan,
 * return number of element was read from chan
 */
uint16_t s_chan_get_n__in_irq(s_chan_t *chan, void *out_object, uint16_t number);
```

### Event for interrupt (中断里的事件，仅嵌入式平台支持，STM8/STM32/M051/Arduino)

#### 任务里调用的 event api

```c
/*
 * Wait event from irq, disable irq before call this function!
 *   S_IRQ_DISABLE()
 *   ...
 *   s_event_wait__from_irq(...)
 *   ...
 *   S_IRQ_ENABLE()
 */
int s_event_wait__from_irq(__async__, s_event_t *event);

/*
 * Wait event from irq, disable irq before call this function!
 *   S_IRQ_DISABLE()
 *   ...
 *   s_event_wait_msec__from_irq(...)
 *   ...
 *   S_IRQ_ENABLE()
 */
int s_event_wait_msec__from_irq(__async__, s_event_t *event, uint32_t msec);

/*
 * Wait event from irq, disable irq before call this function!
 *   S_IRQ_DISABLE()
 *   ...
 *   s_event_wait_sec__from_irq(...)
 *   ...
 *   S_IRQ_ENABLE()
 */
int s_event_wait_sec__from_irq(__async__, s_event_t *event, uint32_t sec);
```

#### 中断里调用的 event api

```c
/* Set event in interrupt */
void s_event_set__in_irq(s_event_t *event);
```

</details>

<details>
  <summary>低功耗运行模式</summary>

如果my_on_idle函数为空，当没有任务运行时，程序将进入忙等待模式，这样通常表现为CPU占据了100%的时间。
为避免此问题，可实现适当的 my_on_idle 函数，以便程序可以低功耗运行。

目前在Windows/Linux/MacOS/Android等平台上，已经实现低功耗运行模式。

在无操作系统的嵌入式环境下，可能并未做低功耗支持（请检查对应平台的my_on_idle函数）。
如果您希望自己优化芯片运行功耗，可在 my_on_idle 函数加入使芯片睡眠一段时间的代码，睡眠时间最长为 max_idle_ms 毫秒。

```
void my_on_idle(uint64_t max_idle_ms) {
    /* 增加使CPU睡眠代码，最长不超过  max_idle_ms 毫秒 */
}
```

</details>

## 希望移植到新的平台？

[移植文档参考此处](porting.md)

## 联系方式

使用中有任何问题或建议，欢迎QQ加群 567780316 交流。

![s_task交流群](qq.png)

## 其他协程库对比

- coro: <http://www.goron.de/~froese/coro/>
- coroutine(a asymmetric coroutine library for C): <https://github.com/cloudwu/coroutine>
- coroutine(a asymmetric coroutine (lua like) with fixed-size stack): <https://github.com/xphh/coroutine>
- coroutine(coroutine library with pthread-like interface in pure C): <https://github.com/Marcus366/coroutine>
- coroutines(A lightweight coroutine library written in C and assembler): <https://github.com/xya/coroutines>
- fcontext: <https://github.com/reginaldl/fcontext>
- hev-task-system: <https://github.com/heiher/hev-task-system>
- libaco: <https://github.com/hnes/libaco>
- libconcurrency: <http://code.google.com/p/libconcurrency/>
- libconcurrent: <https://github.com/sharow/libconcurrent>
- libcoro: <http://software.schmorp.de/pkg/libcoro.html>
- libcoroutine: <https://github.com/stevedekorte/coroutine>
- libfiber: <http://www.rkeene.org/projects/info/wiki/22>
- libtask: <https://code.google.com/p/libtask/>
- libwire: <https://github.com/baruch/libwire>
- micro: <https://github.com/mikewei/micoro>
- mill: <https://github.com/sustrik/mill>
- Portable Coroutine Library (PCL): <http://xmailserver.org/libpcl.html>

## 感谢

[wooley](https://github.com/wooley) 关于cmake+vc编译的修正

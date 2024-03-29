# Scheduler 多功能时分调度器

Author：[Ellu(艾琉)](https://github.com/ElluIFX)

## 1. 简介 📖

Scheduler是一个多功能的时分调度器，它可以在裸机环境下实现按优先级定频任务调度（高精度定时器）、异步事件回调（事件驱动编程）、异步协程、异步软中断、延时调用等功能。

本文档将介绍Scheduler的API接口。

## 2. 先决条件 📋

整个Scheduler的实现依赖于高精度的时钟，因此需要实现文件[`scheduler_internal.h`](scheduler_internal.h)中的`get_sys_tick()`函数和`get_sys_freq()`函数，分别用于获取当前系统时钟计数和系统时钟频率(Hz)。

> [!TIP]
> 在绝大部分场景下，推荐使用[`perf_counter`](https://github.com/GorgonMeducer/perf_counter)组件来实现时基，这也是Release中的默认实现。

## 3.注意事项 ⚠️

在本框架下，所有的任务、事件、协程对象都有一个独立的字符串（`const char*`）作为标识符（任务名），API会根据提供的字符串来查找对应的对象，因此**每个对象的标识符唯一且不可更改**。

本框架采用`UList`模块作为任务列表存储区，因此需要实现动态内存分配。

## 4. 配置宏定义 🛠

```C
#define SCH_CFG_ENABLE_TASK 1       // 支持任务
#define SCH_CFG_ENABLE_EVENT 1      // 支持事件
#define SCH_CFG_ENABLE_COROUTINE 1  // 支持宏协程
#define SCH_CFG_ENABLE_CALLLATER 1  // 支持延时调用
#define SCH_CFG_ENABLE_SOFTINT 1    // 支持软中断

#define SCH_CFG_COMP_RANGE_US 1000  // 任务调度自动补偿范围(us)
#define SCH_CFG_STATIC_NAME 1       // 是否使用静态标识名
#define SCH_CFG_STATIC_NAME_LEN 16  // 静态标识名长度
#define SCH_CFG_PRI_ORDER_ASC 1  // 优先级升序排序(升序:值大的优先级高)

#define SCH_CFG_DEBUG_REPORT 1  // 输出调度器统计信息(调试模式/低性能)
#define SCH_CFG_DEBUG_PERIOD 5  // 调试报告打印周期(s)(超过10s的值可能导致溢出)
#define SCH_CFG_DEBUG_MAXLINE 10  // 调试报告最大行数

#define SCH_CFG_ENABLE_TERMINAL 1  // 是否启用终端命令集(依赖embedded-cli)
```

+ `SCH_CFG_ENABLE_*`：是否编译对应子模块
+ `SCH_CFG_COMP_RANGE_US`：任务调度自动补偿范围，当任务调度的延时小于此值时，调度器会自动补偿延时，以保证调度频率符合设定值，大于此值说明任务耗时与设定频率不匹配，可以通过统计信息查看。
+ `SCH_CFG_DEBUG_*`：调试相关宏定义，启用时会每隔一段时间在串口终端上打印任务、事件、协程相关的统计信息，信息中时间相关的单位均为`us`，占用率单位为`%`，调试模式下会降低调度器性能，仅用于排查问题。
+ `SCH_CFG_STATIC_NAME`: 是否使用静态标识名，启用时会为每个对象分配固定长度的字符串缓冲区，关闭时对象的标识名将直接指向用户提供的字符串指针以最小化占用，此时用户需要保证字符串为不变的全局常量。
+ `SCH_CFG_ENABLE_TERMINAL`：是否启用终端命令集，启用时可在`embedded-cli`中注册调度器相关控制命令，用于调试。

> [!TIP]
> 调试模式非常有用，它类似于PC的任务管理器，可以通过调试信息来判断任务函数的执行效率，了解系统瓶颈并针对性优化。
<!--  -->
> [!WARNING]
> 在启用RTOS后，如果线程在某个任务运行时被切换，会导致该任务的统计占用上涨，可以通过给予调度器所在线程较高的优先级来避免此问题。

## 5. API接口 📑

### 5.1. 主文件 ([`scheduler.h`](scheduler.h))

> 包含scheduler.h时会自动包含所有子模块的头文件。

```C
uint64_t scheduler_run(const uint8_t block)
```

+ 功能：整个调度器的主入口，所有的子模块都在此处实际执行目标函数。
+ 返回：距离下一次调度的时间(us)，CPU应在此时间前再次调用此函数以最小化调度延迟。
+ 参数：
  + `block`：是否阻塞：
    + `0`：不阻塞，执行完所有的任务后立即返回。
    + `1`：阻塞，内部建立SuperLoop，空闲时会调用`scheduler_idle_handler()`函数。
+ 限制：需确保该函数所在线程的栈空间充足。

```C
weak void scheduler_idle_handler(uint64_t idleTimeUs)
```

+ 功能：空闲回调函数，当`scheduler_run()`函数的`block`参数为`1`时，当调度器空闲时会调用此函数。
+ 参数：
  + `idleTimeUs`：距离下一次调度的时间(us)，函数应在此时间内返回。
+ 注意：弱函数，用户可以在自己的代码中重写此函数并实现低功耗等逻辑。

```C
void sch_add_command_to_cli(EmbeddedCli *cli)
```

+ 功能：将调度器相关的命令集添加到`embedded-cli`中。
+ 参数：
  + `cli`：`embedded-cli`对象指针。
+ 添加的命令集：
  + `task`：任务相关命令集，可对任务列表进行增删查改。
  + `event`：事件相关命令集，可对事件列表进行增删和手动触发。
  + `cortn`：协程相关命令集，可对协程列表进行增删查改。
  + `softint`：软中断相关命令集，可对软中断进行手动触发。
+ 注意：仅当`SCH_CFG_ENABLE_TERMINAL`宏定义为`1`时有效。

### 5.2. 任务 ([`scheduler_task.h`](scheduler_task.h))

任务可以被理解为一个高精度的软件定时器，它可以在调度器中以指定的频率调用一个函数，且可以在运行时动态修改调度频率、优先级、启用状态等参数。

```C
uint8_t sch_task_create(const char *name, sch_func_t func, float freqHz, uint8_t enable, uint8_t priority, void *args)
```

+ 功能：创建一个任务。
+ 返回：1：成功，0：失败（内存操作出错）。
+ 参数：
  + `name`：任务名，**不可重复**。
  + `func`：任务函数指针，返回为`void`，参数为`void*`。
  + `freqHz`：任务调度频率(Hz)，>0。
  + `enable`：创建后是否启用，0：不启用，1：启用。
  + `priority`：任务优先级，0：最低，255：最高。
  + `args`：任务参数，会传递给任务函数。

```C
uint8_t sch_task_delete(const char *name)
```

+ 功能：删除任务。
+ 返回：1：成功，0：失败（未找到任务）。

```C
uint8_t sch_task_get_exist(const char *name)
```

+ 功能：判断任务是否存在。
+ 返回：0：不存在，1：存在。

```C
uint16_t sch_task_get_num(void)
```

+ 功能：获取任务数量。
+ 返回：任务数量。

```C
uint8_t sch_task_set_enabled(const char *name, uint8_t enable)
```

+ 功能：设置任务的启用状态。
+ 返回：1：成功，0：失败（未找到任务）。
+ 参数：
  + `name`：任务名。
  + `enable`：0：不启用，1：启用。

```C
uint8_t sch_task_get_enabled(const char *name)
```

+ 功能：获取任务的启用状态。
+ 返回：0：未启用或未找到任务，1：启用。

```C
uint8_t sch_task_set_freq(const char *name, float freqHz)
```

+ 功能：设置任务的调度频率。
+ 返回：1：成功，0：失败（未找到任务）。
+ 参数：
  + `name`：任务名。
  + `freqHz`：任务调度频率(Hz)，>0。

```C
uint8_t sch_task_set_priority(const char *name, uint8_t priority)
```

+ 功能：设置任务的优先级。
+ 返回：1：成功，0：失败（未找到任务）。
+ 参数：
  + `name`：任务名。
  + `priority`：任务优先级，0：最低，255：最高。

```C
uint8_t sch_task_set_args(const char *name, void *args)
```

+ 功能：设置任务的参数。
+ 返回：1：成功，0：失败（未找到任务）。
+ 参数：
  + `name`：任务名。
  + `args`：任务参数，会传递给任务函数。

```C
uint8_t sch_task_delay(const char *name, uint64_t delay_us,
                             uint8_t fromNow)
```

+ 功能：推迟任务的下一次调度。
+ 返回：1：成功，0：失败（未找到任务）。
+ 参数：
  + `name`：任务名。
  + `delay_us`：推迟的时间(us)。
  + `fromNow`：1：从当前时间开始计算，0：从上一次调度时间开始计算。

### 5.3. 事件 ([`scheduler_event.h`](scheduler_event.h))

事件是一种异步回调机制，通过注册一个统一的事件回调函数，可以实现调用方与功能实现的解耦，且异步执行保证了函数不会在调用方的上下文中执行，从而避免了调用方的上下文被破坏。

```C
uint8_t sch_event_create(const char *name, sch_func_t callback,
                               uint8_t enable)
```

+ 功能：创建一个事件。
+ 返回：1：成功，0：失败（内存操作出错）。
+ 参数：
  + `name`：事件名，**不可重复**。
  + `callback`：事件回调函数指针，返回为`void`，参数为`void*`。
  + `enable`：创建后是否启用，0：不启用，1：启用。

```C
uint8_t sch_event_delete(const char *name)
```

+ 功能：删除事件。
+ 返回：1：成功，0：失败（未找到事件）。

```C
uint8_t sch_event_get_exist(const char *name)
```

+ 功能：判断事件是否存在。
+ 返回：0：不存在，1：存在。

```C
uint16_t sch_event_get_num(void)
```

+ 功能：获取事件数量。
+ 返回：事件数量。

```C
uint8_t sch_event_set_enabled(const char *name, uint8_t enable)
```

+ 功能：设置事件的启用状态。
+ 返回：1：成功，0：失败（未找到事件）。
+ 参数：
  + `name`：事件名。
  + `enable`：0：不启用，1：启用。

```C
uint8_t sch_event_get_enabled(const char *name)
```

+ 功能：获取事件的启用状态。
+ 返回：0：未启用或未找到事件，1：启用。

```C
uint8_t sch_event_trigger(const char *name, void *args)
```

+ 功能：触发事件。
+ 返回：1：成功，0：失败（未找到事件或事件禁用）。
+ 参数：
  + `name`：事件名。
  + `args`：事件参数，会传递给事件回调函数。
+ 注意：事件是异步执行，必须注意所传递的参数的生命周期，禁止传递临时数据指针。

```C
uint8_t sch_event_trigger_ex(const char *name, const void *arg_ptr, uint16_t arg_size)
```

+ 功能：触发事件并为参数创建临时拷贝。
+ 返回：1：成功，0：失败（未找到事件或事件禁用）。
+ 参数：
  + `name`：事件名。
  + `arg_ptr`：事件参数指针，会被拷贝到临时缓冲区。
  + `arg_size`：事件参数大小，单位为字节。
+ 备注：拷贝的内存会在回调函数执行完毕后由调度器自动释放。

### 5.4. 协程 ([`scheduler_coroutine.h`](scheduler_coroutine.h))

#### 5.4.1. 介绍

本模块协程依赖goto和宏实现，并且具备管理局部变量缓冲区的能力。

首先，介绍如何定义一个协程：

```C
void coroutine_main(__async__, void *args) // __async__宏必须在函数声明中第一个参数的位置
{
    // 声明一个无局部变量协程
    ASYNC_NOLOCAL // 此宏必须在函数内部第一行

    // 协程的代码
    while(1){
        printf("Hello World!\r\n");
        AWAIT_DELAY(1000);
    }
}
```

上述代码创建了一个协程的`主函数`，协程的`主函数`返回值必须是`void`类型，参数为`__async__`和`void*`，该协程未用到局部变量，下面介绍如何使用局部变量：

```C
void coroutine_main(__async__, void *args)
{
    // 声明为有局部变量协程
    ASYNC_LOCAL_START // 此宏必须在函数内部第一行
    // 声明局部变量（不允许赋值，一律初始化为0）
    uint8_t i;
    struct{
        uint8_t a;
        uint8_t b;
    }s;
    // 局部变量声明结束
    ASYNC_LOCAL_END // 此宏在声明结束完毕的下一行

    LOCAL(i) = 23; // 可以在这里赋初始值
    uint8_t temp = 0; // 此变量不会被维护，每次函数重入时的值是未定义的

    // 协程的代码
    while(1){
        printf("Hello World! %d\r\n", LOCAL(i)++);
        printf("s: %d %d\r\n", LOCAL(s).a++, LOCAL(s).b++);
        AWAIT_DELAY(1000);
    }
}
```

该协程用到了局部变量，则使用`ASYNC_LOCAL_START`和`ASYNC_LOCAL_END`宏将局部变量的定义包裹起来，**所有的局部变量必须在这两个宏之间定义，且使用`LOCAL(var)`宏来访问局部变量**，除此以外定义的变量都是`临时变量`，他们会在任意一次`AWAIT_*`宏调用后被释放，下一次函数重入时的值是**未定义的**。

> [!TIP]
> 除了 `ASYNC_NOLOCAL` 和 `ASYNC_LOCAL_START` / `ASYNC_LOCAL_END` 是定义协程的专有宏外，其他的，以`ASYNC_`开头的宏在协程和正常函数中都可调用，他们是**非阻塞**的，而以`AWAIT_`开头的宏只能在协程函数中调用，是**阻塞**的。

#### 5.4.2. 宏API （一般在协程函数中调用）

下面介绍每个宏的作用

1. `ASYNC_NOLOCAL` **<调用无需分号结尾>**

    + 功能：声明为无局部变量协程。
    + 是 `ASYNC_INIT` 的别名，可以用后者替代。

2. `ASYNC_LOCAL_START` / `ASYNC_LOCAL_END` **<调用无需分号结尾>**

    + 功能：声明为有局部变量协程。
    + 注意：二者需成对使用。

3. `LOCAL(var)`

    + 功能：访问被维护的局部变量。
    + 参数：
      + `var`：局部变量名。
    + 限制：仅用于访问由`ASYNC_LOCAL_START`和`ASYNC_LOCAL_END`包裹的局部变量，其他临时变量正常访问。

4. `YIELD()`

    + 功能：协程主动让出CPU。

5. `AWAIT(func_cmd, args...)`

    + 功能：阻塞等待另一个`协程子函数`执行完毕。
    + 参数：
      + `func_cmd`：协程子函数
      + `args...`：协程子函数的参数

    > [!TIP]
    >AWAIT宏可以类比Python中的`await`关键字，用于等待另一个`协程子函数`执行完毕，`func_cmd`为协程子函数的调用命令
    <!--  -->
    > [!IMPORTANT]
    >`协程子函数` 与 `协程主函数` 不同，除了__async__外的其他参数可以为任意类型和数量，但仍然必须返回`void`，因此数据的传入传出都需要通过参数指针来实现。下面给出一个例子：

    ```C
    void receive_array(__async__, uint8_t *buf, uint16_t len)
    {
        ASYNC_LOCAL_START
        uint16_t i;
        ASYNC_LOCAL_END
        XXXAcquireTransfer();
        while (LOCAL(i) < len) {
          while (!XXXTransferDataReady()) {
              AWAIT_DELAY(1);
          }
          buf[LOCAL(i)++] = XXXGetTransferData();
        }
        XXXReleaseTransfer();
    }

    void coroutine_main(__async__, void *args) {
      ASYNC_LOCAL_START
      uint8_t buf[32];
      ASYNC_LOCAL_END

      while (1) {
        AWAIT(receive_array, LOCAL(buf), 32);
        printf("Recieved: %s\r\n", LOCAL(buf));
      }
    }
    ```

    > [!NOTE]
    > `协程子函数`可以无限嵌套调用更多的`协程子函数`，`协程主函数`也可以是一种单参数的`协程子函数`。

6. `AWAIT_DELAY(ms)` / `AWAIT_DELAY_US(us)`

    + 功能：阻塞等待一段时间。
    + 参数：
      + `ms`：等待时间(ms)。
      + `us`：等待时间(us)。

    > [!TIP]
    > AWAIT_DELAY宏可以类比Python中的`await asyncio.sleep()`，用于等待一段时间。

7. `AWAIT_YIELD_UNTIL(cond)`

    + 功能：阻塞等待条件满足。
    + 参数：
      + `cond`：条件表达式，为真时跳出阻塞。

    > [!WARNING]
    > 由于每次调度周期都需要重入函数以检查条件，该宏占用较大，除非要求极高的实时性，否则应优先使用下述的`AWAIT_DELAY_UNTIL`宏。

8. `AWAIT_DELAY_UNTIL(cond, delayMs)`

    + 功能：阻塞等待条件满足, 每隔delayMs检查一次。
    + 参数：
      + `cond`：条件表达式，为真时跳出阻塞。
      + `delayMs`：检查间隔(ms)。

9. `ASYNC_SELF_NAME()`

    + 功能：获取当前`主协程`的名字。
    + 注意：在子协程中调用返回的是调用该子协程的最上层`主协程`的名字，`子协程`对象不存在名字。
    + 彩蛋：协程外调用此宏会返回 `__main__`。

10. `AWAIT_RECV_MSG(to_ptr)`

    + 功能：阻塞等待消息。
    + 参数：
      + `to_ptr`：消息指针，当函数返回时，消息指针会被赋值。
    + 注意：调用时，参数的指针不需要写取址符。

11. `ASYNC_SEND_MSG(name, msg)`

    + 功能：发送消息给指定协程，立即返回。
    + 参数：
      + `name`：协程名。
      + `msg`：消息指针。
    + 等价：`sch_cortn_send_msg_to`

12. `AWAIT_ACQUIRE_MUTEX(mutex_name)`

    + 功能：阻塞等待互斥锁。
    + 参数：
      + `mutex_name`：互斥锁名。

    > [!NOTE]
    > 由于协程是非抢占的，在大部分代码如数据访问中，实际上不需要使用互斥锁。但在某些特殊场景下，如需要对外设进行访问，且访问代码中包含AWAIT/YIELD，此时就需要使用互斥锁来保证同时只有一个协程访问外设。

13. `ASYNC_RELEASE_MUTEX(mutex_name)`

    + 功能：释放互斥锁，立即返回。
    + 参数：
      + `mutex_name`：互斥锁名。
    + 警告：释放互斥锁时不会检查操作者是否是锁的所有者，允许强制释放。

14. `AWAIT_BARRIER(barr_name)`

    + 功能：阻塞等待屏障。
    + 参数：
      + `barr_name`：屏障名。

    > [!NOTE]
    > 屏障是一种同步机制，它可以让多个协程在某个点上同步，当到达屏障点的协程个数达到目标时，所有协程同时被唤醒。
    >
    > 屏障刚建立时目标值为0xffff，调用`sch_cortn_set_barrier_target`来修改目标值，当目标值为0时，屏障失效。

15. `ASYNC_RELEASE_BARRIER(barr_name)`

    + 功能：手动释放屏障，立即返回。
    + 参数：
      + `barr_name`：屏障名。
    + 等价：`sch_cortn_release_barrier`

16. `ASYNC_SET_BARRIER_TARGET(barr_name, target)`

    + 功能：设置屏障目标值。
    + 参数：
      + `barr_name`：屏障名。
      + `target`：目标值。
    + 等价：`sch_cortn_set_barrier_target`

17. `ASYNC_RUN(name, func, args)`

    + 功能：创建并异步运行一个协程，立即返回。
    + 参数：
      + `name`：协程名, **不可重复**。
      + `func`：协程函数指针，必须是`协程主函数`。
      + `args`：协程参数指针。
    + 等价：`sch_cortn_run`

18. `AWAIT_JOIN(name)`

    + 功能：阻塞等待一个协程结束。
    + 参数：
      + `name`：协程名。

#### 5.4.3. 函数API （一般在正常函数中调用）

```C
uint8_t sch_cortn_run(const char *name, cortn_func_t func, void *args)
```

+ 功能：运行一个协程。
+ 返回：1：成功，0：失败（内存操作出错）。
+ 参数：
  + `name`：协程名，**不可重复**。
  + `func`：协程函数指针，必须是`协程主函数`。
  + `args`：协程参数指针。

```C
uint8_t sch_cortn_stop(const char *name)
```

+ 功能：停止一个协程。
+ 返回：1：成功，0：失败（未找到协程）。
+ 限制：不允许在任何协程中停止自身，这种情况下请直接return。

```C
uint8_t sch_cortn_get_running(const char *name)
```

+ 功能：查询指定协程是否正在运行
+ 返回：0：未运行，1：正在运行。

```C
uint16_t sch_cortn_get_num(void)
```

+ 功能：获取协程数量。
+ 返回：协程数量。

```C
uint8_t sch_cortn_get_waiting_msg(const char *name)
```

+ 功能：判断协程是否正在等待消息。
+ 返回：0：不在等待，1：正在等待。

```C
uint8_t sch_cortn_send_msg_to(const char *name, void *msg)
```

+ 功能：发送消息给指定协程并唤醒。
+ 返回：1：成功，0：失败（未找到协程）。
+ 参数：
  + `name`：协程名。
  + `msg`：消息指针。
+ 警告: 该函数是异步的，需要注意消息的生命周期，禁止传递临时数据指针。

```C
uint8_t sch_cortn_release_barrier(const char *name)
```

+ 功能：手动释放协程屏障。
+ 返回：1：成功，0：失败（屏障未建立）。

```C
uint16_t sch_cortn_get_barrier_num(const char *name)
```

+ 功能：获取协程屏障等待数量。
+ 返回：等待数量。

```C
uint8_t sch_cortn_set_barrier_target(const char *name, uint16_t target)
```

+ 功能：设置协程屏障目标值。
+ 返回：1：成功，0：失败。
+ 参数：
  + `name`：协程名。
  + `target`：目标值。
+ 注意：当目标值为0时，屏障失效。

### 5.5. 延时调用 ([`scheduler_runlater.h`](scheduler_runlater.h))

延时调用可以用于实现延时关机之类的低频率功能，不要高频率地使用。

```C
uint8_t sch_runlater(Any func, uint64_t delay_us, ...)
```

+ 功能：延时调用一个函数。
+ 返回：1：成功，0：失败（内存操作出错）。
+ 参数：
  + `func`：函数指针，不限制返回值和参数。
  + `delay_us`：延时时间(us)。
  + `args`：函数参数，需要与函数声明匹配。
+ 注意1：所有参数必须都是变量(为了判断类型), 常量参数需要先赋值给变量再传递。
+ 注意2：该函数是异步的，需要注意参数的生命周期，禁止传递临时数据指针。
+ 注意3：暂不支持浮点数, 但可以通过指针传递(注意生命周期)。

> [!TIP]
> 小示例

```C
void func(uint8_t a, uint16_t b, uint32_t c, float *d)
{
    printf("a: %d, b: %d, c: %d, d: %f\r\n", a, b, c, *d);
}

void main(void){
    uint8_t a = 1;
    uint16_t b = 2;
    uint32_t c = 3;
    float df = 4.0f;
    float *d = &df;
    sch_runlater(func, 1000000, a, b, c, d);
}
```

```C
void sch_runlater_cancel(Any func)
```

+ 功能：取消对指定函数的所有延时调用。
+ 参数：
  + `func`：函数指针，不限制返回值和参数。

### 5.6. 软中断 ([`scheduler_softint.h`](scheduler_softint.h))

软中断可用于将硬件中断中的调用延迟到调度器中执行，以避免中断嵌套。相比其他方法，软中断无内存操作，可以实现极高频的操作，但存在通道数量限制。

```C
void sch_softint_trigger(uint8_t main_channel, uint8_t sub_channel)
```

+ 功能：触发软中断。
+ 参数：
  + `main_channel`：主通道号，0~7。
  + `sub_channel`：子通道号，0~7。

```C
weak void scheduler_softint_handler(uint8_t main_channel, uint8_t sub_channel_mask)
```

+ 功能：软中断处理函数，由用户实现。
+ 参数：
  + `main_channel`：主通道号，0~7。
  + `sub_channel_mask`：子通道掩码，每一位代表一个子通道(1 << sub_channel)，1：触发，0：未触发。
+ 注意：弱函数，用户根据需要自行定义此函数。

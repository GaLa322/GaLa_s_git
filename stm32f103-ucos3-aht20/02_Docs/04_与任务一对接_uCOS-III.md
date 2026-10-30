# 与任务一对接：把"任务壳"换成真正的 uC/OS-III

> 这份文档解决一个现实矛盾：
> **老师要求"在任务一（uC/OS 多任务）的基础上完成任务二第 3 小问"，但你想先做任务二。**
> 做法是：先写一个 **API 长得和 uC/OS 一样**的极简任务壳（`osal.c`），
> 三个任务的任务体、周期、打印内容全部就位；等你回头做任务一时，
> **只把 `OSAL_*` 换成 `OS*`，任务体一行不用改**。

---

## 1. 现在的"任务壳"长什么样

`Src/osal.c` 只有 108 行，核心是一个"时间片轮转 + 固定节拍"的协作式调度器：

```c
/* Inc/osal.h */
typedef void (*OSAL_TaskFn)(void *arg);

int8_t  OSAL_TaskCreate(const char *name, OSAL_TaskFn fn, void *arg, uint32_t period_ms);
void    OSAL_Start(void);        /* 不返回 */
uint32_t OSAL_Now(void);         /* 毫秒计数 */
uint8_t OSAL_TaskCount(void);
uint32_t OSAL_TaskRuns(uint8_t idx);
```

调用方式：

```c
OSAL_TaskCreate("LED1",  Task_Led1,  0, 1000);
OSAL_TaskCreate("LED2",  Task_Led2,  0, 3000);
OSAL_TaskCreate("AHT20", Task_Aht20, 0, 10);    /* 10 ms 轮询，内部 2 s 采样 */
OSAL_Start();
```

### 它和 uC/OS-III 的关系

| 维度 | 现在的 OSAL | 真正的 uC/OS-III |
| --- | --- | --- |
| 调度方式 | 协作式轮转，任务必须自己"让出" | 抢占式，按优先级 |
| 任务切换 | 任务函数返回后回主循环 | PendSV 异常，保存/恢复上下文 |
| 延时 | 任务返回即"睡"到下一个周期 | `OSTimeDly()` 主动阻塞 |
| 栈 | 共用主栈 | 每个任务独立栈 |
| 内核对象 | 无 | 信号量/互斥量/消息队列/事件标志 |
| 代码量 | 108 行 | 约 2 万行 |

**所以：它只是"长得像"，绝不是 uC/OS。报告里要如实写清楚这一点**（写"过渡内核/任务框架"，
不要写"已移植 uC/OS"）。这样做的好处是：任务逻辑、打印格式、验证方法全部可以原样复用。

---

## 2. API 对照表（迁移时照这张表替换）

| 现在（OSAL） | uC/OS-III | 说明 |
| --- | --- | --- |
| `OSAL_TaskCreate(name, fn, arg, period)` | `OSTaskCreate(&tcb, name, fn, arg, prio, &stk[0], stk_size/10, 0, 0, 0, &err)` | uC/OS 要自己给 TCB、栈、优先级 |
| `OSAL_Start()` | `OSStart(&err)` | 启动调度器，不返回 |
| `OSAL_Now()` | `OSTimeGet(&err)` 或 `OSTimeTickHook` 里的计数 | 单位是 tick，`OS_CFG_TICK_RATE_HZ=1000` 时 1 tick = 1 ms |
| 任务函数返回 = 睡一个周期 | `OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err)` | 1 s；或 `OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &err)` |
| `OSAL_TaskRuns(i)` | `OSTaskCtrGet` 等统计（需开 `OS_CFG_STAT_EN`） | 调试用 |
| `HAL_GetTick()` 打时间戳 | `OSTimeGet(&err)` | 见 5.3 的坑 |

---

## 3. 迁移后的 `app_tasks.c` 长什么样

下面是**完整可用的 uC/OS-III 版本**（假设 `OS_CFG_TICK_RATE_HZ = 1000`）。
和现在的版本对比，你会发现**任务体几乎一模一样**，只有创建和延时两处变了。

```c
#include "app_tasks.h"
#include "board_config.h"
#include "bsp_uart.h"
#include "aht20.h"
#include "gpio.h"
#include "main.h"

#include "os.h"          /* uC/OS-III */
#include <stdio.h>

/* ---- 任务栈与 TCB ------------------------------------------------------ */
#define LED1_STK_SIZE    128
#define LED2_STK_SIZE    128
#define AHT_STK_SIZE     512      /* 有 printf/vsnprintf，栈要大 */

static CPU_STK s_led1_stk[LED1_STK_SIZE];
static CPU_STK s_led2_stk[LED2_STK_SIZE];
static CPU_STK s_aht_stk[AHT_STK_SIZE];

static OS_TCB  s_led1_tcb, s_led2_tcb, s_aht_tcb;

/* ---- 优先级（数字越小优先级越高）-------------------------------------- */
#define PRIO_LED1   6
#define PRIO_LED2   7
#define PRIO_AHT    8

/* ---- 毫秒时间戳 -------------------------------------------------------- */
static uint32_t NowMs(void)
{
    OS_ERR err;
    return (uint32_t)OSTimeGet(&err);      /* 1 tick = 1 ms */
}

/* ---- LED 任务 ---------------------------------------------------------- */
static void Task_Led1(void *p_arg)
{
    OS_ERR err;
    uint8_t on = 0;
    (void)p_arg;

    for (;;) {
        on = !on;
        Led_Write(BOARD_LED1_PORT, BOARD_LED1_PIN, BOARD_LED1_ON_LEVEL, on);
#if (APP_LOG_LED != 0)   /* 默认关：串口只留温湿度 */
        BSP_UART_Printf("[%8lu ms] TASK1  LED1 -> %s\r\n",
                        (unsigned long)NowMs(), on ? "ON" : "OFF");
#endif
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

static void Task_Led2(void *p_arg)
{
    OS_ERR err;
    uint8_t on = 0;
    (void)p_arg;

    for (;;) {
        on = !on;
        Led_Write(BOARD_LED2_PORT, BOARD_LED2_PIN, BOARD_LED2_ON_LEVEL, on);
        OSTimeDlyHMSM(0, 0, 3, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

/* ---- 温湿度任务（阻塞版，这才是 RTOS 该有的写法）---------------------- */
static void Task_Aht20(void *p_arg)
{
    OS_ERR err;
    float t, h;
    char  ts[12], hs[12];
    uint8_t ret;

    (void)p_arg;

    for (;;) {
        ret = AHT20_ReadBlocking(&t, &h);      /* 内部 OSTimeDly(80ms)，CPU 去跑别的任务 */
        if (ret == AHT20_OK) {
            FmtFix2(ts, sizeof(ts), t);
            FmtFix2(hs, sizeof(hs), h);
            BSP_UART_Printf("[%8lu ms] TASK3  T=%s C  H=%s %%RH  (CRC %s)\r\n",
                            (unsigned long)NowMs(), ts, hs,
                            AHT20_LastCrcOk() ? "ok" : "NG");
        } else {
            BSP_UART_Printf("[%8lu ms] TASK3  AHT20 read failed (%u)\r\n",
                            (unsigned long)NowMs(), ret);
        }
        OSTimeDlyHMSM(0, 0, 2, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

/* ---- 创建所有任务 ------------------------------------------------------ */
void App_TasksInit(void)
{
    OS_ERR err;

    BSP_UART_Printf("\r\n==== uC/OS-III + AHT20 ====\r\n");

    OSTaskCreate(&s_led1_tcb, "LED1", Task_Led1, 0, PRIO_LED1,
                 &s_led1_stk[0], LED1_STK_SIZE / 10, LED1_STK_SIZE,
                 0, 0, 0, &err);
    OSTaskCreate(&s_led2_tcb, "LED2", Task_Led2, 0, PRIO_LED2,
                 &s_led2_stk[0], LED2_STK_SIZE / 10, LED2_STK_SIZE,
                 0, 0, 0, &err);
    OSTaskCreate(&s_aht_tcb, "AHT", Task_Aht20, 0, PRIO_AHT,
                 &s_aht_stk[0], AHT_STK_SIZE / 10, AHT_STK_SIZE,
                 0, 0, 0, &err);
}
```

> `OSTaskCreate` 原型（uC/OS-III v3.04+）：
> ```c
> void OSTaskCreate(OS_TCB *p_tcb, CPU_CHAR *p_name,
>                   OS_TASK_PTR p_task, void *p_arg,
>                   OS_PRIO prio, CPU_STK *p_stk_base,
>                   CPU_STK_SIZE stk_limit, CPU_STK_SIZE stk_size,
>                   void *p_ext, OS_OPT opt, OS_ERR *p_err);
> ```
> 注意参数顺序是 `p_stk_base`、`stk_limit`、`stk_size`，**栈底在前**。

### 关键变化只有一个：`AHT20_ReadBlocking`

现在的版本用状态机（因为协作式调度器里不能阻塞），uC/OS 版直接用阻塞函数：

```c
/* 放在 aht20.c 里 */
uint8_t AHT20_ReadBlocking(float *temp, float *humi)
{
    OS_ERR err;
    uint8_t ret;

    if (AHT20_StartMeasurement() != AHT20_OK) return AHT20_ERROR;
    OSTimeDly(80, OS_OPT_TIME_DLY, &err);        /* ← 就在这里，CPU 去跑 LED 任务了 */
    ret = AHT20_ReadMeasurement(temp, humi);
    return ret;
}
```

**这就是 RTOS 的意义**：任务可以"睡 80 ms"，CPU 立刻去跑别的任务；
而在裸机/协作式里，你只能 `HAL_Delay(80)` 干等，或者写丑陋的状态机。

---

## 4. 移植 uC/OS-III 到 STM32F103 的完整步骤

### 4.1 准备源码

从 Micrium 官方（GitHub `weston-embedded/uC-OS3`）或野火/正点原子的资料包里拿三部分：

```text
uC-CPU/             cpu_c.c  cpu_core.c  cpu_def.h  cpu_core.h
uC-LIB/             lib_ascii.c  lib_def.h  lib_math.c  lib_mem.c  lib_str.c ...
uC-OS3/Source/      os_core.c  os_task.c  os_time.c  os_tick.c  os_sem.c
                    os_q.c  os_mem.c  os_mutex.c  os_msg.c  os_flag.c
                    os_prio.c  os_int.c  os_pend_multi.c  os_stat.c
                    os_tmr.c  os_var.c  os_dbg.c  os_cfg_app.c
uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/ARM/Keil/
                    os_cpu.h  os_cpu_c.c  os_cpu_a.asm
```

### 4.2 目录结构建议

把源码放到工程里（**不要**放在 `Drivers/`，那是 HAL 的地盘）：

```text
AHT20_RTOS/
├── Inc/  Src/                     ← 应用层（现在这些）
├── Drivers/                       ← HAL + CMSIS（不变）
├── RTOS/
│   ├── uC-CPU/
│   ├── uC-LIB/
│   └── uC-OS3/
│       ├── Source/
│       └── Ports/
└── MDK-ARM/
```

然后在 Keil 里新建 `RTOS` 分组，把上面这些 `.c` 和 `os_cpu_a.asm` 加进去。
**注意**：`os_cpu_a.asm` 是 ARM 汇编（armasm 语法），Keil 里文件类型选 "Assembler Source File"，
AC6 工程一样能编（走 armasm 而不是 armclang）。

### 4.3 配置文件

`os_cfg.h` 和 `os_cfg_app.h` 放在 `Inc/` 或 `RTOS/uC-OS3/` 下，关键项：

```c
/* os_cfg.h */
#define OS_CFG_TASK_PRIO_MAX      32u      /* 32 个优先级够用 */
#define OS_CFG_TICK_RATE_HZ       1000u    /* 1 tick = 1 ms */
#define OS_CFG_SCHED_ROUND_ROBIN_EN 1u     /* 同优先级轮转 */
#define OS_CFG_STAT_EN            1u       /* 统计（调试用） */
#define OS_CFG_DBG_EN             0u       /* 发布时关掉，省空间 */

/* os_cfg_app.h */
#define OS_CFG_TASK_STK_SIZE      256u     /* 默认栈大小 */
#define OS_CFG_ISR_STK_SIZE       256u
#define OS_CFG_TICK_TASK_PRIO     2u
#define OS_CFG_TMR_TASK_PRIO      3u
```

### 4.4 中断向量对接（**最容易出错的地方**）

uC/OS-III 靠两个异常工作：**SysTick**（节拍）和 **PendSV**（任务切换）。
`startup_stm32f103xb.s` 里的默认实现必须让位：

```c
/* 方法 A：直接改启动文件里的向量名（最干净） */
DCD     OS_CPU_PendSVHandler      ; 原来是 PendSV_Handler
DCD     OS_CPU_SysTickHandler     ; 原来是 SysTick_Handler

/* 方法 B：在 stm32f1xx_it.c 里删掉同名函数，改写成跳转 */
void SysTick_Handler(void)     { OS_CPU_SysTickHandler(); }
void PendSV_Handler(void)      { OS_CPU_PendSVHandler();  }
```

两种方法选一种，**不要同时在启动文件和 `.c` 里定义同名函数**，否则 `multiply defined` 链接错误。

优先级（`os_cpu_c.c` 的 `OS_CPU_SysTickInit` 或 `CPU_Init` 里）：

| 异常 | 优先级 | 说明 |
| --- | --- | --- |
| PendSV | 最低（0xFF） | 必须最低，保证切换不打断其他中断 |
| SysTick | 中（如 0xE0） | 节拍中断 |
| 其他外设 | 高于 SysTick | 可以用 `OSIntEnter/Exit` 包一层 |

### 4.5 `main.c` 的改动

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    BSP_I2C_Init();

    OSInit(&err);                 /* ① 初始化内核 */
    App_TasksInit();              /* ② 创建任务（内部调用 OSTaskCreate） */
    OSStart(&err);                /* ③ 启动调度器，永远不返回 */

    for (;;) { }
}
```

**注意 `OSInit` 必须在 `App_TasksInit` 之前**，否则 `OSTaskCreate` 会返回 `OS_ERR_OS_NOT_INIT`。

### 4.6 推荐的任务划分

| 任务 | 优先级 | 周期 | 栈 | 职责 |
| --- | --- | --- | --- | --- |
| 统计/空闲 | 最低（`OS_CFG_PRIO_MAX-1`） | - | 128 | 内核自带 |
| Tick | 2 | 1 ms | 256 | 内核自带 |
| Timer | 3 | - | 256 | 内核自带 |
| LED1 | 6 | 1 s | 128 | PC13 翻转 |
| LED2 | 7 | 3 s | 128 | PB12 翻转 |
| AHT20 | 8 | 2 s | 512 | 采集 + 串口 |
| 串口发送 | 9（可选） | - | 256 | 如果改用中断/DMA 发送 |

优先级原则：**按"实时性要求"排，不按"重要性"排**。
LED 只是视觉效果，AHT20 有 80 ms 阻塞，放在最低业务优先级即可。

---

## 5. 迁移时的五个坑（都是真实会遇到的）

### 5.1 `HAL_GetTick()` 永远返回 0

`HAL_Init()` 里 `HAL_InitTick()` 用 SysTick 累加 `HAL_GetTick()` 的计数。
一旦 SysTick 被 uC/OS 接管（`OS_CPU_SysTickHandler` 只调 `OSTimeTick`），
`HAL_IncTick()` 就没人调了 → `HAL_Delay()` 卡死、`HAL_GetTick()` 恒为 0。

**解决**：在 `os_cpu_c.c` 的 SysTick 处理函数里补一行：

```c
void OS_CPU_SysTickHandler(void)
{
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();
    OSIntEnter();
    CPU_CRITICAL_EXIT();
    OSTimeTick();
    HAL_IncTick();            /* ← 加这一行，保住 HAL 的毫秒计数 */
    OSIntExit();
}
```

或者在应用层**改用 `OSTimeGet()`**（本文第 3 节的写法），完全不依赖 HAL。
两种都行，**推荐后者**，因为更"RTOS 原生"。

### 5.2 `HAL_Delay()` 在任务里不能用

`HAL_Delay` 是死等（`while ((HAL_GetTick() - tickstart) < wait)`）。
在 RTOS 里死等会**独占总 CPU**（因为它是最高优先级之一，循环里没有阻塞点）。
一律换成 `OSTimeDly` / `OSTimeDlyHMSM`。

### 5.3 栈太小 → HardFault

`vsnprintf` + 浮点格式化很吃栈（几百字节）。
AHT20 任务的栈给 **512 words（2 KB）** 起步，LED 任务 128 words 够了。
判断栈溢出的方法：把 `OS_CFG_STAT_EN` 打开，用 `OSTaskStkChk()` 查剩余栈空间；
或者调试时在 `HardFault_Handler` 里看 `MSP/PSP` 和栈内容。

### 5.4 `multiply defined symbol`

启动文件里 `PendSV_Handler` / `SysTick_Handler` 是 `WEAK` 的，
但如果 `stm32f1xx_it.c` 里也定义了，而你又改了启动文件向量名，就会出现重复定义。
**统一用方法 B（改 `.c` 里的跳转）最省事**，启动文件不动。

### 5.5 `OS_ERR_OS_NOT_INIT` / `OS_ERR_TASK_CREATE_ISR`

- 前者：`OSInit()` 没调或调用顺序错了；
- 后者：在中断里调 `OSTaskCreate`（不允许），或者 `OSRunning` 还没置位就创建任务。

---

## 6. 迁移前后代码对比（说明"任务体不用改"）

| 位置 | 现在（OSAL） | uC/OS-III 版 | 改动量 |
| --- | --- | --- | --- |
| 任务函数签名 | `void Task_Led1(void *arg)` | 相同 | 0 |
| LED 翻转 | `Led_Write(...)` | 相同 | 0 |
| 打印 | `BSP_UART_Printf(...)` | 相同 | 0 |
| 周期延时 | 任务返回（隐式） | `OSTimeDlyHMSM(...)` | 1 行 |
| 时间戳 | `OSAL_Now()` | `OSTimeGet()` | 1 处 |
| AHT20 采集 | 状态机 | `AHT20_ReadBlocking()` | 整段简化 |
| 创建任务 | `OSAL_TaskCreate` | `OSTaskCreate` + TCB/栈 | 3 处 |
| 启动 | `OSAL_Start()` | `OSInit` + `OSStart` | 2 行 |

**结论**：应用层（`aht20.c`、`bsp_uart.c`、`bsp_i2c_*.c`、`gpio.c`）**一行都不用改**，
因为它们只依赖 `BSP_I2C_*` / `HAL_UART_*`，和内核无关。
这就是"先把任务二做出来、回头再换内核"能成立的架构原因。

---

## 7. 任务一的"hello uc/OS!"怎么办

任务一要求 task3 每 2 s 发：

```text
hello uc/OS! 欢迎来到RTOS多任务环境！
```

而任务二第 3 小问要求 task3 每 2 s 发温湿度。
两者**周期一样（都是 2 s）**，最优雅的做法是**合并到同一个任务里，一次输出两行**：

```c
for (;;) {
    /* 任务一的要求 */
    BSP_UART_Printf("hello uc/OS! 欢迎来到RTOS多任务环境！\r\n");

    /* 任务二第 3 小问的要求 */
    AHT20_ReadBlocking(&t, &h);
    BSP_UART_Printf("[%8lu ms] TASK3  T=%s C  H=%s %%RH\r\n", ...);

    OSTimeDlyHMSM(0, 0, 2, 0, OS_OPT_TIME_HMSM_STRICT, &err);
}
```

这样**一个任务同时满足两份作业**，答辩时也好讲：
"任务一验证了多任务调度，任务二把第三个任务从'打印字符串'升级成'读传感器'，
既保留了任务一的输出，又完成了数据采集。"

如果你希望两个实验**各自独立可交**，就在 `board_config.h` 加一个宏：

```c
#define APP_TASK3_HELLO   1    /* 1 = 也输出 hello 行；0 = 只输出温湿度 */
```

---

## 8. 完成顺序建议

1. **今天**：交任务二（当前工程，OSAL 版，已编译通过、串口可验证）；
2. **下一步**：按第 4 节把 uC/OS-III 源码加进工程，先只让 `Task_Led1` 跑起来（验证内核能跑）；
3. 再迁 `Task_Led2`、`Task_Aht20`（此时 `AHT20_ReadBlocking` 才有意义）；
4. 用 `OSTaskStkChk` + 串口统计验证栈够用；
5. 最后录一段"两个灯不同频率闪 + 串口每 2 s 出温湿度"的视频，两个任务一起交。

> 提醒：uC/OS-III 的许可证是 **Apache-2.0（v3.08 起）**，学校实验用完全没问题；
> 但报告里引用时建议注明版本号（如 v3.08.01）。

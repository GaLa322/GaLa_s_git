# STM32F103 + AHT20 多任务温湿度采集（任务一 + 任务二）

> 嵌入式系统课程实验：**任务一**把 uC/OS-III 移植到 STM32F103 并建 3 个任务；
> **任务二**用 I2C 采 AHT20 温湿度并从串口输出，最后把任务二的采集放进任务一的第 3 个任务里。
>
> 本目录给了**两个可编译工程**，代码 95% 相同，方便对照着讲"为什么需要 RTOS"：
>
> | 工程 | 内核 | 用途 |
> | --- | --- | --- |
> | `01_Project/AHT20_RTOS/` | 无内核（自带约 100 行的 `osal.c` 任务调度器） | 先跑通任务二，不依赖内核源码 |
> | `01_Project/AHT20_uCOS3/` | **真 uC/OS-III V3.08.02** | 任务一 + 任务二第 3 小问的最终交付 |

---

## 1. 这个工程做了什么

开发板每 2 秒通过 I2C 读一次 AHT20 的温湿度，并且**同时**在做另外两件事：

| 任务 | 周期 | 行为 |
| --- | --- | --- |
| Task_Led1 | 1000 ms | 翻转 PC13 板载 LED（1 s 亮 / 1 s 灭） |
| Task_Led2 | 3000 ms | 翻转 PB12 外接 LED（3 s 亮 / 3 s 灭） |
| Task_Aht20 | 10 ms 轮询 | 每 2000 ms 采一次温湿度，串口打印 |

> 下表是工程①（OSAL 版）的形状；工程②（uC/OS-III）同样是三个任务，
> 只是 TASK3 改成"先打印 hello uc/OS! 再阻塞式采一次温湿度"，见 `06_任务一_uCOS-III移植全记录.md`。

串口（USART1，115200 8N1）上的输出长这样：

```text
==================================================
  STM32F103C8T6 + AHT20  RTOS 多任务实验
  姓名: 你的名字   学号: 你的学号
  CPU : 8 MHz (HSI)
  TASK1: LED1 每 1000 ms 翻转一次
  TASK2: LED2 每 3000 ms 翻转一次
  TASK3: AHT20 每 2000 ms 采集一次
  I2C  : 硬件外设 I2C1 @ PB6/PB7, AHT20 addr 0x38
==================================================
[     112 ms] BOOT   AHT20 init OK
[     112 ms] OSAL   3 tasks created, starting...
T=26.50 C  H=55.95 %RH
T=26.52 C  H=56.03 %RH
...
```

串口上只有温湿度，每 2 s 一行、间隔稳定；LED 的状态不上报，靠眼睛看
（PC13 亮 1 s / 灭 1 s，PB12 亮 3 s / 灭 3 s）。
想让串口连 LED 翻转一起打印，把 `Inc/board_config.h` 里的 `APP_LOG_LED` 改成 `1` 重新编译即可。

## 2. 目录结构

```text
STM32F103_RTOS_AHT20/
├─ 01_Project/
│  ├─ AHT20_RTOS/                  Keil 工程①：无内核（OSAL）版，先跑通任务二
│  │  ├─ AHT20_RTOS.ioc            CubeMX 配置（时钟/I2C1/USART1/GPIO）
│  │  ├─ Inc/  Src/                应用代码（见下表）
│  │  ├─ Drivers/                  STM32F1 HAL 库 + CMSIS
│  │  └─ MDK-ARM/AHT20_RTOS.uvprojx
│  └─ AHT20_uCOS3/                 Keil 工程②：真 uC/OS-III（任务一 + 任务二第3小问）
│     ├─ AHT20_uCOS3.ioc           同上的 CubeMX 配置
│     ├─ Inc/  Src/  Drivers/      同工程①（4 个文件按内核改写）
│     ├─ RTOS/                     uC-CPU + uC-LIB + uC-OS3 源码（1.7 MB）
│     └─ MDK-ARM/AHT20_uCOS3.uvprojx
└─ 02_Docs/                        教程、博客稿、接线图
   ├─ images/wiring.png            接线示意图
   ├─ tools/gen_wiring.py          生成接线图的脚本
   ├─ tools/build_uvprojx.py       生成工程① 的脚本
   └─ tools/build_uvprojx_ucos.py  生成工程② 的脚本
```

应用层代码分工（都可以单独拿去讲）：

| 文件 | 作用 |
| --- | --- |
| `Src/main.c` | 初始化外设 → 建任务 → 启动调度器 |
| `Src/app_tasks.c` | 三个任务的具体实现 |
| `Src/osal.c` `Inc/osal.h` | 轻量任务调度器（uC/OS 的"替身"，**只在工程①里有**） |
| `RTOS/` | uC/OS-III + uC-CPU + uC-LIB 源码（**只在工程②里有**） |
| `Src/aht20.c` `Inc/aht20.h` | AHT20 驱动：初始化 / 触发 / 读数据 / CRC8 |
| `Src/bsp_i2c_hw.c` | 硬件 I2C 实现（HAL I2C1） |
| `Src/bsp_i2c_sw.c` | 软件 I2C 实现（GPIO 模拟时序） |
| `Src/bsp_uart.c` | 串口打印 `BSP_UART_Printf()` |
| `Inc/board_config.h` | 所有可调参数集中在这里（周期、引脚、I2C 方式…） |

## 3. 五分钟跑起来

1. **接线**：见 `images/wiring.png` 和下面第 4 节表格。
2. **打开工程**：先跑任务二就双击 `01_Project/AHT20_RTOS/MDK-ARM/AHT20_RTOS.uvprojx`；
   要交任务一就双击 `01_Project/AHT20_uCOS3/MDK-ARM/AHT20_uCOS3.uvprojx`。
3. **选下载器**：`Options for Target → Debug` 选 `CMSIS-DAP Debugger`（DAPLink），
   `Settings → Flash Download` 勾上 `Reset and Run`。
4. **编译下载**：`F7` 编译，`F8` 下载。
5. **看串口**：USB-TTL 插电脑，串口助手选 115200 8N1，**编码选 UTF-8**，打开串口。

## 4. 接线表（最短版本）

| STM32F103 | 接到 | 说明 |
| --- | --- | --- |
| 3.3V | AHT20 VDD (脚1) | **不要接 5V**，和 I2C 电平保持一致 |
| GND | AHT20 GND (脚3) | |
| PB6 | AHT20 SCL (脚4) | I2C1_SCL，400 kHz |
| PB7 | AHT20 SDA (脚2) | I2C1_SDA，开漏 + 上拉 |
| PA9 | USB-TTL **RXD** | 单片机的 TX 接模块的 RX |
| PA10 | USB-TTL **TXD** | 单片机的 RX 接模块的 TX |
| GND | USB-TTL GND | **必须共地**，否则串口全是乱码 |
| PB12 | 330 Ω → LED → GND | 第二个灯（没有就只用板载 PC13） |
| PA13 / PA14 | DAPLink SWDIO / SWCLK | 下载调试用，另接 3.3V、GND |

> AHT20 模块通常自带 4.7 kΩ 上拉；如果你的模块没有，请给 SDA、SCL 各加一个
> 4.7 kΩ 到 3.3V，否则 I2C 一定读不到数据（表现为 init FAIL）。

## 5. 文档索引

| 文档 | 内容 | 对应作业要求 |
| --- | --- | --- |
| [01_任务二_完整教程.md](01_任务二_完整教程.md) | 从零到出数据的全部步骤、代码讲解、验证与排错 | 任务二整体 |
| [02_软件I2C与硬件I2C.md](02_软件I2C与硬件I2C.md) | 两种 I2C 的原理、实现、优缺点对比 | 任务二第 1 小问 |
| [03_AHT20数据手册解读.md](03_AHT20数据手册解读.md) | 手册里的命令、时序、数据格式、CRC、换算 | 任务二第 2 小问 |
| [04_与任务一对接_uCOS-III.md](04_与任务一对接_uCOS-III.md) | 现在的"任务壳"如何换成真正的 uC/OS-III | 任务二第 3 小问 + 任务一 |
| [05_博客稿_CSDN版.md](05_博客稿_CSDN版.md) | 可直接发布的一篇博客（含踩坑记录） | 写博客 |
| [06_任务一_uCOS-III移植全记录.md](06_任务一_uCOS-III移植全记录.md) | 从官方仓库拿源码到 3 个任务跑起来的**完整移植过程**（含 4 个真实报错） | 任务一 + 任务二第 3 小问 |
| [07_博客_任务一与任务二.md](07_博客_任务一与任务二.md) | **两个任务合并成一篇博客**（按参考资料的文章结构：实验任务→器材接线→任务一→任务二→总结，图片留了 20 个插入位） | 交作业 / 发 CSDN |

## 6. 构建结果（本机实测，Arm Compiler 6.24）

**工程①（无内核 OSAL 版）**

```text
Program Size: Code=17934 RO-data=1298 RW-data=16 ZI-data=2008
"AHT20_RTOS\AHT20_RTOS.axf" - 0 Error(s), 0 Warning(s).
```

Flash ≈ 18.8 KB / 64 KB，RAM ≈ 2.0 KB / 20 KB。

**工程②（uC/OS-III）**

```text
Program Size: Code=27222 RO-data=1482 RW-data=16 ZI-data=7856
"AHT20_uCOS3\AHT20_uCOS3.axf" - 0 Error(s), 2 Warning(s).
```

Flash ≈ 28.0 KB / 64 KB，RAM ≈ 7.7 KB / 20 KB（ZI 里主要是 7 个任务的栈）。
那 1 条警告是 uVision 给汇编文件多传了 `-D __UVISION_VERSION=543`，IDE 行为，可忽略。

## 7. 参数都在 `board_config.h`

```c
#define APP_LED1_PERIOD_MS   1000U   /* 任务1周期 */
#define APP_LED2_PERIOD_MS   3000U   /* 任务2周期 */
#define APP_LOG_LED          0       /* 0：串口只打温湿度；1：连 LED 翻转一起打印 */
#define APP_AHT20_PERIOD_MS  2000U   /* 任务3采样周期 */
#define APP_AHT20_POLL_MS      10U   /* 任务3轮询步长 */
#define APP_I2C_USE_SOFT        0    /* 0=硬件I2C  1=软件I2C */
#define AHT20_I2C_ADDR7      0x38U
#define APP_STUDENT_NAME     "你的名字"   /* ← 改成你自己的 */
#define APP_STUDENT_ID       "你的学号"   /* ← 改成你自己的 */
```

改宏 → 重新编译即可，不用动别的文件。

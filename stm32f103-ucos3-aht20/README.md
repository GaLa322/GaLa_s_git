# STM32F103 + uC/OS-III + AHT20 温湿度采集

> 嵌入式系统课程实验的完整工程与教程：**在 STM32F103C8T6 上移植 uC/OS-III 建 3 个任务**，
> **用 I2C 采 AHT20 温湿度并从串口输出**，最后把采集任务放进 RTOS 的第 3 个任务里。
>
> 仓库里给了两个可以**直接用 Keil MDK 打开、编译、下载**的工程，代码 95% 相同，
> 方便对照讲"为什么需要 RTOS"：

| 工程 | 内核 | 用途 | 编译结果 |
| --- | --- | --- | --- |
| [`01_Project/AHT20_RTOS/`](01_Project/AHT20_RTOS/) | 无内核（自带约 100 行的 `osal.c` 时间片调度器） | 先跑通任务二，不依赖内核源码 | Code 17 934 B / 0 Error 0 Warning |
| [`01_Project/AHT20_uCOS3/`](01_Project/AHT20_uCOS3/) | **真 uC/OS-III V3.08.02** | 任务一 + 任务二第(3)小问的最终交付 | Code 21 640 B（只打印）/ 27 846 B（带采集） |

两者都在 **ARMCLANG V6.24（Keil MDK 5.43a）+ STM32F1 HAL** 下编译通过，
目标芯片 `STM32F103C8`，时钟 **HSI 8 MHz**（不依赖外部晶振）。

---

## 1. 两个任务做了什么

| 任务 | 优先级 | 周期 | 行为 |
| --- | --- | --- | --- |
| `Task_Led1` | 6 | 1000 ms | 翻转 **PC13** 板载 LED（1 s 亮 / 1 s 灭） |
| `Task_Led2` | 7 | 3000 ms | 翻转 **PB12** 外接 LED（3 s 亮 / 3 s 灭） |
| `Task_Aht20` | 8 | 2000 ms | 任务一：打印 `hello uc/OS! 欢迎来到RTOS多任务环境！`<br>任务二(3)：采集温湿度并打印 `T=26.51 C  H=55.94 %RH` |

任务三的行为用 `board_config.h` 里一个宏切换，改一行就换版本：

```c
#define APP_TASK3_AHT20   0    /* 0：任务一（只打印 hello）；1：任务二第(3)小问（加采集）*/
```

## 2. 目录结构

```text
STM32F103_RTOS_AHT20/
├── README.md                       ← 你正在看的这个文件
├── .gitignore                      ← 已排除 Keil 编译产物与 60 MB 用不到的 CMSIS DSP
├── THIRD_PARTY_NOTICES.md          ← 第三方代码许可（uC/OS-III 是 Apache-2.0）
├── LICENSES/Apache-2.0.txt
├── 01_Project/
│   ├── AHT20_RTOS/                 ← 工程① 无内核版
│   │   ├── AHT20_RTOS.ioc           CubeMX 工程文件（USART1 / I2C1 / PC13 / PB12）
│   │   ├── Inc/  Src/               应用层代码（下面是各文件分工）
│   │   ├── Drivers/                 STM32F1 HAL + CMSIS
│   │   └── MDK-ARM/AHT20_RTOS.uvprojx
│   └── AHT20_uCOS3/                ← 工程② uC/OS-III 版
│       ├── RTOS/                    uC-OS3 V3.08.02 + uC-CPU + uC-LIB（保持官方目录层级）
│       └── MDK-ARM/AHT20_uCOS3.uvprojx
└── 02_Docs/                        ← 7 篇中文文档 + 接线图 + 生成脚本
```

应用层文件分工（两个工程基本一致）：

| 文件 | 作用 |
| --- | --- |
| `Src/main.c` | 初始化外设 → 建任务 → 启动调度器（OSAL 或 `OSStart`） |
| `Src/app_tasks.c` | 三个任务的具体实现（本文重点） |
| `Src/aht20.c` `Inc/aht20.h` | AHT20 驱动：初始化 / 触发测量 / 读数据 / CRC8 |
| `Src/bsp_i2c_hw.c` | 硬件 I2C（HAL `I2C1`，400 kHz） |
| `Src/bsp_i2c_sw.c` | 软件 I2C（GPIO 位翻转，约 100 kHz，含总线恢复） |
| `Src/bsp_uart.c` | `BSP_UART_Printf()`：`vsnprintf` + `HAL_UART_Transmit`（不用 printf 免 semihosting） |
| `Src/osal.c` | 工程①的轻量调度器（工程②里已删，改用 uC/OS-III） |
| `Inc/board_config.h` | **所有可调参数**：引脚、周期、I2C 方式、任务三行为、姓名学号 |

## 3. 五分钟跑起来

```bash
git clone https://github.com/<你的用户名>/<仓库名>.git
```

1. Keil 打开 `01_Project/AHT20_uCOS3/MDK-ARM/AHT20_uCOS3.uvprojx`
2. `F7` 编译（首次会提示装 `Keil.STM32F1xx_DFP`，装 2.4.1 即可）
3. `F8` 下载（CMSIS-DAP + STM32F10x Med-density Flash，勾 Reset and Run）
4. 串口助手：**115200 / 8N1 / UTF-8**

接线只要 4 类（详见 [`02_Docs/images/wiring.png`](02_Docs/images/wiring.png) 与
[`02_Docs/images/wiring_task1.png`](02_Docs/images/wiring_task1.png)）：

| 外设 | 接到 | 说明 |
| --- | --- | --- |
| AHT20 VDD / GND | 3V3 / GND | **不要接 5V**（与 I2C 电平一致） |
| AHT20 SDA / SCL | **PB7 / PB6** | 模块自带 4.7 kΩ 上拉；无上拉时外接 4.7 kΩ 到 3V3 |
| AHT20 第 4 脚是 SCL | — | 该模块引脚顺序是 VDD-SDA-GND-**SCL**，接反不工作 |
| LED2 | **PB12** → 330 Ω → LED → GND | 任务二/任务一的第二个灯，没有就用板载 PC13 |
| USB-TTL | PA9→RXD、PA10→TXD、GND↔GND | 必须共地；串口助手编码选 UTF-8 |

预期串口输出（工程②，`APP_TASK3_AHT20 0`）：

```text
==================================================
  STM32F103C8T6 + AHT20  uC/OS-III 多任务实验
  RTOS : uC/OS-III V3.08.02, tick 1000 Hz
  TASK1: LED1 每 1000 ms 翻转 (prio 6)
  TASK2: LED2 每 3000 ms 翻转 (prio 7)
  TASK3: hello uc/OS! 每 2000 ms 一次 (prio 8)
==================================================
hello uc/OS! 欢迎来到RTOS多任务环境！
```

## 4. 文档

| 文档 | 内容 | 对应作业 |
| --- | --- | --- |
| [01_任务二_完整教程.md](02_Docs/01_任务二_完整教程.md) | 从零到出数据的全部步骤、代码讲解、验证与排错 | 任务二整体 |
| [02_软件I2C与硬件I2C.md](02_Docs/02_软件I2C与硬件I2C.md) | 两种 I2C 的原理、实现、优缺点对比 | 任务二第 1 小问 |
| [03_AHT20数据手册解读.md](02_Docs/03_AHT20数据手册解读.md) | 手册里的命令、时序、数据格式、CRC、换算 | 任务二第 2 小问 |
| [04_与任务一对接_uCOS-III.md](02_Docs/04_与任务一对接_uCOS-III.md) | 任务壳如何换成真正的 uC/OS-III | 任务二第 3 小问 |
| [05_博客稿_CSDN版.md](02_Docs/05_博客稿_CSDN版.md) | 任务二单篇博客稿 | 写博客 |
| [06_任务一_uCOS-III移植全记录.md](02_Docs/06_任务一_uCOS-III移植全记录.md) | 从官方仓库拿源码到 3 个任务跑起来的**完整移植过程**（含 4 个真实报错与修复） | 任务一 |
| [07_博客_任务一与任务二.md](02_Docs/07_博客_任务一与任务二.md) | 两个任务合并成一篇博客（留了 20 个截图位） | 交作业 / 发 CSDN |
| [08_上传到GitHub.md](02_Docs/08_上传到GitHub.md) | 本仓库怎么推到 GitHub（含代理与常见报错） | 存档 |

## 5. 环境

| 项 | 版本 |
| --- | --- |
| Keil MDK | 5.43a，编译器 ARMCLANG V6.24 |
| STM32CubeMX | 6.x（`.ioc` 可直接打开重新生成） |
| Device Family Pack | Keil.STM32F1xx_DFP 2.4.1 |
| 目标板 | STM32F103C8T6 最小系统板（Blue Pill），20 KB RAM / 64 KB Flash |
| 下载器 | DAPLink / ST-Link（SWD：SWDIO-PA13、SWCLK-PA14、GND） |

## 6. 许可

应用层代码为课程实验代码，可自由参考。
`RTOS/` 下的 uC/OS-III、uC/CPU、uC/LIB 以及 CMSIS 为 **Apache-2.0**，
`Drivers/STM32F1xx_HAL_Driver/` 为 ST 的 **BSD-3-Clause**，详见
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

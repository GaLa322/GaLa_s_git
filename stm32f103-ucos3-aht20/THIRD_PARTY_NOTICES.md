# 第三方代码与许可声明

本仓库的**应用层代码**（`01_Project/*/Src`、`01_Project/*/Inc`、`01_Project/*/*.ioc`、
`02_Docs/`）是作者为课程实验编写的。
仓库里同时包含以下第三方开源代码，它们**各自保留原有许可证**，版权归原作者所有：

| 目录 | 内容 | 版本 | 许可证 |
| --- | --- | --- | --- |
| `01_Project/AHT20_uCOS3/RTOS/uC-OS3/` | uC/OS-III 实时内核（Weston Embedded / Silicon Labs） | V3.08.02 | Apache-2.0 |
| `01_Project/AHT20_uCOS3/RTOS/uC-CPU/` | uC/CPU 移植层 | V1.32.01 | Apache-2.0 |
| `01_Project/AHT20_uCOS3/RTOS/uC-LIB/` | uC/LIB 通用库 | V1.39.01 | Apache-2.0 |
| `01_Project/*/Drivers/STM32F1xx_HAL_Driver/` | STM32F1 HAL 驱动（STMicroelectronics） | CubeMX 生成 | BSD-3-Clause |
| `01_Project/*/Drivers/CMSIS/` | ARM CMSIS 设备头文件与内核定义 | V5.x | Apache-2.0 |

## 关于 Apache-2.0

uC/OS-III、uC/CPU、uC/LIB 与 CMSIS 采用 Apache License 2.0 授权，许可证全文见仓库根目录
`LICENSES/Apache-2.0.txt`。每个源文件头部都带有

```c
/*  SPDX-License-Identifier: APACHE-2.0  */
```

形式的声明，再分发时请勿删除这些版权头。

上游地址：

- uC/OS-III：<https://github.com/weston-embedded/uC-OS3>
- uC/CPU：<https://github.com/weston-embedded/uC-CPU>
- uC/LIB：<https://github.com/weston-embedded/uC-LIB>

## 关于 BSD-3-Clause

`Drivers/STM32F1xx_HAL_Driver/` 下的文件由 STMicroelectronics 以 BSD-3-Clause 授权，
文件头部保留了原始版权声明（"COPYRIGHT(c) 2016 STMicroelectronics"），请一并保留。

## 本仓库自己的代码

课程实验代码建议使用 MIT 或直接声明"仅用于课程学习"。如果要公开发布，请把
`LICENSE` 文件补上（可选，不影响编译）。

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""用 TimerLED.uvprojx（AC6 模板）生成 AHT20_uCOS3 的 Keil 工程。

和 build_uvprojx.py 的区别：
  1. IncludePath 追加 8 条 RTOS 头文件目录；
  2. 文件组多了 uC-CPU / uC-LIB / uC-OS3 三组，
     其中 os_cpu_a.asm / cpu_a.asm 是 ARMASM 汇编（FileType=2）。
"""
from pathlib import Path
import xml.etree.ElementTree as ET

TEMPLATE = Path(r"D:\dsh_list\STM32F103_PWM_LED\01_TimerLED_2s\MDK-ARM\TimerLED.uvprojx")
PROJECT = Path(r"D:\dsh_list\STM32F103_RTOS_AHT20\01_Project\AHT20_uCOS3")
OUT = PROJECT / "MDK-ARM" / "AHT20_uCOS3.uvprojx"
NAME = "AHT20_uCOS3"

INCLUDE_PATH = (
    "../Inc;"
    "../Drivers/STM32F1xx_HAL_Driver/Inc;"
    "../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy;"
    "../Drivers/CMSIS/Device/ST/STM32F1xx/Include;"
    "../Drivers/CMSIS/Include;"
    "../RTOS/uC-CPU;"
    "../RTOS/uC-CPU/Cfg;"
    "../RTOS/uC-CPU/ARM-Cortex-M/ARMv7-M/ARM;"
    "../RTOS/uC-LIB;"
    "../RTOS/uC-LIB/Cfg;"
    "../RTOS/uC-OS3/Source;"
    "../RTOS/uC-OS3/Cfg;"
    "../RTOS/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/ARM"
)

USER_FILES = [
    "main.c",
    "diag.c",
    "gpio.c",
    "i2c.c",
    "usart.c",
    "stm32f1xx_it.c",
    "stm32f1xx_hal_msp.c",
    "app_tasks.c",
    "aht20.c",
    "bsp_uart.c",
    "bsp_i2c_hw.c",
    "bsp_i2c_sw.c",
]

HAL_FILES = [
    "stm32f1xx_hal.c",
    "stm32f1xx_hal_rcc.c",
    "stm32f1xx_hal_rcc_ex.c",
    "stm32f1xx_hal_gpio.c",
    "stm32f1xx_hal_gpio_ex.c",
    "stm32f1xx_hal_dma.c",
    "stm32f1xx_hal_cortex.c",
    "stm32f1xx_hal_pwr.c",
    "stm32f1xx_hal_flash.c",
    "stm32f1xx_hal_flash_ex.c",
    "stm32f1xx_hal_exti.c",
    "stm32f1xx_hal_i2c.c",
    "stm32f1xx_hal_uart.c",
]

OS3_SOURCE = [
    "os_cfg_app.c",
    "os_core.c",
    "os_dbg.c",
    "os_flag.c",
    "os_mem.c",
    "os_msg.c",
    "os_mutex.c",
    "os_prio.c",
    "os_q.c",
    "os_sem.c",
    "os_stat.c",
    "os_task.c",
    "os_tick.c",
    "os_time.c",
    "os_tmr.c",
    "os_var.c",
]

ET.register_namespace("xsi", "http://www.w3.org/2001/XMLSchema-instance")
tree = ET.parse(TEMPLATE)
root = tree.getroot()
target = root.find(".//Targets/Target")
assert target is not None

target.find("TargetName").text = NAME
common = target.find("TargetOption/TargetCommonOption")
common.find("OutputDirectory").text = NAME + "\\"
common.find("OutputName").text = NAME
common.find("CreateHexFile").text = "1"

various = target.find("TargetOption/TargetArmAds/Cads/VariousControls")
ic = various.find("IncludePath")
ic.text = INCLUDE_PATH

groups = target.find("Groups")
for group in list(groups):
    groups.remove(group)


def add_group(name, files):
    group = ET.SubElement(groups, "Group")
    ET.SubElement(group, "GroupName").text = name
    files_el = ET.SubElement(group, "Files")
    for filename, filetype, path in files:
        file_el = ET.SubElement(files_el, "File")
        ET.SubElement(file_el, "FileName").text = filename
        ET.SubElement(file_el, "FileType").text = str(filetype)
        ET.SubElement(file_el, "FilePath").text = path


add_group("Application/MDK-ARM", [
    ("startup_stm32f103xb.s", 2, "startup_stm32f103xb.s"),
])
add_group("Application/User", [
    (name, 1, "../Src/" + name) for name in USER_FILES
])
add_group("Drivers/STM32F1xx_HAL_Driver", [
    (name, 1, "../Drivers/STM32F1xx_HAL_Driver/Src/" + name) for name in HAL_FILES
])
add_group("Drivers/CMSIS", [
    ("system_stm32f1xx.c", 1, "../Src/system_stm32f1xx.c"),
])
add_group("RTOS/uC-CPU", [
    ("cpu_core.c", 1, "../RTOS/uC-CPU/cpu_core.c"),
    ("cpu_c.c", 1, "../RTOS/uC-CPU/ARM-Cortex-M/ARMv7-M/cpu_c.c"),
    ("cpu_a.asm", 2, "../RTOS/uC-CPU/ARM-Cortex-M/ARMv7-M/ARM/cpu_a.asm"),
])
add_group("RTOS/uC-LIB", [
    ("lib_ascii.c", 1, "../RTOS/uC-LIB/lib_ascii.c"),
    ("lib_math.c", 1, "../RTOS/uC-LIB/lib_math.c"),
    ("lib_mem.c", 1, "../RTOS/uC-LIB/lib_mem.c"),
    ("lib_str.c", 1, "../RTOS/uC-LIB/lib_str.c"),
])
add_group("RTOS/uC-OS3", [
    (name, 1, "../RTOS/uC-OS3/Source/" + name) for name in OS3_SOURCE
] + [
    ("os_cpu_c.c", 1, "../RTOS/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/os_cpu_c.c"),
    ("os_cpu_a.asm", 2, "../RTOS/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/ARM/os_cpu_a.asm"),
    ("os_app_hooks.c", 1, "../RTOS/uC-OS3/Cfg/os_app_hooks.c"),
])
add_group("::CMSIS", [])

body = ET.tostring(root, encoding="utf-8").decode("utf-8")
if not body.startswith("<?xml"):
    body = '<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n' + body
OUT.write_text(body, encoding="utf-8")
print(f"Wrote {OUT}")

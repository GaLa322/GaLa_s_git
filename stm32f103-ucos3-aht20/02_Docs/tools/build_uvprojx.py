#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""用能编译的 TimerLED.uvprojx 作模板，重建 AHT20_RTOS 的 Keil 工程。

模板已经是 ARMCLANG(AC6) 结构，Device=STM32F103C8，
Define=USE_HAL_DRIVER,STM32F103xB，IncludePath 也齐了，
所以这里只需要改工程名、输出目录和文件组。
"""
from pathlib import Path
import xml.etree.ElementTree as ET

TEMPLATE = Path(r"D:\dsh_list\STM32F103_PWM_LED\01_TimerLED_2s\MDK-ARM\TimerLED.uvprojx")
PROJECT = Path(r"D:\dsh_list\STM32F103_RTOS_AHT20\01_Project\AHT20_RTOS")
OUT = PROJECT / "MDK-ARM" / "AHT20_RTOS.uvprojx"
NAME = "AHT20_RTOS"

USER_FILES = [
    "main.c",
    "gpio.c",
    "i2c.c",
    "usart.c",
    "stm32f1xx_it.c",
    "stm32f1xx_hal_msp.c",
    "osal.c",
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
add_group("::CMSIS", [])

body = ET.tostring(root, encoding="utf-8").decode("utf-8")
if not body.startswith("<?xml"):
    body = '<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n' + body
OUT.write_text(body, encoding="utf-8")
print(f"Wrote {OUT}")

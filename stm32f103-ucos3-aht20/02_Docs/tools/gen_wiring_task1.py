# -*- coding: utf-8 -*-
"""生成任务一接线示意图（Blue Pill + 板载 LED + 外接 LED + USB-TTL + DAPLink）。

任务一只需要：两盏灯（PC13 板载、PB12 外接）+ 一路串口（PA9/PA10），
不需要 AHT20、不需要面包板也能跑。
"""
import os
from PIL import Image, ImageDraw, ImageFont

FONT = r"C:\Windows\Fonts\simhei.ttf"
W, H = 1500, 1000
BG = "#ffffff"
INK = "#12222f"
MUTED = "#5b6b7a"

f_title = ImageFont.truetype(FONT, 42)
f_sub = ImageFont.truetype(FONT, 22)
f_box = ImageFont.truetype(FONT, 26)
f_pin = ImageFont.truetype(FONT, 21)
f_note = ImageFont.truetype(FONT, 20)
f_tiny = ImageFont.truetype(FONT, 18)

img = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(img)


def rbox(xy, fill, outline, radius=14, width=3):
    d.rounded_rectangle(xy, radius=radius, fill=fill, outline=outline, width=width)


def wire(points, color, width=5):
    for a, b in zip(points, points[1:]):
        d.line([a, b], fill=color, width=width, joint="curve")


def dot(x, y, color, r=6):
    d.ellipse([x - r, y - r, x + r, y + r], fill=color)


def label(x, y, text, color, font=f_tiny, anchor="lm"):
    bbox = d.textbbox((x, y), text, font=font, anchor=anchor)
    d.rectangle([bbox[0] - 4, bbox[1] - 2, bbox[2] + 4, bbox[3] + 2], fill=BG)
    d.text((x, y), text, font=font, fill=color, anchor=anchor)


# ---------------------------------------------------------------- 标题
d.text((60, 26), "STM32F103C8T6 接线示意图（任务一：uC/OS-III 三个任务）", font=f_title, fill=INK)
d.text((62, 86), "任务一只需要两盏灯 + 一路串口：PC13 板载 LED、PB12 外接 LED、PA9/PA10 串口；不接 AHT20",
       font=f_sub, fill=MUTED)

# ---------------------------------------------------------------- 开发板
BX0, BY0, BX1, BY1 = 110, 200, 440, 760
rbox((BX0, BY0, BX1, BY1), "#1565c0", "#0d47a1")
d.text((BX0 + 20, BY0 + 16), "STM32F103C8T6", font=f_box, fill="#ffffff")
d.text((BX0 + 20, BY0 + 50), "Blue Pill 最小系统板", font=f_sub, fill="#cfe2ff")

# 板载 LED（PC13）示意
d.text((BX0 + 20, BY0 + 300), "PC13 = 板载 LED", font=f_tiny, fill="#cfe2ff")
d.text((BX0 + 20, BY0 + 324), "（TASK1 直接翻转它）", font=f_tiny, fill="#cfe2ff")
lx, ly = BX0 + 34, BY0 + 366
d.rectangle([lx - 16, ly - 12, lx + 16, ly + 12], fill="#ffca28", outline="#6d4c41", width=2)
d.polygon([(lx + 20, ly - 12), (lx + 20, ly + 12), (lx + 44, ly)], fill="#4fc3f7")
d.line([(lx + 44, ly - 12), (lx + 44, ly + 12)], fill="#0288d1", width=3)
d.text((lx + 54, ly - 10), "LED1", font=f_tiny, fill="#bbdefb")

for y, name in [
    (240, "3V3"),
    (300, "GND"),
    (560, "PB12"),
    (620, "PA9   TX"),
    (680, "PA10 RX"),
]:
    dot(BX1, y, "#0d47a1")
    tw = d.textlength(name, font=f_pin)
    d.text((BX1 - 12 - tw, y - 12), name, font=f_pin, fill="#e8f1ff")

for x, name in [(230, "PA13"), (330, "PA14"), (420, "GND")]:
    tw = d.textlength(name, font=f_pin)
    d.text((x - tw / 2, BY1 - 34), name, font=f_pin, fill="#e8f1ff")
    dot(x, BY1, "#0d47a1")

# ---------------------------------------------------------------- 三个任务说明
IX0, IY0, IX1, IY1 = 880, 150, 1230, 420
rbox((IX0, IY0, IX1, IY1), "#f1f8e9", "#558b2f")
d.text((IX0 + 22, IY0 + 12), "三个任务（跑起来应该看到）", font=f_box, fill="#33691e")

rows = [
    ("TASK1", "1 s 翻转 PC13 板载 LED", "#1565c0"),
    ("TASK2", "3 s 翻转 PB12 外接 LED", "#6a1b9a"),
    ("TASK3", "2 s 串口打印 hello uc/OS!", "#00695c"),
]
for i, (name, what, color) in enumerate(rows):
    y = IY0 + 62 + i * 62
    d.rectangle([IX0 + 22, y - 14, IX0 + 108, y + 16], fill=color)
    d.text((IX0 + 30, y - 12), name, font=f_tiny, fill="#ffffff")
    d.text((IX0 + 120, y - 12), what, font=f_tiny, fill=INK)

d.text((IX0 + 22, IY0 + 234), "优先级 6 / 7 / 8", font=f_tiny, fill="#558b2f")

# ---------------------------------------------------------------- 外接 LED
LX0, LY0, LX1, LY1 = 880, 450, 1230, 590
rbox((LX0, LY0, LX1, LY1), "#fff8e1", "#f9a825")
d.text((LX0 + 22, LY0 + 12), "LED2（第二个灯，接 PB12）", font=f_box, fill="#e65100")
d.text((LX0 + 22, LY0 + 44), "PB12 → 330Ω → LED 正极", font=f_tiny, fill="#8d6e63")
d.text((LX0 + 22, LY0 + 70), "LED 负极 → 板子 GND 引脚", font=f_tiny, fill="#8d6e63")

wire([(BX1, 560), (600, 560), (600, 512), (LX0, 512)], "#6a1b9a")
label(608, 548, "PB12", "#6a1b9a")

cx, cy = 990, 556
d.rectangle([cx - 42, cy - 14, cx - 16, cy + 14], fill="#ffb300", outline="#6d4c41", width=2)
d.text((cx - 52, cy + 20), "330Ω", font=f_tiny, fill="#5d4037")
d.polygon([(cx + 4, cy - 16), (cx + 4, cy + 16), (cx + 40, cy)], fill="#e53935")
d.line([(cx + 40, cy - 16), (cx + 40, cy + 16)], fill="#b71c1c", width=4)
d.line([(cx + 42, cy), (cx + 150, cy)], fill="#000000", width=4)

# ---------------------------------------------------------------- USB-TTL
UX0, UY0, UX1, UY1 = 880, 600, 1230, 750
rbox((UX0, UY0, UX1, UY1), "#e3f2fd", "#1565c0")
d.text((UX0 + 22, UY0 + 12), "USB-TTL（CH340 等）", font=f_box, fill="#0d47a1")
d.text((UX0 + 22, UY0 + 46), "115200 8N1，另一端接电脑 USB", font=f_tiny, fill="#1565c0")

dot(UX0, 688, "#37474f")
d.text((UX0 + 16, 676), "RXD", font=f_pin, fill=INK)
dot(UX0, 726, "#37474f")
d.text((UX0 + 16, 714), "TXD", font=f_pin, fill=INK)

wire([(BX1, 620), (700, 620), (700, 688), (UX0, 688)], "#0277bd")
label(596, 602, "PA9(TX) → RXD", "#0277bd")

wire([(BX1, 680), (770, 680), (770, 726), (UX0, 726)], "#00838f")
label(590, 662, "PA10(RX) ← TXD", "#00838f")

# 共地：从板子底部绕到 TTL
wire([(420, BY1), (420, 812), (1050, 812), (1050, UY1)], "#000000")
label(700, 798, "共地 GND", "#000000", anchor="mm")
dot(1050, UY1, "#000000")

# ---------------------------------------------------------------- DAPLink
DX0, DY0, DX1, DY1 = 110, 826, 430, 916
rbox((DX0, DY0, DX1, DY1), "#f3e5f5", "#6a1b9a")
d.text((DX0 + 22, DY0 + 10), "DAPLink / ST-Link", font=f_box, fill="#4a148c")
d.text((DX0 + 22, DY0 + 46), "SWD 下载调试（可选）", font=f_tiny, fill="#6a1b9a")

wire([(230, BY1), (230, DY0)], "#4e342e")
wire([(330, BY1), (330, DY0)], "#4e342e")
label(240, 782, "SWDIO", "#4e342e")
label(340, 782, "SWCLK", "#4e342e")

# ---------------------------------------------------------------- 图例
gx, gy = 1262, 150
d.text((gx, gy), "图例", font=f_pin, fill=INK)
for i, (c, t) in enumerate([
    ("#000000", "GND"),
    ("#0277bd", "UART"),
    ("#6a1b9a", "LED"),
    ("#4e342e", "SWD"),
]):
    y = gy + 60 + i * 30
    d.line([(gx, y), (gx + 40, y)], fill=c, width=5)
    d.text((gx + 52, y - 10), t, font=f_tiny, fill=INK)

d.text((60, 932), "任务一不需要 AHT20、不需要面包板；LED2 一定要串 330Ω 限流电阻，别把 LED 直接跨在 3.3V 与 GND 之间。",
       font=f_note, fill=MUTED)
d.text((60, 958), "板子供电二选一：DAPLink 的 3V3/GND 或开发板自己的 USB 口，不要同时接两个电源。",
       font=f_note, fill=MUTED)
d.text((60, 984), "串口助手用 115200 8N1、编码 UTF-8，否则中文的 hello uc/OS! 会乱码。",
       font=f_note, fill=MUTED)

os.makedirs(r"D:\dsh_list\STM32F103_RTOS_AHT20\02_Docs\images", exist_ok=True)
img.save(r"D:\dsh_list\STM32F103_RTOS_AHT20\02_Docs\images\wiring_task1.png")
print("ok")

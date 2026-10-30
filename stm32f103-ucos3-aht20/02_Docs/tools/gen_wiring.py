# -*- coding: utf-8 -*-
"""生成任务二接线示意图（Blue Pill + AHT20 + USB-TTL + DAPLink + 外接 LED）。"""
import os
from PIL import Image, ImageDraw, ImageFont

FONT = r"C:\Windows\Fonts\simhei.ttf"
W, H = 1500, 1020
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
d.text((60, 28), "STM32F103C8T6 + AHT20 接线示意图（任务二）", font=f_title, fill=INK)
d.text((62, 86), "只画信号关系，引脚位置以开发板丝印为准；PB6/PB7 在 CubeMX 里配成 I2C1", font=f_sub, fill=MUTED)

# ---------------------------------------------------------------- 开发板
BX0, BY0, BX1, BY1 = 110, 200, 440, 760
rbox((BX0, BY0, BX1, BY1), "#1565c0", "#0d47a1")
d.text((BX0 + 20, BY0 + 16), "STM32F103C8T6", font=f_box, fill="#ffffff")
d.text((BX0 + 20, BY0 + 50), "Blue Pill 最小系统板", font=f_sub, fill="#cfe2ff")
d.text((BX0 + 20, BY1 - 86), "PC13 = 板载 LED", font=f_tiny, fill="#cfe2ff")

for y, name in [
    (240, "3V3"),
    (300, "GND"),
    (380, "PB6  SCL"),
    (440, "PB7  SDA"),
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

# ---------------------------------------------------------------- AHT20
AX0, AY0, AX1, AY1 = 900, 150, 1230, 400
rbox((AX0, AY0, AX1, AY1), "#e8f5e9", "#2e7d32")
d.text((AX0 + 22, AY0 + 12), "AHT20 温湿度模块", font=f_box, fill="#1b5e20")
d.text((AX0 + 22, AY0 + 44), "7 位地址 0x38，引脚 1~4 见丝印", font=f_tiny, fill="#33691e")

for y, name, color in [
    (230, "1  VDD", "#c62828"),
    (280, "2  SDA", "#2e7d32"),
    (330, "3  GND", "#000000"),
    (380, "4  SCL", "#ef6c00"),
]:
    dot(AX0, y, "#37474f")
    d.text((AX0 + 14, y - 12), name, font=f_pin, fill=color)

d.text((AX0, AY1 + 12), "无上拉时：SDA/SCL 各 4.7k 到 3V3", font=f_tiny, fill="#33691e")

wire([(BX1, 240), (620, 240), (620, 230), (AX0, 230)], "#c62828")      # 3V3 -> VDD
label(628, 210, "3V3", "#c62828")

wire([(BX1, 300), (720, 300), (720, 330), (AX0, 330)], "#000000")      # GND -> GND
label(700, 300, "GND", "#000000", anchor="rm")

wire([(BX1, 380), (AX0, 380)], "#ef6c00")                              # PB6 -> SCL
label(700, 380, "SCL", "#ef6c00", anchor="rm")

wire([(BX1, 440), (820, 440), (820, 280), (AX0, 280)], "#2e7d32")      # PB7 -> SDA
label(806, 424, "SDA", "#2e7d32", anchor="rm")

# ---------------------------------------------------------------- 外接 LED
LX0, LY0, LX1, LY1 = 900, 450, 1230, 600
rbox((LX0, LY0, LX1, LY1), "#fff8e1", "#f9a825")
d.text((LX0 + 22, LY0 + 12), "LED2（第二个灯）", font=f_box, fill="#e65100")
d.text((LX0 + 22, LY0 + 40), "PB12 → 330Ω → LED 正极，负极 → GND", font=f_tiny, fill="#8d6e63")

wire([(BX1, 560), (600, 560), (600, 515), (LX0, 515)], "#6a1b9a")
label(608, 548, "PB12", "#6a1b9a")

cx, cy = 1000, 552
d.rectangle([cx - 42, cy - 14, cx - 16, cy + 14], fill="#ffb300", outline="#6d4c41", width=2)
d.text((cx - 50, cy + 18), "330Ω", font=f_tiny, fill="#5d4037")
d.polygon([(cx + 4, cy - 16), (cx + 4, cy + 16), (cx + 40, cy)], fill="#e53935")
d.line([(cx + 40, cy - 16), (cx + 40, cy + 16)], fill="#b71c1c", width=4)
d.text((cx + 52, cy - 40), "LED", font=f_tiny, fill="#b71c1c")
d.line([(cx + 42, cy), (cx + 160, cy)], fill="#000000", width=4)
d.text((cx + 116, cy + 8), "GND", font=f_tiny, fill="#000000")

# ---------------------------------------------------------------- USB-TTL
UX0, UY0, UX1, UY1 = 900, 600, 1230, 760
rbox((UX0, UY0, UX1, UY1), "#e3f2fd", "#1565c0")
d.text((UX0 + 22, UY0 + 12), "USB-TTL（CH340 等）", font=f_box, fill="#0d47a1")
d.text((UX0 + 22, UY0 + 44), "115200 8N1，另一端接电脑 USB", font=f_tiny, fill="#1565c0")

dot(UX0, 698, "#37474f")
d.text((UX0 + 16, 686), "RXD", font=f_pin, fill=INK)
dot(UX0, 736, "#37474f")
d.text((UX0 + 16, 724), "TXD", font=f_pin, fill=INK)

wire([(BX1, 620), (700, 620), (700, 698), (UX0, 698)], "#0277bd")
label(596, 602, "PA9(TX) → RXD", "#0277bd")

wire([(BX1, 680), (770, 680), (770, 736), (UX0, 736)], "#00838f")
label(590, 662, "PA10(RX) ← TXD", "#00838f")

# 共地：从板子底部绕到 TTL
wire([(420, BY1), (420, 806), (1050, 806), (1050, UY1)], "#000000")
label(700, 792, "共地 GND", "#000000", anchor="mm")
dot(1050, UY1, "#000000")

# ---------------------------------------------------------------- DAPLink
DX0, DY0, DX1, DY1 = 110, 845, 430, 935
rbox((DX0, DY0, DX1, DY1), "#f3e5f5", "#6a1b9a")
d.text((DX0 + 22, DY0 + 10), "DAPLink / ST-Link", font=f_box, fill="#4a148c")
d.text((DX0 + 22, DY0 + 46), "SWD 下载调试（可选）", font=f_tiny, fill="#6a1b9a")

wire([(230, BY1), (230, DY0)], "#4e342e")
wire([(330, BY1), (330, DY0)], "#4e342e")
label(240, 800, "SWDIO", "#4e342e")
label(340, 800, "SWCLK", "#4e342e")

# ---------------------------------------------------------------- 图例
lx, ly = 1262, 150
d.text((lx, ly), "图例", font=f_pin, fill=INK)
for i, (c, t) in enumerate([
    ("#c62828", "3.3V 电源"),
    ("#000000", "GND"),
    ("#ef6c00", "I2C SCL"),
    ("#2e7d32", "I2C SDA"),
    ("#0277bd", "UART"),
    ("#6a1b9a", "LED"),
]):
    y = ly + 60 + i * 30
    d.line([(lx, y), (lx + 40, y)], fill=c, width=5)
    d.text((lx + 52, y - 10), t, font=f_tiny, fill=INK)

d.text((60, 955), "跨线处没有圆点表示不相连。AHT20 的 VDD 支持 2.2~5.5V，为与 I2C 电平一致统一用 3.3V，不要接 5V。",
       font=f_note, fill=MUTED)
d.text((60, 985), "DAPLink 的 3V3 与 GND 也接到板子可给板子供电；若板子已用 USB 供电，只接 SWDIO/SWCLK/GND。",
       font=f_note, fill=MUTED)

os.makedirs(r"D:\dsh_list\STM32F103_RTOS_AHT20\02_Docs\images", exist_ok=True)
img.save(r"D:\dsh_list\STM32F103_RTOS_AHT20\02_Docs\images\wiring.png")
print("ok")

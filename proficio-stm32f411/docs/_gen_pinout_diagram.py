"""Mother-board net → STM32 stick diagram + unused pins → STEW-BlackPill-pinout-diagram.jpg"""
from PIL import Image, ImageDraw, ImageFont
import os

OUT = os.path.join(os.path.dirname(__file__), "STEW-BlackPill-pinout-diagram.jpg")

W, H = 1100, 1450
img = Image.new("RGB", (W, H), (255, 255, 255))
d = ImageDraw.Draw(img)


def font(size, bold=False):
    for p in (
        r"C:\Windows\Fonts\consolab.ttf" if bold else r"C:\Windows\Fonts\consola.ttf",
        r"C:\Windows\Fonts\courbd.ttf" if bold else r"C:\Windows\Fonts\cour.ttf",
        r"C:\Windows\Fonts\arialbd.ttf" if bold else r"C:\Windows\Fonts\arial.ttf",
    ):
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()


ft = font(22, True)
fh = font(16, True)
f = font(15)
fs = font(13)

BLACK = (0, 0, 0)
GRAY = (80, 80, 80)
ARROW = (30, 30, 30)
UNUSED = (100, 100, 100)

d.text((40, 20), "Proficio MKII mother-board nets  -->  STM32F411", fill=BLACK, font=ft)
d.text((40, 50), "Daughter board map.  Logic = 3.3V.  Tables in STEW-DAUGHTER-BOARD-PINOUT.md", fill=GRAY, font=fs)

x_net, x_arrow, x_pin, x_note = 40, 420, 520, 680
y = 100
row_h = 34

d.text((x_net, y - 28), "MKII mother-board net", fill=BLACK, font=fh)
d.text((x_pin, y - 28), "STM32", fill=BLACK, font=fh)
d.text((x_note, y - 28), "Notes", fill=BLACK, font=fh)
d.line([40, y - 6, 1060, y - 6], fill=BLACK, width=1)

rows = [
    ("+3.3V", "", "+3.3V", "logic supply to daughter"),
    ("GND", "", "GND", "common with mother board"),
    ("DOUT (from PCM3060)", "in", "PB14", "I2S2ext_SD"),
    ("DIN (to PCM3060)", "out", "PB15", "I2S2_SD"),
    ("BCK1 / BCK2", "out", "PB13", "tie BCK1-BCK2 on PCB"),
    ("LRCK1 / LRCK2", "out", "PB12", "tie LRCK1-LRCK2; NOT GND"),
    ("SCK1 / SCK2", "out", "PC6", "tie SCK1-SCK2 (pad if needed)"),
    ("SDA", "OD", "PB9", "I2C1"),
    ("SCL", "OD", "PB8", "I2C1"),
    ("BS0", "out", "PA7", "band"),
    ("BS1", "out", "PB5", "band"),
    ("BS2", "out", "PB3", "band"),
    ("LED1", "out", "PC13", "on-module LED"),
    ("RX", "out", "PA1", "active-low"),
    ("AMP", "out", "PB4", "active-low"),
    ("BOOT", "in", "PA8", "status / boot sense"),
    ("KEY_0", "in", "PB0", "pull-up; low=key"),
    ("KEY_1", "in", "PB1", "pull-up; low=key"),
    ("PTT", "in", "PA6", "sense, active-low"),
    ("USB D-", "", "PA11", "Black Pill USB-C"),
    ("USB D+", "", "PA12", "Black Pill USB-C"),
]


def draw_arrow(x0, y0, x1):
    d.line([x0, y0, x1 - 8, y0], fill=ARROW, width=2)
    d.polygon([(x1, y0), (x1 - 10, y0 - 5), (x1 - 10, y0 + 5)], fill=ARROW)


for i, (net, direction, pin, note) in enumerate(rows):
    yy = y + i * row_h
    if i == 2 or net.startswith("USB D-"):
        d.line([40, yy - 6, 1060, yy - 6], fill=(180, 180, 180), width=1)
    label = net if not direction else f"{net}  [{direction}]"
    d.text((x_net, yy), label, fill=BLACK, font=f)
    draw_arrow(x_arrow, yy + 8, x_pin)
    d.text((x_pin, yy), pin, fill=BLACK, font=fh)
    d.text((x_note, yy), note, fill=GRAY, font=f)

# ----- UNUSED PINS -----
uy = y + len(rows) * row_h + 24
d.line([40, uy, 1060, uy], fill=BLACK, width=2)
d.text((40, uy + 12), "UNUSED on WeAct Black Pill headers (not assigned to MKII mother-board nets)", fill=BLACK, font=fh)
d.text((40, uy + 38), "Pin", fill=UNUSED, font=fh)
d.text((140, uy + 38), "Header notes / typical use if free", fill=UNUSED, font=fh)
d.line([40, uy + 58, 1060, uy + 58], fill=(180, 180, 180), width=1)

unused = [
    ("PA0",  "User button on module (KEY) — not a MKII mother-board key"),
    ("PA2",  "Free (often USART2_TX debug)"),
    ("PA3",  "Free (often USART2_RX debug)"),
    ("PA4",  "Free (SPI1_NSS — onboard flash footprint on some WeAct revs)"),
    ("PA5",  "Free (SPI1_SCK — onboard flash footprint)"),
    ("PA9",  "Free (USART1_TX)"),
    ("PA10", "Free (USART1_RX)"),
    ("PA15", "Free"),
    ("PB2",  "Free (BOOT1 on some STM32; check board)"),
    ("PB6",  "Free (I2C1_SCL alt / TIM)"),
    ("PB7",  "Free (I2C1_SDA alt / TIM)"),
    ("PB10", "Free (USART3_TX / I2C2_SCL)"),
    ("PC14", "OSC32_IN — usually crystal; leave alone unless you know"),
    ("PC15", "OSC32_OUT — usually crystal; leave alone unless you know"),
    ("NRST", "Reset input (header R) — not a GPIO net"),
    ("VBAT", "Backup domain (header VB)"),
    ("5V",   "USB 5V rail on module headers ONLY — not MKII GPIO; do not use for 3.3V I/O"),
]

for i, (pin, note) in enumerate(unused):
    yy = uy + 68 + i * 28
    d.text((40, yy), pin, fill=UNUSED, font=fh)
    d.text((140, yy), note, fill=UNUSED, font=f)

fy = uy + 68 + len(unused) * 28 + 16
d.line([40, fy, 1060, fy], fill=BLACK, width=1)
footer = [
    "EXAMPLES:  DOUT --> PB14   DIN --> PB15   LRCK1/LRCK2 --> PB12   KEY_0 --> PB0",
    "PB12 = LRCK only — do not tie PB12 to GND.  KEY_0=PB0, KEY_1=PB1; no KEY_7.  PA1 = RX only.",
    "PC6 (SCK/MCLK) is used by MKII but may need a wire/pad on bare Black Pill (not always on 2x20).",
]
for i, line in enumerate(footer):
    d.text((40, fy + 12 + i * 24), line, fill=BLACK, font=f)

img.save(OUT, "JPEG", quality=95)
print("Wrote", OUT, "h=", H)

"""Reference drawing for the stand-alone keyer: parts as boxes, pins with net labels.
Writes keyer-standalone/hardware/keyer-standalone-reference.svg"""
import html, sys

OUT = sys.argv[1]

# (title, [(ref, part, [(pin, net), ...]), ...])
SECTIONS = [
 ("POWER (9 V battery)", [
  ("J1", "9 V battery header", [("1", "BATT+"), ("2", "GND")]),
  ("J2", "power switch header", [("1", "BATT+"), ("2", "SW_OUT")]),
  ("Q1", "AO3401A reverse-battery", [("D", "SW_OUT"), ("S", "VBAT"), ("G", "Q1_G")]),
  ("R1", "100k", [("1", "Q1_G"), ("2", "GND")]),
  ("U1", "MCP1703T-5002E/CB", [("3 VIN", "VBAT"), ("2 VOUT", "+5V"), ("1 GND", "GND")]),
  ("C1", "1uF", [("1", "VBAT"), ("2", "GND")]),
  ("C2", "1uF", [("1", "+5V"), ("2", "GND")]),
  ("C3", "10uF", [("1", "+5V"), ("2", "GND")]),
  ("R2", "1M", [("1", "VBAT"), ("2", "VSENSE")]),
  ("R3", "470k", [("1", "VSENSE"), ("2", "GND")]),
  ("C4", "100nF", [("1", "VSENSE"), ("2", "GND")]),
 ]),
 ("PIC16F18326-I/P (DIP-14)", [
  ("U2", "PIC16F18326-I/P", [
    ("1 VDD", "+5V"), ("2 RA5", "PAD_A"), ("3 RA4", "PAD_B"), ("4 RA3/MCLR", "FN"),
    ("5 RC5", "KEY_K"), ("6 RC4", "PTT_K"), ("7 RC3", "TX_CW"),
    ("8 RC2", "SIDE_TONE"), ("9 RC1", "AMP_EN"), ("10 RC0", "VSENSE"), ("11 RA2", "SPEED"),
    ("12 RA1", "LED_PGC"), ("13 RA0", "RX_CW_PGD"), ("14 VSS", "GND")]),
  ("C5", "100nF at pin 1", [("1", "+5V"), ("2", "GND")]),
  ("J8", "ICSP (PICkit)", [("1", "FN"), ("2", "+5V"), ("3", "GND"), ("4", "RX_CW_PGD"),
                           ("5", "LED_PGC"), ("6", "nc")]),
 ]),
 ("CONTROLS", [
  ("R4", "10k pull-up", [("1", "+5V"), ("2", "FN")]),
  ("R5", "1k", [("1", "FN"), ("2", "FN_SW")]),
  ("J5", "FN button header", [("1", "FN_SW"), ("2", "GND")]),
  ("J6", "SPEED pot 100k lin", [("1", "+5V"), ("2", "SPEED"), ("3", "GND")]),
  ("C6", "100nF", [("1", "SPEED"), ("2", "GND")]),
  ("R6", "1k", [("1", "LED_PGC"), ("2", "LED_A")]),
  ("J7", "status LED header", [("1 A", "LED_A"), ("2 K", "GND")]),
  ("Q2", "AO3401A amp switch", [("S", "+5V"), ("D", "AMP_VCC"), ("G", "Q2_G")]),
  ("R7", "100R", [("1", "AMP_EN"), ("2", "Q2_G")]),
  ("R8", "100k", [("1", "Q2_G"), ("2", "+5V")]),
 ]),
 ("PADDLE IN", [
  ("J3", "SJ1-3535NG PADDLE", [("Tip", "PAD_A_RAW"), ("Ring", "PAD_B_RAW"), ("Sleeve", "GND")]),
  ("U3", "PRTR5V0U2X", [("1 GND", "GND"), ("2 IO1", "PAD_A_RAW"), ("3 IO2", "PAD_B_RAW"), ("4 VCC", "+5V")]),
  ("R9", "1k", [("1", "PAD_A_RAW"), ("2", "PAD_A")]),
  ("R10", "1k", [("1", "PAD_B_RAW"), ("2", "PAD_B")]),
  ("R11", "10k", [("1", "+5V"), ("2", "PAD_A")]),
  ("R12", "10k", [("1", "+5V"), ("2", "PAD_B")]),
  ("C7", "1nF", [("1", "PAD_A"), ("2", "GND")]),
  ("C8", "1nF", [("1", "PAD_B"), ("2", "GND")]),
 ]),
 ("KEY OUT (isolated)", [
  ("R13", "560R", [("1", "+5V"), ("2", "U4_A")]),
  ("U4", "VO1400AEFTR KEY", [("1 A", "U4_A"), ("2 C", "KEY_K"), ("3 S", "KEY_COM"), ("4 S", "KEY_TIP")]),
  ("R14", "560R", [("1", "+5V"), ("2", "U5_A")]),
  ("U5", "VO1400AEFTR PTT", [("1 A", "U5_A"), ("2 C", "PTT_K"), ("3 S", "KEY_COM"), ("4 S", "PTT_RING")]),
  ("J4", "SJ1-3535NG KEY OUT", [("Tip", "KEY_TIP"), ("Ring", "PTT_RING"), ("Sleeve", "KEY_COM")]),
 ]),
 ("SIDETONE (as Rev 2 sheet 2)", [
  ("R15", "1k5", [("1", "SIDE_TONE"), ("2", "ST1")]),
  ("C9", "100nF", [("1", "ST1"), ("2", "GND")]),
  ("R16", "1k5", [("1", "ST1"), ("2", "ST2")]),
  ("C10", "100nF", [("1", "ST2"), ("2", "GND")]),
  ("R17", "1k5", [("1", "ST2"), ("2", "ST3")]),
  ("C11", "100nF", [("1", "ST3"), ("2", "GND")]),
  ("L1", "1mH", [("1", "ST3"), ("2", "ST4")]),
  ("C12", "470nF", [("1", "ST4"), ("2", "GND")]),
  ("C13", "10nF", [("1", "ST3"), ("2", "VOL_TOP")]),
  ("J9", "VOLUME pot 10k audio", [("1", "VOL_TOP"), ("2", "VOL_W"), ("3", "GND")]),
  ("C14", "10nF", [("1", "VOL_W"), ("2", "GND")]),
 ]),
 ("LM386 on switched 5 V", [
  ("U6", "LM386MX-1/NOPB", [("1 GAIN", "GAIN1"), ("2 IN-", "GND"), ("3 IN+", "VOL_W"), ("4 GND", "GND"),
                            ("5 VOUT", "AMP_OUT"), ("6 VS", "AMP_VCC"), ("7 BYP", "BYP"), ("8 GAIN", "GAIN8")]),
  ("R18", "680R", [("1", "GAIN1"), ("2", "G_MID")]),
  ("C15", "10uF", [("+", "G_MID"), ("-", "GAIN8")]),
  ("C16", "100nF", [("1", "BYP"), ("2", "GND")]),
  ("C17", "100nF", [("1", "AMP_VCC"), ("2", "GND")]),
  ("C18", "47uF", [("+", "AMP_VCC"), ("-", "GND")]),
  ("C19", "100nF", [("1", "AMP_OUT"), ("2", "ZOB")]),
  ("R19", "10R", [("1", "ZOB"), ("2", "GND")]),
  ("C20", "220uF", [("+", "AMP_OUT"), ("-", "SIDE_TONE_OUT")]),
  ("C21", "10nF", [("1", "SIDE_TONE_OUT"), ("2", "GND")]),
 ]),
 ("AUDIO SWITCH AND JACKS", [
  ("R20", "1k2", [("1", "+5V"), ("2", "U7_A")]),
  ("D1", "1.8V zener", [("K", "U7_A"), ("A", "GND")]),
  ("U7", "VO1400AEFTR sidetone (NO)", [("1 A", "U7_A"), ("2 C", "TX_CW"), ("4 S", "SIDE_TONE_OUT"), ("3 S", "AUDIO_OUT")]),
  ("R21", "1k2", [("1", "+5V"), ("2", "U8_A")]),
  ("D2", "1.8V zener", [("K", "U8_A"), ("A", "GND")]),
  ("U8", "CPC1117NTR RX (NC)", [("1 A", "U8_A"), ("2 C", "RX_CW_PGD"), ("4 S", "RX_AUDIO"), ("3 S", "AUDIO_OUT")]),
  ("J10", "SJ1-3533NG PHONES", [("Tip", "AUDIO_OUT"), ("Sleeve", "GND")]),
  ("J11", "SJ1-3533NG RIG AUDIO IN", [("Tip", "RX_AUDIO"), ("Sleeve", "GND")]),
  ("J12", "AUDIO_OUT header (opt)", [("1", "AUDIO_OUT"), ("2", "GND")]),
 ]),
]

# Colour nets that matter
POWER = {"+5V", "VBAT", "BATT+", "AMP_VCC"}
GROUND = {"GND"}
ISO = {"KEY_COM", "KEY_TIP", "PTT_RING"}

PIN_H = 18
BOX_W = 196
COL_W = 330
TOP = 70

def part_height(pins):
    return 28 + PIN_H * len(pins) + 8

out = []
x0 = 20
cols = 4
col_y = [TOP] * cols
placed = []
for title, parts in SECTIONS:
    h = 26 + sum(part_height(p[2]) + 10 for p in parts)
    c = min(range(cols), key=lambda i: col_y[i])
    placed.append((c, col_y[c], title, parts, h))
    col_y[c] += h + 24
W = x0 + cols * COL_W + 20
H = max(col_y) + 40

def net_color(n):
    if n in GROUND: return "#555"
    if n in POWER: return "#c0392b"
    if n in ISO: return "#8e44ad"
    return "#1f5fa8"

out.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="Arial, Helvetica, sans-serif">')
out.append(f'<rect width="{W}" height="{H}" fill="#fff"/>')
out.append(f'<text x="{x0}" y="32" font-size="22" font-weight="bold">Stand-alone keyer, reference drawing (draft 1, 2026-11-05)</text>')
out.append(f'<text x="{x0}" y="54" font-size="13" fill="#444">Net labels connect pins with the same name (as EasyEDA net labels). '
           f'Red = power, grey = GND, purple = isolated key output. Source: hardware/NETLIST.md and BOM.csv.</text>')
for c, y, title, parts, h in placed:
    x = x0 + c * COL_W
    out.append(f'<rect x="{x-6}" y="{y-4}" width="{COL_W-14}" height="{h}" fill="none" stroke="#bbb" stroke-dasharray="4 3" rx="6"/>')
    out.append(f'<text x="{x}" y="{y+14}" font-size="14" font-weight="bold" fill="#222">{html.escape(title)}</text>')
    yy = y + 26
    for ref, part, pins in parts:
        ph = part_height(pins)
        out.append(f'<rect x="{x}" y="{yy}" width="{BOX_W}" height="{ph-8}" fill="#f7f9fc" stroke="#333" rx="3"/>')
        out.append(f'<text x="{x+6}" y="{yy+15}" font-size="13" font-weight="bold">{html.escape(ref)}</text>')
        out.append(f'<text x="{x+6+len(ref)*9+6}" y="{yy+15}" font-size="11" fill="#333">{html.escape(part)}</text>')
        py = yy + 28
        for pin, net in pins:
            cy = py + 6
            out.append(f'<text x="{x+BOX_W-6}" y="{cy+4}" font-size="11" text-anchor="end" fill="#000">{html.escape(pin)}</text>')
            out.append(f'<line x1="{x+BOX_W}" y1="{cy}" x2="{x+BOX_W+22}" y2="{cy}" stroke="#000"/>')
            col = net_color(net)
            out.append(f'<text x="{x+BOX_W+26}" y="{cy+4}" font-size="11" font-weight="bold" fill="{col}">{html.escape(net)}</text>')
            py += PIN_H
        yy += ph + 10
out.append('</svg>')
open(OUT, "w", encoding="utf-8").write("\n".join(out))
print("wrote", OUT, W, "x", H)

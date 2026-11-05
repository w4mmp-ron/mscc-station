"""Stand-alone keyer: Eagle 9 schematic (.sch, XML) drawn in the style of the Rev 2 sheets.

Proper symbols, local chains wired, net labels / GND / +5V symbols between blocks.
No instance rotation (each orientation is its own symbol), so importers can't get it wrong.

    python make_eagle.py ../keyer-standalone.sch [preview.svg]

Checks (fails on any): every net matches NETLIST (EXPECTED below), every pin is wired,
no wire touches a pin or wire of another net, no wire runs through a part body.
"""
import math
import sys
from collections import defaultdict
from xml.sax.saxutils import quoteattr, escape

MM = 2.54          # one grid step in mm; all layout below is in grid steps

# ============================================================ symbols
# graphics: ("l", x1, y1, x2, y2, w) | ("a", x1, y1, x2, y2, curve) | ("t", x, y, text, size, layer)
# pins: (name, x, y, dx, dy, length, visible)  dx,dy = direction from the pin end into the part
SYM = {}

def sym(name, graphics, pins, body):
    SYM[name] = {"g": graphics, "p": pins, "body": body}

def two_v(name, extra, pin_names=("1", "2")):
    g = [("l", 0, 1, 0, 0.6 if name.startswith(("C", "CP")) else 1, 0.15)] if False else []
    sym(name, extra + [("t", 0.7, 0.35, ">NAME", 1.524, 95), ("t", 0.7, -0.75, ">VALUE", 1.524, 96)],
        [(pin_names[0], 0, 2, 0, -1, "short", "off"), (pin_names[1], 0, -2, 0, 1, "short", "off")],
        (-0.45, -1, 0.45, 1))

def two_h(name, extra, pin_names=("1", "2")):
    sym(name, extra + [("t", -1, 1.35, ">NAME", 1.524, 95), ("t", -1, 0.6, ">VALUE", 1.524, 96)],
        [(pin_names[0], -2, 0, 1, 0, "short", "off"), (pin_names[1], 2, 0, -1, 0, "short", "off")],
        (-1, -0.45, 1, 0.45))

W = 0.1524 / MM   # thin line in grid units
TH = 0.254 / MM
RES_V = [("l", -0.35, -1, 0.35, -1, TH), ("l", 0.35, -1, 0.35, 1, TH), ("l", 0.35, 1, -0.35, 1, TH), ("l", -0.35, 1, -0.35, -1, TH)]
RES_H = [("l", -1, -0.35, 1, -0.35, TH), ("l", 1, -0.35, 1, 0.35, TH), ("l", 1, 0.35, -1, 0.35, TH), ("l", -1, 0.35, -1, -0.35, TH)]
CAP_V = [("l", 0, 1, 0, 0.2, TH), ("l", 0, -1, 0, -0.2, TH), ("l", -0.7, 0.2, 0.7, 0.2, 0.4 / MM), ("l", -0.7, -0.2, 0.7, -0.2, 0.4 / MM)]
CAP_H = [("l", -1, 0, -0.2, 0, TH), ("l", 1, 0, 0.2, 0, TH), ("l", -0.2, -0.7, -0.2, 0.7, 0.4 / MM), ("l", 0.2, -0.7, 0.2, 0.7, 0.4 / MM)]
PLUS_V = [("t", -0.95, 0.35, "+", 1.27, 94)]
PLUS_H = [("t", -0.9, 0.35, "+", 1.27, 94)]
IND_V = [("a", 0, 1, 0, 0.5, -180), ("a", 0, 0.5, 0, 0, -180), ("a", 0, 0, 0, -0.5, -180), ("a", 0, -0.5, 0, -1, -180)]
ZEN_V = [("l", 0, 1, 0, 0.4, TH), ("l", 0, -1, 0, -0.4, TH), ("l", -0.55, -0.4, 0.55, -0.4, TH),
         ("l", 0.55, -0.4, 0, 0.4, TH), ("l", 0, 0.4, -0.55, -0.4, TH), ("l", -0.55, 0.4, 0.55, 0.4, TH),
         ("l", -0.55, 0.4, -0.8, 0.15, TH), ("l", 0.55, 0.4, 0.8, 0.65, TH)]

two_v("R_V", RES_V); two_h("R_H", RES_H)
two_v("C_V", CAP_V); two_h("C_H", CAP_H)
two_v("CP_V", CAP_V + PLUS_V, ("P", "N")); two_h("CP_H", CAP_H + PLUS_H, ("P", "N"))
two_v("L_V", IND_V)
two_v("Z_V", ZEN_V, ("K", "A"))

# P-channel MOSFET, D left / S right / G bottom
PF_H_G = [("l", -1, 0, -0.6, 0, TH), ("l", -0.6, 0, -0.6, 0.35, TH), ("l", 1, 0, 0.6, 0, TH), ("l", 0.6, 0, 0.6, 0.35, TH),
          ("l", -0.8, 0.35, -0.4, 0.35, TH), ("l", -0.2, 0.35, 0.2, 0.35, TH), ("l", 0.4, 0.35, 0.8, 0.35, TH),
          ("l", -0.8, -0.1, 0.8, -0.1, TH), ("l", 0, -0.1, 0, -1, TH),
          ("l", 0, 0.35, 0, 0.85, TH), ("l", 0, 0.85, -0.2, 0.6, TH), ("l", 0, 0.85, 0.2, 0.6, TH)]
sym("PFET_H", PF_H_G + [("t", -1.6, 1.1, ">NAME", 1.524, 95), ("t", 1.2, -1.0, ">VALUE", 1.524, 96)],
    [("D", -2, 0, 1, 0, "short", "pad"), ("S", 2, 0, -1, 0, "short", "pad"), ("G", 0, -2, 0, 1, "short", "pad")],
    (-1, -1, 1, 0.9))
def swap(g):  # (x,y) -> (y,x): D bottom, S top, G left
    out = []
    for e in g:
        if e[0] in ("l", "a"):
            out.append((e[0], e[2], e[1], e[4], e[3], e[5] if e[0] == "l" else -e[5]))
    return out
sym("PFET_V", swap(PF_H_G) + [("t", 1.1, 0.4, ">NAME", 1.524, 95), ("t", 1.1, -0.6, ">VALUE", 1.524, 96)],
    [("D", 0, -2, 0, 1, "short", "pad"), ("S", 0, 2, 0, -1, "short", "pad"), ("G", -2, 0, 1, 0, "short", "pad")],
    (-1, -1, 0.9, 1))

# supplies (pin at the symbol origin)
sym("SUP_GND", [("l", 0, 0, 0, -0.5, TH), ("l", -0.6, -0.5, 0.6, -0.5, TH), ("l", -0.4, -0.72, 0.4, -0.72, TH),
                ("l", -0.2, -0.94, 0.2, -0.94, TH), ("t", -0.6, -1.7, ">VALUE", 1.27, 96)],
    [("GND", 0, 0, 0, -1, "point", "off")], (-0.6, -1.0, 0.6, -0.3))
sym("SUP_P5V", [("l", 0, 0, 0, 0.6, TH), ("l", -0.6, 0.6, 0.6, 0.6, TH), ("t", -0.65, 0.9, ">VALUE", 1.27, 96)],
    [("+5V", 0, 0, 0, 1, "point", "off")], (-0.6, 0.3, 0.6, 0.6))

def box(name, wb, left=(), right=(), top=(), bottom=(), pinvis="both"):
    hb = max(len(left), len(right)) + 1
    wb = max(wb, max(len(top), len(bottom)) + 1)
    g = [("l", 0, 0, wb, 0, TH), ("l", wb, 0, wb, -hb, TH), ("l", wb, -hb, 0, -hb, TH), ("l", 0, -hb, 0, 0, TH),
         ("t", 0, 0.45, ">NAME", 1.778, 95), ("t", 0, -hb - 0.9, ">VALUE", 1.524, 96)]
    pins = []
    for i, n in enumerate(left):
        pins.append((n, -1, -(i + 1), 1, 0, "short", pinvis))
    for i, n in enumerate(right):
        pins.append((n, wb + 1, -(i + 1), -1, 0, "short", pinvis))
    for j, n in enumerate(top):
        pins.append((n, j + 1, 1, 0, -1, "short", pinvis))
    for j, n in enumerate(bottom):
        pins.append((n, j + 1, -hb - 1, 0, 1, "short", pinvis))
    sym(name, g, pins, (0, -hb, wb, 0))

box("MCP1703", 6, left=["VIN"], right=["VOUT"], bottom=["GND"])
box("PIC16F18326", 10, left=["VDD", "RA5", "RA4", "RA3_MCLR", "RC5", "RC4", "RC3"],
    right=["VSS", "RA0_PGD", "RA1_PGC", "RA2", "RC0", "RC1", "RC2"])
box("PRTR5V0U2X", 4, left=["IO1", "IO2"], top=["VCC"], bottom=["GND"])
box("PHOTOMOS", 6, left=["A", "C"], right=["S4", "S3"])
box("LM386", 8, left=["IN_P", "IN_N", "GND", "GAIN1"], right=["VS", "GAIN8", "VOUT", "BYP"])
box("JACK3_R", 4, right=["TIP", "RING", "SLEEVE"], pinvis="pin")
box("JACK3_L", 4, left=["TIP", "RING", "SLEEVE"], pinvis="pin")
box("JACK2_L", 4, left=["TIP", "SLEEVE"], pinvis="pin")
box("HDR2_R", 4, right=["1", "2"]); box("HDR2_L", 4, left=["1", "2"])
box("HDR3_R", 4, right=["1", "2", "3"]); box("HDR3_L", 4, left=["1", "2", "3"])
box("HDR6_R", 4, right=["1", "2", "3", "4", "5", "6"])

# pads (for the placeholder packages): symbol pin -> pad
PADS = {
    "R_V": {"1": "1", "2": "2"}, "R_H": {"1": "1", "2": "2"}, "C_V": {"1": "1", "2": "2"}, "C_H": {"1": "1", "2": "2"},
    "CP_V": {"P": "1", "N": "2"}, "CP_H": {"P": "1", "N": "2"}, "L_V": {"1": "1", "2": "2"}, "Z_V": {"K": "1", "A": "2"},
    "PFET_H": {"G": "1", "S": "2", "D": "3"}, "PFET_V": {"G": "1", "S": "2", "D": "3"},
    "MCP1703": {"GND": "1", "VOUT": "2", "VIN": "3"},
    "PIC16F18326": {"VDD": "1", "RA5": "2", "RA4": "3", "RA3_MCLR": "4", "RC5": "5", "RC4": "6", "RC3": "7",
                    "RC2": "8", "RC1": "9", "RC0": "10", "RA2": "11", "RA1_PGC": "12", "RA0_PGD": "13", "VSS": "14"},
    "PRTR5V0U2X": {"GND": "1", "IO1": "2", "IO2": "3", "VCC": "4"},
    "PHOTOMOS": {"A": "1", "C": "2", "S3": "3", "S4": "4"},
    "LM386": {"GAIN1": "1", "IN_N": "2", "IN_P": "3", "GND": "4", "VOUT": "5", "VS": "6", "BYP": "7", "GAIN8": "8"},
    "JACK3_R": {"TIP": "T", "RING": "R", "SLEEVE": "S"}, "JACK3_L": {"TIP": "T", "RING": "R", "SLEEVE": "S"},
    "JACK2_L": {"TIP": "T", "SLEEVE": "S"},
    "HDR2_R": {"1": "1", "2": "2"}, "HDR2_L": {"1": "1", "2": "2"}, "HDR3_R": {"1": "1", "2": "2", "3": "3"},
    "HDR3_L": {"1": "1", "2": "2", "3": "3"}, "HDR6_R": {str(i): str(i) for i in range(1, 7)},
}
PREFIX = {"R_V": "R", "R_H": "R", "C_V": "C", "C_H": "C", "CP_V": "C", "CP_H": "C", "L_V": "L", "Z_V": "D",
          "PFET_H": "Q", "PFET_V": "Q", "SUP_GND": "GND", "SUP_P5V": "P+"}

# ============================================================ model
parts = {}          # ref -> dict(sym, value, mpn, lcsc, sheet, x, y, supply)
wires = []          # (sheet, net, x1, y1, x2, y2)
labels = []         # (sheet, net, x, y)
pinnet = {}         # (ref, pin) -> net
texts = []          # (sheet, x, y, text, size)
sheet = 1
_sup = defaultdict(int)

def place(ref, symname, x, y, value="", mpn="", lcsc=""):
    parts[ref] = dict(sym=symname, value=value, mpn=mpn, lcsc=lcsc, sheet=sheet, x=x, y=y, supply=False)

def P(ref, pin):
    p = parts[ref]
    for n, px, py, dx, dy, *_ in SYM[p["sym"]]["p"]:
        if n == pin:
            return (p["x"] + px, p["y"] + py)
    raise KeyError(f"{ref}.{pin}")

def outdir(ref, pin):
    for n, px, py, dx, dy, *_ in SYM[parts[ref]["sym"]]["p"]:
        if n == pin:
            return (-dx, -dy)

def net(ref, pin, name):
    pinnet[(ref, pin)] = name

def wire(name, *pts):
    """Polyline; points are (x, y) or (ref, pin) tuples. Marks the pins with the net."""
    xy = []
    for q in pts:
        if isinstance(q[0], str):
            net(q[0], q[1], name)
            xy.append(P(*q))
        else:
            xy.append(q)
    for (x1, y1), (x2, y2) in zip(xy, xy[1:]):
        wires.append((sheet, name, x1, y1, x2, y2))

def label(name, x, y):
    labels.append((sheet, name, x, y))

def stub(ref, pin, name, length=None, at=None):
    """Short wire out of a pin with a net label at its end (or at 'at' grid steps from the pin)."""
    dx, dy = outdir(ref, pin)
    if length is None:
        length = 6 if dx > 0 else (7 if dx < 0 else 3)
    x, y = P(ref, pin)
    ex, ey = x + dx * length, y + dy * length
    wire(name, (ref, pin), (ex, ey))
    if dx < 0:
        label(name, ex, ey)            # text runs right, over the wire, towards the pin
    else:
        label(name, ex, ey)

def supply(kind, x, y):
    _sup[kind] += 1
    ref = ("GND" if kind == "GND" else "P5V") + str(_sup[kind])
    parts[ref] = dict(sym="SUP_GND" if kind == "GND" else "SUP_P5V", value="GND" if kind == "GND" else "+5V",
                      mpn="", lcsc="", sheet=sheet, x=x, y=y, supply=True)
    net(ref, "GND" if kind == "GND" else "+5V", "GND" if kind == "GND" else "+5V")
    return ref

def gnd(ref, pin, length=1):
    dx, dy = outdir(ref, pin)
    x, y = P(ref, pin)
    ex, ey = x + dx * length, y + dy * length
    s = supply("GND", ex, ey)
    wire("GND", (ref, pin), (s, "GND"))

def p5v(ref, pin, length=1):
    dx, dy = outdir(ref, pin)
    x, y = P(ref, pin)
    ex, ey = x + dx * length, y + dy * length
    s = supply("+5V", ex, ey)
    wire("+5V", (ref, pin), (s, "+5V"))

def title(x, y, t, size=2.1844):
    texts.append((sheet, x, y, t, size))

# ============================================================ sheet 1
sheet = 1
title(0, 8, "STAND-ALONE KEYER - sheet 1/2: power, PIC, paddle, key out, controls  (draft 1, 2026-11-05)", 3.0)

# ---- POWER (9 V battery)
title(0, 2.5, "POWER - 9 V battery")
place("J1", "HDR2_R", 0, 0, "9V BATTERY")
place("J2", "HDR2_R", 0, -6, "POWER SW")
place("Q1", "PFET_H", 11, -8, "AO3401A", "AO3401A", "C15127")
place("R1", "R_V", 11, -13, "100k")
place("U1", "MCP1703", 23, -7, "MCP1703T-5002E/CB", "MCP1703T-5002E/CB", "C50754")
place("C1", "C_V", 18, -11, "1uF")
place("R2", "R_V", 15, -12, "1M")
place("R3", "R_V", 15, -19, "470k")
place("C4", "C_V", 19, -18, "100nF")
place("C2", "C_V", 32, -11, "1uF")
place("C3", "C_V", 35, -11, "10uF")
wire("BATT+", ("J1", "1"), (8, -1), (8, -7), ("J2", "1")); label("BATT+", 5.5, -1)
gnd("J1", "2")
wire("SW_OUT", ("J2", "2"), ("Q1", "D"))
wire("Q1_G", ("Q1", "G"), ("R1", "1"))
gnd("R1", "2")
wire("VBAT", ("Q1", "S"), (15, -8), (18, -8), ("U1", "VIN"))
wire("VBAT", (15, -8), ("R2", "1"))
wire("VBAT", (18, -8), ("C1", "1"))
label("VBAT", 19, -8)
gnd("C1", "2"); gnd("U1", "GND")
wire("VSENSE", ("R2", "2"), (15, -16), ("R3", "1"))
wire("VSENSE", (15, -16), ("C4", "1"), (23, -16)); label("VSENSE", 23, -16)
gnd("R3", "2"); gnd("C4", "2")
wire("+5V", ("U1", "VOUT"), (32, -8), (35, -8), (37, -8))
wire("+5V", (32, -8), ("C2", "1")); wire("+5V", (35, -8), ("C3", "1"))
s = supply("+5V", 37, -8); wire("+5V", (37, -8), (s, "+5V"))
gnd("C2", "2"); gnd("C3", "2")
texts.append((sheet, 0, -26, "Q1: reverse-battery protection (body diode, then the FET turns on).", 1.524))
texts.append((sheet, 0, -27, "VSENSE = VBAT x 0.32 (9.5 V -> 3.0 V; LB 6.5 V; stop 5.8 V).", 1.524))

# ---- PIC
ox = 55
title(ox - 8, 2.5, "PIC16F18326 KEYER")
place("U2", "PIC16F18326", ox, -2, "PIC16F18326-I/P", "PIC16F18326-I/P", "C645826")
for pin, nm in [("VDD", "+5V"), ("RA5", "PAD_A"), ("RA4", "PAD_B"), ("RA3_MCLR", "FN"), ("RC5", "KEY_K"),
                ("RC4", "PTT_K"), ("RC3", "TX_CW"), ("VSS", "GND"), ("RA0_PGD", "RX_CW_PGD"), ("RA1_PGC", "LED_PGC"),
                ("RA2", "SPEED"), ("RC0", "VSENSE"), ("RC1", "AMP_EN"), ("RC2", "SIDE_TONE")]:
    stub("U2", pin, nm)
place("C5", "C_V", ox + 5, -16, "100nF")
p5v("C5", "1"); gnd("C5", "2")
texts.append((sheet, ox + 7, -15, "C5 close to U2 pin 1", 1.524))
place("J8", "HDR6_R", ox - 4, -21, "ICSP")
for pin, nm in [("1", "FN"), ("2", "+5V"), ("3", "GND"), ("4", "RX_CW_PGD"), ("5", "LED_PGC")]:
    stub("J8", pin, nm)
texts.append((sheet, ox + 9, -27, "J8 pin 6: not connected", 1.524))

# ---- KEY OUT
ox = 100
title(ox - 5, 2.5, "KEY OUT - isolated (modern rigs: 60 V / 100 mA)")
place("R13", "R_H", ox + 1, -7, "560R")
place("U4", "PHOTOMOS", ox + 5, -6, "VO1400AEFTR", "VO1400AEFTR", "C142551")
place("R14", "R_H", ox + 1, -14, "560R")
place("U5", "PHOTOMOS", ox + 5, -13, "VO1400AEFTR", "VO1400AEFTR", "C142551")
place("J4", "JACK3_L", ox + 22, -6, "KEY OUT SJ1-3535NG", "SJ1-3535NG", "C4991610")
p5v("R13", "1"); wire("U4_A", ("R13", "2"), ("U4", "A"))
stub("U4", "C", "KEY_K", 8)
p5v("R14", "1"); wire("U5_A", ("R14", "2"), ("U5", "A"))
stub("U5", "C", "PTT_K", 8)
wire("KEY_TIP", ("U4", "S4"), ("J4", "TIP"))
wire("PTT_RING", ("U5", "S4"), (ox + 17, -14), (ox + 17, -8), ("J4", "RING"))
wire("KEY_COM", ("U4", "S3"), (ox + 13, -8), (ox + 13, -9), (ox + 19, -9), ("J4", "SLEEVE"))
wire("KEY_COM", ("U5", "S3"), (ox + 19, -15), (ox + 19, -9))
label("KEY_COM", ox + 13.5, -9)
texts.append((sheet, ox + 2, -19, "Tip = key, ring = PTT / amp, sleeve = common.", 1.524))
texts.append((sheet, ox + 2, -20, "KEY_COM is NOT connected to the keyer GND.", 1.524))

# ---- PADDLE
oy = -34
title(0, oy + 3, "PADDLE IN")
place("J3", "JACK3_R", 0, oy, "PADDLE SJ1-3535NG", "SJ1-3535NG", "C4991610")
place("R9", "R_H", 14, oy - 1, "1k")
place("R11", "R_V", 18, oy + 3, "10k")
place("C7", "C_V", 18, oy - 5, "1nF")
place("R10", "R_H", 14, oy - 12, "1k")
place("R12", "R_V", 26, oy - 8, "10k")
place("C8", "C_V", 26, oy - 16, "1nF")
place("U3", "PRTR5V0U2X", 37, oy - 2, "PRTR5V0U2X", "PRTR5V0U2X,215", "C12333")
wire("PAD_A_RAW", ("J3", "TIP"), ("R9", "1")); label("PAD_A_RAW", 5.5, oy - 1)
wire("PAD_A", ("R9", "2"), (18, oy - 1), (22, oy - 1)); label("PAD_A", 22, oy - 1)
wire("PAD_A", (18, oy - 1), ("R11", "2")); wire("PAD_A", (18, oy - 1), ("C7", "1"))
p5v("R11", "1"); gnd("C7", "2")
wire("PAD_B_RAW", ("J3", "RING"), (7, oy - 2), (7, oy - 12), ("R10", "1")); label("PAD_B_RAW", 7.5, oy - 12)
gnd("J3", "SLEEVE")
wire("PAD_B", ("R10", "2"), (26, oy - 12), (30, oy - 12)); label("PAD_B", 30, oy - 12)
wire("PAD_B", (26, oy - 12), ("R12", "2")); wire("PAD_B", (26, oy - 12), ("C8", "1"))
p5v("R12", "1"); gnd("C8", "2")
stub("U3", "IO1", "PAD_A_RAW", 7); stub("U3", "IO2", "PAD_B_RAW", 7)
p5v("U3", "VCC"); gnd("U3", "GND")
texts.append((sheet, 30, oy - 9, "U3 sits at the jack (ESD).", 1.524))
texts.append((sheet, 0, oy - 21, "Tip = dit, ring = dah (swapped by the reverse setting).", 1.524))

# ---- CONTROLS
ox, oy = 50, -40
title(ox - 4, oy + 5, "CONTROLS (panel parts on headers)")
place("R4", "R_V", ox + 3, oy, "10k")
place("R5", "R_H", ox + 7, oy - 4, "1k")
place("J5", "HDR2_L", ox + 12, oy - 3, "FN BUTTON")
p5v("R4", "1")
wire("FN", ("R4", "2"), (ox + 3, oy - 4), ("R5", "1"))
wire("FN", (ox + 3, oy - 4), (ox - 2, oy - 4)); label("FN", ox - 2, oy - 4)
wire("FN_SW", ("R5", "2"), ("J5", "1")); gnd("J5", "2")
place("J6", "HDR3_R", ox, oy - 10, "SPEED 100k LIN")
place("C6", "C_V", ox + 8, oy - 15, "100nF")
p5v("J6", "1")
wire("SPEED", ("J6", "2"), (ox + 8, oy - 12), (ox + 12, oy - 12)); label("SPEED", ox + 12, oy - 12)
wire("SPEED", (ox + 8, oy - 12), ("C6", "1"))
gnd("J6", "3"); gnd("C6", "2")
place("J7", "HDR2_R", ox, oy - 21, "STATUS LED")
place("R6", "R_H", ox + 9, oy - 22, "1k")
wire("LED_A", ("J7", "1"), ("R6", "1"))
wire("LED_PGC", ("R6", "2"), (ox + 18, oy - 22)); label("LED_PGC", ox + 12, oy - 22)
gnd("J7", "2")
place("R7", "R_H", ox + 28, oy - 4, "100R")
place("R8", "R_V", ox + 32, oy, "100k")
place("Q2", "PFET_V", ox + 36, oy - 4, "AO3401A", "AO3401A", "C15127")
wire("AMP_EN", ("R7", "1"), (ox + 19, oy - 4)); label("AMP_EN", ox + 19, oy - 4)
wire("Q2_G", ("R7", "2"), (ox + 32, oy - 4), ("Q2", "G"))
wire("Q2_G", (ox + 32, oy - 4), ("R8", "2"))
p5v("R8", "1"); p5v("Q2", "S")
wire("AMP_VCC", ("Q2", "D"), (ox + 36, oy - 8), (ox + 42, oy - 8)); label("AMP_VCC", ox + 37, oy - 8)
texts.append((sheet, ox + 19, oy - 11, "Q2: LM386 power, off while the keyer is idle (AMP_EN low = on).", 1.524))

# ============================================================ sheet 2
sheet = 2
title(-6, 8, "STAND-ALONE KEYER - sheet 2/2: sidetone, LM386, audio switch  (draft 1, 2026-11-05)", 3.0)
title(-6, 2.5, "SIDETONE (as Rev 2 sheet 2; LM386 on switched 5 V)")
place("R15", "R_H", 4, -4, "1k5"); place("R16", "R_H", 12, -4, "1k5"); place("R17", "R_H", 20, -4, "1k5")
place("C9", "C_V", 8, -7, "100nF"); place("C10", "C_V", 16, -7, "100nF"); place("C11", "C_V", 24, -7, "100nF")
place("L1", "L_V", 28, -7, "1mH"); place("C12", "C_V", 28, -13, "470nF")
place("C13", "C_H", 32, -4, "10nF")
place("J9", "HDR3_L", 42, -3, "VOLUME 10k AUDIO")
place("C14", "C_V", 35, -9, "10nF")
wire("SIDE_TONE", (-4, -4), ("R15", "1")); label("SIDE_TONE", -4, -4)
wire("ST1", ("R15", "2"), (8, -4), ("R16", "1")); wire("ST1", (8, -4), ("C9", "1"))
wire("ST2", ("R16", "2"), (16, -4), ("R17", "1")); wire("ST2", (16, -4), ("C10", "1"))
wire("ST3", ("R17", "2"), (24, -4), (28, -4), ("C13", "1")); wire("ST3", (24, -4), ("C11", "1")); wire("ST3", (28, -4), ("L1", "1"))
wire("ST4", ("L1", "2"), ("C12", "1"))
for c in ("C9", "C10", "C11", "C12"):
    gnd(c, "2")
wire("VOL_TOP", ("C13", "2"), ("J9", "1"))
gnd("J9", "3")
place("U6", "LM386", 41, -9, "LM386MX-1/NOPB", "LM386MX-1/NOPB", "C23573")
wire("VOL_W", ("J9", "2"), (39, -5), (39, -7), (39, -10), ("U6", "IN_P"))
wire("VOL_W", (39, -7), ("C14", "1"))
gnd("C14", "2")
stub("U6", "IN_N", "GND", 4); stub("U6", "GND", "GND", 4); stub("U6", "GAIN1", "GAIN1", 4)
stub("U6", "VS", "AMP_VCC", 3); stub("U6", "GAIN8", "GAIN8", 3)
place("C16", "C_V", 51, -16, "100nF")
wire("BYP", ("U6", "BYP"), (51, -13), ("C16", "1")); gnd("C16", "2")
place("C19", "C_V", 54, -15, "100nF"); place("R19", "R_V", 54, -21, "10R")
place("C20", "CP_H", 60, -12, "220uF"); place("C21", "C_V", 63, -15, "10nF")
wire("AMP_OUT", ("U6", "VOUT"), (54, -12), ("C20", "P")); wire("AMP_OUT", (54, -12), ("C19", "1"))
wire("ZOB", ("C19", "2"), ("R19", "1")); gnd("R19", "2")
wire("SIDE_TONE_OUT", ("C20", "N"), (63, -12), (68, -12)); wire("SIDE_TONE_OUT", (63, -12), ("C21", "1"))
label("SIDE_TONE_OUT", 64, -12); gnd("C21", "2")
title(44, 5, "LM386 gain network (as Rev 2)", 1.778)
place("R18", "R_H", 48, 1, "680R"); place("C15", "CP_H", 54, 1, "10uF")
wire("GAIN1", ("R18", "1"), (42, 1)); label("GAIN1", 42, 1)
wire("G_MID", ("R18", "2"), ("C15", "P"))
wire("GAIN8", ("C15", "N"), (61, 1)); label("GAIN8", 57, 1)
title(70, 2.5, "AMP SUPPLY DECOUPLING", 1.778)
place("C17", "C_V", 74, -4, "100nF"); place("C18", "CP_V", 78, -4, "47uF")
wire("AMP_VCC", ("C17", "1"), (74, 0), (78, 0), ("C18", "P")); label("AMP_VCC", 74.5, 0)
gnd("C17", "2"); gnd("C18", "N")

# ---- audio switch
ox, oy = 0, -30
title(ox - 6, oy + 5, "AUDIO SWITCH: sidetone or rig audio to the phones")
place("R20", "R_V", ox + 5, oy + 1, "1k2"); place("D1", "Z_V", ox + 5, oy - 6, "1.8V DDZ9678-7", "DDZ9678-7")
place("U7", "PHOTOMOS", ox + 10, oy - 2, "VO1400AEFTR (NO)", "VO1400AEFTR", "C142551")
p5v("R20", "1")
wire("U7_A", ("R20", "2"), (ox + 5, oy - 3), ("U7", "A")); wire("U7_A", (ox + 5, oy - 3), ("D1", "K"))
gnd("D1", "A")
stub("U7", "C", "TX_CW", 3)
stub("U7", "S4", "SIDE_TONE_OUT", 6); stub("U7", "S3", "AUDIO_OUT", 6)
place("R21", "R_V", ox + 3, oy - 13, "1k2"); place("D2", "Z_V", ox + 3, oy - 20, "1.8V DDZ9678-7", "DDZ9678-7")
place("U8", "PHOTOMOS", ox + 10, oy - 16, "CPC1117NTR (NC)", "CPC1117NTR", "C150424")
p5v("R21", "1")
wire("U8_A", ("R21", "2"), (ox + 3, oy - 17), ("U8", "A")); wire("U8_A", (ox + 3, oy - 17), ("D2", "K"))
gnd("D2", "A")
wire("RX_CW_PGD", ("U8", "C"), (ox + 7, oy - 18), (ox + 7, oy - 24), (ox + 15, oy - 24)); label("RX_CW_PGD", ox + 8, oy - 24)
place("J11", "JACK2_L", ox + 27, oy - 16, "RIG AUDIO IN SJ1-3533NG", "SJ1-3533NG", "C4992459")
wire("RX_AUDIO", ("U8", "S4"), ("J11", "TIP")); label("RX_AUDIO", ox + 18, oy - 17)
gnd("J11", "SLEEVE")
wire("AUDIO_OUT", ("U8", "S3"), (ox + 19, oy - 18), (ox + 19, oy - 21), (ox + 25, oy - 21)); label("AUDIO_OUT", ox + 19.5, oy - 21)
place("J10", "JACK2_L", ox + 34, oy - 6, "PHONES SJ1-3533NG", "SJ1-3533NG", "C4992459")
stub("J10", "TIP", "AUDIO_OUT", 6); gnd("J10", "SLEEVE")
place("J12", "HDR2_L", ox + 34, oy - 24, "AUDIO_OUT")
stub("J12", "1", "AUDIO_OUT", 6); gnd("J12", "2")
texts.append((sheet, ox - 6, oy - 30, "U8 is normally closed: rig audio passes with its LED off (no current).", 1.524))
texts.append((sheet, ox - 6, oy - 31, "Firmware (stand-alone): RX_CW high = rig audio on, low = muted while sending.", 1.524))

# ============================================================ expected nets (from NETLIST.md)
EXPECTED = {
 "BATT+": {"J1.1", "J2.1"}, "SW_OUT": {"J2.2", "Q1.D"}, "Q1_G": {"Q1.G", "R1.1"},
 "VBAT": {"Q1.S", "U1.VIN", "C1.1", "R2.1"}, "VSENSE": {"R2.2", "R3.1", "C4.1", "U2.RC0"},
 "+5V": {"U1.VOUT", "C2.1", "C3.1", "U2.VDD", "C5.1", "J8.2", "R4.1", "J6.1", "Q2.S", "R8.1", "U3.VCC",
         "R11.1", "R12.1", "R13.1", "R14.1", "R20.1", "R21.1"},
 "PAD_A": {"U2.RA5", "R9.2", "R11.2", "C7.1"}, "PAD_B": {"U2.RA4", "R10.2", "R12.2", "C8.1"},
 "FN": {"U2.RA3_MCLR", "R4.2", "R5.1", "J8.1"}, "FN_SW": {"R5.2", "J5.1"},
 "KEY_K": {"U2.RC5", "U4.C"}, "PTT_K": {"U2.RC4", "U5.C"}, "TX_CW": {"U2.RC3", "U7.C"},
 "SIDE_TONE": {"U2.RC2", "R15.1"}, "AMP_EN": {"U2.RC1", "R7.1"}, "SPEED": {"U2.RA2", "J6.2", "C6.1"},
 "LED_PGC": {"U2.RA1_PGC", "R6.2", "J8.5"}, "RX_CW_PGD": {"U2.RA0_PGD", "J8.4", "U8.C"},
 "LED_A": {"R6.1", "J7.1"}, "Q2_G": {"Q2.G", "R7.2", "R8.2"},
 "AMP_VCC": {"Q2.D", "U6.VS", "C17.1", "C18.P"},
 "PAD_A_RAW": {"J3.TIP", "U3.IO1", "R9.1"}, "PAD_B_RAW": {"J3.RING", "U3.IO2", "R10.1"},
 "U4_A": {"U4.A", "R13.2"}, "U5_A": {"U5.A", "R14.2"},
 "KEY_COM": {"U4.S3", "U5.S3", "J4.SLEEVE"}, "KEY_TIP": {"U4.S4", "J4.TIP"}, "PTT_RING": {"U5.S4", "J4.RING"},
 "ST1": {"R15.2", "C9.1", "R16.1"}, "ST2": {"R16.2", "C10.1", "R17.1"},
 "ST3": {"R17.2", "C11.1", "L1.1", "C13.1"}, "ST4": {"L1.2", "C12.1"},
 "VOL_TOP": {"C13.2", "J9.1"}, "VOL_W": {"J9.2", "C14.1", "U6.IN_P"},
 "GAIN1": {"U6.GAIN1", "R18.1"}, "G_MID": {"R18.2", "C15.P"}, "GAIN8": {"C15.N", "U6.GAIN8"},
 "BYP": {"U6.BYP", "C16.1"}, "AMP_OUT": {"U6.VOUT", "C19.1", "C20.P"}, "ZOB": {"C19.2", "R19.1"},
 "SIDE_TONE_OUT": {"C20.N", "C21.1", "U7.S4"}, "U7_A": {"U7.A", "R20.2", "D1.K"}, "U8_A": {"U8.A", "R21.2", "D2.K"},
 "RX_AUDIO": {"U8.S4", "J11.TIP"}, "AUDIO_OUT": {"U7.S3", "U8.S3", "J10.TIP", "J12.1"},
}
GND_PINS = {"J1.2", "R1.2", "U1.GND", "C1.2", "R3.2", "C4.2", "C2.2", "C3.2", "U2.VSS", "C5.2", "J8.3",
            "J5.2", "J6.3", "C6.2", "J7.2", "J3.SLEEVE", "U3.GND", "C7.2", "C8.2", "C9.2", "C10.2", "C11.2",
            "C12.2", "J9.3", "C14.2", "U6.IN_N", "U6.GND", "C16.2", "R19.2", "C21.2", "C17.2", "C18.N",
            "D1.A", "D2.A", "J10.SLEEVE", "J11.SLEEVE", "J12.2"}
EXPECTED["GND"] = GND_PINS

# ============================================================ checks
err = []
got = defaultdict(set)
for (ref, pin), n in pinnet.items():
    if not parts[ref]["supply"]:
        got[n].add(f"{ref}.{pin}")
for n in sorted(set(EXPECTED) | set(got)):
    if EXPECTED.get(n, set()) != got.get(n, set()):
        err.append(f"net {n}: expected-only {sorted(EXPECTED.get(n, set()) - got.get(n, set()))} "
                   f"drawn-only {sorted(got.get(n, set()) - EXPECTED.get(n, set()))}")
for ref, p in parts.items():
    for pn, *_ in SYM[p["sym"]]["p"]:
        if (ref, pn) not in pinnet and not (ref == "J8" and pn == "6"):
            err.append(f"unconnected pin {ref}.{pn}")

def on_seg(px, py, x1, y1, x2, y2, eps=1e-6):
    if abs((x2 - x1) * (py - y1) - (y2 - y1) * (px - x1)) > eps:
        return False
    return min(x1, x2) - eps <= px <= max(x1, x2) + eps and min(y1, y2) - eps <= py <= max(y1, y2) + eps

pinpts = defaultdict(list)    # sheet -> [(x, y, net, ref.pin)]
for (ref, pin), n in pinnet.items():
    x, y = P(ref, pin)
    pinpts[parts[ref]["sheet"]].append((x, y, n, f"{ref}.{pin}"))
for sh in (1, 2):
    ws = [w for w in wires if w[0] == sh]
    for _, n, x1, y1, x2, y2 in ws:
        if x1 != x2 and y1 != y2:
            err.append(f"sheet {sh} {n}: diagonal wire {(x1, y1, x2, y2)}")
        for px, py, pn, who in pinpts[sh]:
            if pn != n and on_seg(px, py, x1, y1, x2, y2):
                err.append(f"sheet {sh}: wire of {n} touches pin {who} ({pn})")
        for _, n2, a1, b1, a2, b2 in ws:
            if n2 == n:
                continue
            for ex, ey in ((x1, y1), (x2, y2)):
                if on_seg(ex, ey, a1, b1, a2, b2):
                    err.append(f"sheet {sh}: wire end of {n} at {(ex, ey)} on a wire of {n2}")
        # wire through a part body (not the part it connects to at that end)
        for ref, p in parts.items():
            if p["sheet"] != sh:
                continue
            bx1, by1, bx2, by2 = SYM[p["sym"]]["body"]
            bx1 += p["x"] + 0.1; bx2 += p["x"] - 0.1; by1 += p["y"] + 0.1; by2 += p["y"] - 0.1
            if x1 == x2:
                hit = bx1 < x1 < bx2 and min(y1, y2) < by2 and max(y1, y2) > by1
            else:
                hit = by1 < y1 < by2 and min(x1, x2) < bx2 and max(x1, x2) > bx1
            if hit:
                err.append(f"sheet {sh}: wire of {n} {(x1, y1, x2, y2)} runs through {ref}")
for sh, n, x, y in labels:
    if not any(w[0] == sh and w[1] == n and on_seg(x, y, *w[2:]) for w in wires):
        err.append(f"label {n} at {(x, y)} not on a wire of {n}")
if err:
    print("\n".join(sorted(set(err))))
    sys.exit(1)

# ============================================================ segments, junctions
def components(sh, n):
    ws = [w for w in wires if w[0] == sh and w[1] == n]
    parent = list(range(len(ws)))
    def f(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]; i = parent[i]
        return i
    for i, a in enumerate(ws):
        for j, b in enumerate(ws):
            if j <= i:
                continue
            if any(on_seg(ex, ey, *b[2:]) for ex, ey in ((a[2], a[3]), (a[4], a[5]))) or \
               any(on_seg(ex, ey, *a[2:]) for ex, ey in ((b[2], b[3]), (b[4], b[5]))):
                parent[f(i)] = f(j)
    groups = defaultdict(list)
    for i, w in enumerate(ws):
        groups[f(i)].append(w)
    return list(groups.values())

segments = []   # (sheet, net, wires, pinrefs, labels, junctions)
for sh in (1, 2):
    for n in sorted({w[1] for w in wires if w[0] == sh}):
        for g in components(sh, n):
            prs = [(ref, pin) for (ref, pin), nn in pinnet.items() if nn == n and parts[ref]["sheet"] == sh
                   and any(on_seg(*P(ref, pin), *w[2:]) for w in g)]
            lbs = [(x, y) for s2, nn, x, y in labels if s2 == sh and nn == n and any(on_seg(x, y, *w[2:]) for w in g)]
            ends = defaultdict(int)
            for w in g:
                ends[(w[2], w[3])] += 1; ends[(w[4], w[5])] += 1
            jn = [pt for pt, c in ends.items() if c >= 3 or (c >= 2 and any(on_seg(*P(r, p), *[pt[0], pt[1], pt[0], pt[1]]) for r, p in prs))]
            jn = [pt for pt, c in ends.items() if c >= 3]
            segments.append((sh, n, g, prs, lbs, jn))
# every multi-segment net piece needs a name on it (label or supply symbol)
for sh, n, g, prs, lbs, jn in segments:
    pieces = sum(1 for s in segments if s[1] == n)
    if pieces > 1 and not lbs and not any(parts[r]["supply"] for r, _ in prs):
        err.append(f"sheet {sh} net {n}: a piece without a label")
if err:
    print("\n".join(err)); sys.exit(1)

# ============================================================ write Eagle XML
def m(v):
    return f"{v * MM:.4f}".rstrip("0").rstrip(".")

def rot_of(dx, dy):
    return {(1, 0): "R0", (0, 1): "R90", (-1, 0): "R180", (0, -1): "R270"}[(dx, dy)]

used = sorted({p["sym"] for p in parts.values()})
o = ['<?xml version="1.0" encoding="utf-8"?>', '<!DOCTYPE eagle SYSTEM "eagle.dtd">', '<eagle version="9.6.2">', '<drawing>',
     '<settings><setting alwaysvectorfont="no"/><setting verticaltext="up"/></settings>',
     '<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" altdistance="0.01" altunitdist="inch" altunit="inch"/>',
     '<layers>']
for num, name, col in [(1, "Top", 4), (16, "Bottom", 1), (17, "Pads", 2), (18, "Vias", 2), (19, "Unrouted", 6), (20, "Dimension", 15),
                       (21, "tPlace", 7), (25, "tNames", 7), (27, "tValues", 7), (29, "tStop", 7), (31, "tCream", 7), (51, "tDocu", 7),
                       (91, "Nets", 2), (92, "Busses", 1), (93, "Pins", 2), (94, "Symbols", 4), (95, "Names", 7), (96, "Values", 7),
                       (97, "Info", 7), (98, "Guide", 6)]:
    o.append(f'<layer number="{num}" name="{name}" color="{col}" fill="1" visible="yes" active="yes"/>')
o += ['</layers>', '<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">',
      '<description>Stand-alone keyer draft 1 (2026-11-05). Generated by hardware/tools/make_eagle.py; '
      'nets match hardware/NETLIST.md. Footprints are placeholders: use the LCSC parts in BOM.csv.</description>',
      '<libraries>', '<library name="keyer">', '<packages>']
for s in used:
    if s.startswith("SUP_"):
        continue
    o.append(f'<package name="PKG_{s}"><description>Placeholder footprint</description>')
    for i, pad in enumerate(PADS[s].values()):
        o.append(f'<smd name={quoteattr(pad)} x="{i * 1.27:.2f}" y="0" dx="0.8" dy="1.2" layer="1"/>')
    o.append('</package>')
o += ['</packages>', '<symbols>']
for s in used:
    d = SYM[s]
    o.append(f'<symbol name="SYM_{s}">')
    for e in d["g"]:
        if e[0] == "l":
            o.append(f'<wire x1="{m(e[1])}" y1="{m(e[2])}" x2="{m(e[3])}" y2="{m(e[4])}" width="{e[5] * MM:.4f}" layer="94"/>')
        elif e[0] == "a":
            o.append(f'<wire x1="{m(e[1])}" y1="{m(e[2])}" x2="{m(e[3])}" y2="{m(e[4])}" width="0.254" layer="94" curve="{e[5]}"/>')
        else:
            o.append(f'<text x="{m(e[1])}" y="{m(e[2])}" size="{e[4]}" layer="{e[5]}">{escape(e[3])}</text>')
    for n, px, py, dx, dy, ln, vis in d["p"]:
        direction = ' direction="sup"' if s.startswith("SUP_") else ""
        o.append(f'<pin name={quoteattr(n)} x="{m(px)}" y="{m(py)}" visible="{vis}" length="{ln}"{direction} rot="{rot_of(dx, dy)}"/>')
    o.append('</symbol>')
o += ['</symbols>', '<devicesets>']
for s in used:
    if s.startswith("SUP_"):
        o.append(f'<deviceset name="{s}" prefix="{PREFIX[s]}"><gates><gate name="G$1" symbol="SYM_{s}" x="0" y="0"/></gates>'
                 '<devices><device name=""><technologies><technology name=""/></technologies></device></devices></deviceset>')
        continue
    o.append(f'<deviceset name="{s}" prefix="{PREFIX.get(s, "U" if not s.startswith(("JACK", "HDR")) else "J")}">'
             f'<gates><gate name="G$1" symbol="SYM_{s}" x="0" y="0"/></gates><devices><device name="" package="PKG_{s}"><connects>')
    for pn, pad in PADS[s].items():
        o.append(f'<connect gate="G$1" pin={quoteattr(pn)} pad={quoteattr(pad)}/>')
    o.append('</connects><technologies><technology name=""/></technologies></device></devices></deviceset>')
o += ['</devicesets>', '</library>', '</libraries>', '<attributes/>', '<variantdefs/>',
      '<classes><class number="0" name="default" width="0" drill="0"/></classes>', '<parts>']
for ref, p in parts.items():
    o.append(f'<part name={quoteattr(ref)} library="keyer" deviceset="{p["sym"]}" device="" value={quoteattr(p["value"])}>')
    if p["mpn"]:
        o.append(f'<attribute name="MPN" value={quoteattr(p["mpn"])}/>')
    if p["lcsc"]:
        o.append(f'<attribute name="LCSC" value={quoteattr(p["lcsc"])}/>')
    o.append('</part>')
o += ['</parts>', '<sheets>']
for sh in (1, 2):
    o.append('<sheet><plain>')
    for s2, x, y, t, size in texts:
        if s2 == sh:
            o.append(f'<text x="{m(x)}" y="{m(y)}" size="{size}" layer="97">{escape(t)}</text>')
    o.append('</plain><instances>')
    for ref, p in parts.items():
        if p["sheet"] == sh:
            o.append(f'<instance part={quoteattr(ref)} gate="G$1" x="{m(p["x"])}" y="{m(p["y"])}"/>')
    o.append('</instances><busses/><nets>')
    for n in sorted({s[1] for s in segments if s[0] == sh}):
        o.append(f'<net name={quoteattr(n)} class="0">')
        for s2, nn, g, prs, lbs, jn in segments:
            if s2 != sh or nn != n:
                continue
            o.append('<segment>')
            for r, pn in prs:
                o.append(f'<pinref part={quoteattr(r)} gate="G$1" pin={quoteattr(pn)}/>')
            for w in g:
                o.append(f'<wire x1="{m(w[2])}" y1="{m(w[3])}" x2="{m(w[4])}" y2="{m(w[5])}" width="0.1524" layer="91"/>')
            for x, y in jn:
                o.append(f'<junction x="{m(x)}" y="{m(y)}"/>')
            for x, y in lbs:
                o.append(f'<label x="{m(x)}" y="{m(y + 0.15)}" size="1.778" layer="95"/>')
            o.append('</segment>')
        o.append('</net>')
    o.append('</nets></sheet>')
o += ['</sheets>', '</schematic>', '</drawing>', '</eagle>']
out = sys.argv[1] if len(sys.argv) > 1 else "keyer-standalone.sch"
open(out, "w", encoding="utf-8", newline="\n").write("\n".join(o) + "\n")
print(f"wrote {out}: {len([p for p in parts.values() if not p['supply']])} parts, "
      f"{len([p for p in parts.values() if p['supply']])} supply symbols, {len({s[1] for s in segments})} nets")

# ============================================================ preview (SVG of both sheets)
if len(sys.argv) > 2:
    S = 12.0   # px per grid
    def bounds(sh):
        xs, ys = [], []
        for ref, p in parts.items():
            if p["sheet"] == sh:
                xs += [p["x"] - 3, p["x"] + 16]; ys += [p["y"] + 3, p["y"] - 10]
        for w in wires:
            if w[0] == sh:
                xs += [w[2], w[4]]; ys += [w[3], w[5]]
        for t in texts:
            if t[0] == sh:
                xs += [t[1], t[1] + len(t[3]) * t[4] * 0.6 / MM]; ys += [t[2] + 1]
        return min(xs) - 2, max(xs) + 8, min(ys) - 3, max(ys) + 2
    svg = []
    yoff = 0
    sheets_svg = []
    for sh in (1, 2):
        x0, x1, y0, y1 = bounds(sh)
        wpx, hpx = (x1 - x0) * S, (y1 - y0) * S
        def X(x): return (x - x0) * S
        def Y(y): return yoff + (y1 - y) * S
        g = [f'<rect x="0" y="{yoff}" width="{wpx}" height="{hpx}" fill="#fff" stroke="#999"/>']
        for ref, p in parts.items():
            if p["sheet"] != sh:
                continue
            d = SYM[p["sym"]]
            for e in d["g"]:
                if e[0] == "l":
                    g.append(f'<line x1="{X(p["x"] + e[1]):.1f}" y1="{Y(p["y"] + e[2]):.1f}" x2="{X(p["x"] + e[3]):.1f}" y2="{Y(p["y"] + e[4]):.1f}" stroke="#a52a2a" stroke-width="1.4"/>')
                elif e[0] == "a":
                    r = abs(e[2] - e[4]) / 2 * S
                    g.append(f'<path d="M{X(p["x"] + e[1]):.1f},{Y(p["y"] + e[2]):.1f} A{r:.1f},{r:.1f} 0 0 1 {X(p["x"] + e[3]):.1f},{Y(p["y"] + e[4]):.1f}" fill="none" stroke="#a52a2a" stroke-width="1.4"/>')
                else:
                    t = e[3].replace(">NAME", ref).replace(">VALUE", p["value"])
                    if p["supply"] and e[3] == ">VALUE":
                        t = p["value"]
                    g.append(f'<text x="{X(p["x"] + e[1]):.1f}" y="{Y(p["y"] + e[2]):.1f}" font-size="{e[4] / MM * S * 0.9:.1f}" fill="{"#222" if e[5] == 95 else "#555"}" font-family="Arial">{escape(t)}</text>')
            for n, px, py, dx, dy, ln, vis in d["p"]:
                L = 1 if ln == "short" else 0
                ax, ay = p["x"] + px, p["y"] + py
                if L:
                    g.append(f'<line x1="{X(ax):.1f}" y1="{Y(ay):.1f}" x2="{X(ax + dx * L):.1f}" y2="{Y(ay + dy * L):.1f}" stroke="#a52a2a" stroke-width="1.2"/>')
                if vis in ("both", "pin") and not p["supply"]:
                    if dx == 1:
                        g.append(f'<text x="{X(ax + 1.2):.1f}" y="{Y(ay - 0.25):.1f}" font-size="8" fill="#333" font-family="Arial">{escape(n)}</text>')
                    elif dx == -1:
                        g.append(f'<text x="{X(ax - 1.2):.1f}" y="{Y(ay - 0.25):.1f}" font-size="8" fill="#333" text-anchor="end" font-family="Arial">{escape(n)}</text>')
                    elif dy == -1:
                        g.append(f'<text x="{X(ax):.1f}" y="{Y(ay - 1.7):.1f}" font-size="8" fill="#333" text-anchor="middle" font-family="Arial">{escape(n)}</text>')
                    else:
                        g.append(f'<text x="{X(ax):.1f}" y="{Y(ay + 1.2):.1f}" font-size="8" fill="#333" text-anchor="middle" font-family="Arial">{escape(n)}</text>')
                if vis == "both" and not p["supply"] and dx != 0:
                    pad = PADS[p["sym"]][n]
                    g.append(f'<text x="{X(ax + dx * 0.5):.1f}" y="{Y(ay + 0.15):.1f}" font-size="6.5" fill="#777" text-anchor="middle" font-family="Arial">{escape(pad)}</text>')
        for w in wires:
            if w[0] == sh:
                g.append(f'<line x1="{X(w[2]):.1f}" y1="{Y(w[3]):.1f}" x2="{X(w[4]):.1f}" y2="{Y(w[5]):.1f}" stroke="#1a7f1a" stroke-width="1.3"/>')
        for s2, nn, gg, prs, lbs, jn in segments:
            if s2 == sh:
                for x, y in jn:
                    g.append(f'<circle cx="{X(x):.1f}" cy="{Y(y):.1f}" r="3" fill="#1a7f1a"/>')
        for s2, n, x, y in labels:
            if s2 == sh:
                g.append(f'<text x="{X(x):.1f}" y="{Y(y + 0.2):.1f}" font-size="9.5" fill="#00408a" font-family="Arial">{escape(n)}</text>')
        for s2, x, y, t, size in texts:
            if s2 == sh:
                g.append(f'<text x="{X(x):.1f}" y="{Y(y):.1f}" font-size="{size / MM * S * 0.95:.1f}" fill="#111" font-weight="{"bold" if size > 2 else "normal"}" font-family="Arial">{escape(t)}</text>')
        sheets_svg.append((wpx, hpx, g))
        yoff += hpx + 20
    W_ = max(s[0] for s in sheets_svg)
    open(sys.argv[2], "w", encoding="utf-8").write(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W_:.0f}" height="{yoff:.0f}">' +
        "".join("".join(s[2]) for s in sheets_svg) + "</svg>")
    print("preview", sys.argv[2], f"{W_:.0f} x {yoff:.0f}")

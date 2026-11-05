# For Stew: a stand-alone keyer, for you to take into EasyEDA

From Ron, 2026-11-05. I'd like to make the PIC keyer from your Rev 2 STM32 / PIC / CW board
into a box of its own, running on a 9 V battery. You're the EasyEDA and PCB expert, so I'm
handing the hardware to you. Everything below is a draft: change whatever you think is
better. No rush.

## Where it is

`keyer-standalone/` in mscc-station:

| File | What |
|---|---|
| `STANDALONE-KEYER-SPEC.md` | What it does, block diagram, PIC pin map, power budget, controls |
| `hardware/NETLIST.md` | Every part and connection |
| `hardware/BOM.csv` | Parts list, with LCSC numbers for the chips, photoMOS, FETs and jacks |
| `hardware/keyer-standalone.sch` | Eagle schematic drawn in your Rev 2 style; EasyEDA imports it |
| `hardware/keyer-standalone-schematic-preview.svg` | What the Eagle file should look like |

To open the schematic in EasyEDA: use the **Eagle import** and pick `keyer-standalone.sch`.
I tried it and it comes in readable. The symbols are a generic library with placeholder
footprints, so each part still needs its LCSC footprint (or swapping for the LCSC part).

## What I decided

- Standard 9 V battery; no charger.
- Keys modern rigs only: VO1400 photoMOS, isolated, either polarity, 60 V / 100 mA.
- A second output for PTT / amp keying on the ring of the KEY OUT jack.
- Rig audio pass-through kept, through your photoMOS audio switch.
- Speed knob 5–60 wpm, a volume knob, one FN button, a status LED.
- Small aluminium box, about Hammond 1590B size (the 1590BB if it's too tight).

## What changed from your Rev 2 board

- The STM32, the PCIe connector, the thermistor and the ATU lines are gone.
- Power: 9 V → power switch → AO3401A P-FET (reverse-battery protection) → MCP1703 5 V
  regulator (about 2 µA of its own draw, instead of the 7805's few mA).
- The LM386 runs on 5 V, switched off by a second AO3401A while the keyer is idle.
- The receive-audio photoMOS is now a **CPC1117 (normally closed)**. With the Rev 2 part,
  the PIC has to keep its LED lit the whole time you're not sending, about 3 mA, which is a
  lot on a 9 V battery. The CPC1117 passes the rig audio with its LED off. The firmware
  inverts RX_CW to suit.
- Paddle jack with an ESD part (PRTR5V0U2X, as your U9) and a small RC filter per line.
- Battery check: a 1M / 470k divider into a spare PIC pin.
- Your sidetone filter and the LM386 network are copied unchanged.

## Worth checking when you draw it

These are marked "(check)" in `NETLIST.md`:
- the 3.5 mm jack pin numbers (I wrote them as tip / ring / sleeve),
- the photoMOS and AO3401A symbol pin numbers,
- the sidetone filter wiring: I read it from your sheet 2 and may have it slightly off,
- whether an FN button on the MCLR pin (with a 1k in series) is OK with the PICkit.

## If you want a second pair of eyes

When you have it in EasyEDA, export the netlist (`.net`) and send it to me. I'll compare it
automatically with `NETLIST.md` and list any connection that changed or went missing.
Up to you.

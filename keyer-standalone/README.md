# keyer-standalone

Stand-alone, 9 V battery version of the PIC keyer (from the Rev 2 STM32 / PIC / CW board).
Firmware will come from `../keyer-buffer` (buffered Farnsworth keyer, in Ron's rig).

| File | What |
|---|---|
| `STANDALONE-KEYER-SPEC.md` | Spec: block diagram, pin map, power budget, controls / paddle menu, firmware plan, open questions |
| `hardware/NETLIST.md` | Every part and connection, sheet by sheet, for drawing in EasyEDA |
| `hardware/BOM.csv` | Parts list; LCSC numbers for the chips, photoMOS, FETs and jacks |
| `hardware/keyer-standalone-reference.svg` | Reference drawing (parts with net labels); open in a browser |
| `hardware/keyer-standalone.sch` | Eagle 9 schematic, Rev 2 style (2 sheets, real symbols, wired chains, GND / +5V symbols), for import into EasyEDA; placeholder footprints |
| `hardware/keyer-standalone-schematic-preview.svg` | What the Eagle file should look like (both sheets); open in a browser |
| `hardware/tools/draw_reference.py` | Regenerates the drawing: `python draw_reference.py keyer-standalone-reference.svg` |
| `hardware/tools/make_eagle.py` | Regenerates the Eagle file, checks it against the netlist, optional preview: `python make_eagle.py ../keyer-standalone.sch [preview.svg]` |

Status (2026-11-05): draft 1, not reviewed. Items marked (check) need the datasheet or the
EasyEDA symbol confirmed when drawing.

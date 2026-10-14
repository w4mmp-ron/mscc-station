# For Stew — MKII Rev 7: TX carrier level and the 10 m LO divider

From Ron, 2026-10-14. For your information and as options for a future board revision; your
call on all of it. Based on the Rev 7 schematic, netlist and Gerbers, and Ron's TX I/Q
measurements with his I/Q balance detector.

## What Ron measured

With the TX I/Q amplitude set correctly, the image is below the carrier: on 10 m about
13-14 dB under the LO leakage, on 20 m also under it. Phase error is very small (best null
-0.25 deg on 10 m). So on the MKII the carrier, not the image, is the larger unwanted signal.

## 1. TX carrier level is set by the hardware

The TX baseband reaches the mixer (U15) through C35-C38 (10 uF in series). They block DC,
so the trans software cannot null the carrier with a DC offset on I and Q, as many SDRs do.
The carrier level is set by U15 (switch mismatch, charge injection, LO feedthrough), T2
balance and the T2 centre-tap bias (R24/R25, about 2.0 V); the U16 op-amps sit at VCOM
(2.5 V), which is presumably why the caps are there.

Options, if lowering the carrier is ever worth it:

1. **Parts only:** a lower charge-injection switch in place of the SN74CBT3253, and/or a
   better-balanced T2. Small gain, no circuit change.
2. **Hardware trim:** a small adjustable DC into the four mixer-side nodes after C35-C38
   (e.g. a trim pot through high-value resistors), set once per rig. Manual, may drift.
3. **DC-coupled path:** drop C35-C38 and match the U16 output bias to the mixer bias.
   Then trans could null the carrier with a DC offset per band, saved like `IQ_OFFSET`
   (Ron would do the Pi trans side). Best result, but a redesign of that stage.

## 2. 10 m LO divider margin

The Si5351 CLK0 runs at 4 x the operating frequency (firmware `si5351a.c`), so on 10 m U12
(74ACT74) is clocked at about 112-119 MHz. Older 74ACT74 datasheets give 125 MHz minimum
over temperature (145 MHz typical); TI's current product page lists 90 MHz. Either way the
margin is thin on 10 m, more so with temperature, another maker's part, or the 3.3 V clock
edges from the Si5351.

Ron's rig is fine (phase -0.25 deg on 10 m), so this is a margin point across rigs, not a
fault. Option: a faster flip-flop in the same place (74LVC74 / 74AUC74 class), or check the
BOM part's own datasheet first.

## Layout (Gerbers) — no problems found for I/Q

- Inner layer 1 is solid ground under the whole I/Q area.
- LO tracks are short; the RX I and Q LO tracks differ by a few mm (well under 1 deg at 30 MHz).
- Mixer-to-transformer tracks (U10 -> T1, U15 -> T2) are short and symmetric.
- Minor, options only: the QSD_I / QSD_Q tracks pass close to X1 / C147 (TCXO into the
  Si5351); the six band-data lines U22 -> P1 run on inner layer 2 under T1 / T2 (DC and
  shielded by inner layer 1, so low risk; small caps at P1 would also keep cable noise off).

## Also

The schematic title blocks still say Rev 6 (2024-02-10); the PCB is Rev 7.

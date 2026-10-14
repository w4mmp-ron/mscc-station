# TX I/Q balance tools (PC side)

Python scripts that run on the Windows PC next to the rig. They need Ron's I/Q Balance
Detector (IQBD) on the transmit output, feeding a 96 kHz sound card line input.
`pip install sounddevice` (numpy and matplotlib also needed).

| Script | What it does |
|---|---|
| `iqbd-meter.py` | Live spectrum of the detector audio. Reads LO leakage (12 kHz) and image (24 kHz) lines in dBFS, with lowest-image holds. |
| `iq-phase.py` | Slider window for the TX phase correction in Pi `sdrcore-trans` (UDP 9200, opcodes 0x2A / 0x2B, SAVE = 0x57). |
| `iq-auto.py` | Automatic TX amplitude (and with `--phase`, phase) null. You key from the client TX IQ tab; it steps the value, finds the lowest image and saves `iq.ini`. |

Typical run (sound card number from `--list`):

    python iq-auto.py --device 33 --host proficio --phase

Only one program can use the sound card at a time: close `iqbd-meter.py` before `iq-auto.py`.
The phase opcodes exist only in the Pi `sdrcore-trans` build (see `rpi/CLAUDE.md`).

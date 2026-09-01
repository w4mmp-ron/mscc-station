# Proficio STM32 — Windows USB test jig

GUI for **bare Black Pill** bring-up (no daughter / PCM3060 required).

Talks vendor USB **VID `16C0` / PID `05DC`** like ms-sdr.

## Setup (Windows)

```powershell
cd C:\Users\Ron\.grok\worktrees\proficio-stm32f411-25MHz\tools\stm32-test-jig
py -3 -m pip install -r requirements.txt
```

**Driver:** If Windows does not open the device, run [Zadig](https://zadig.akeo.ie/), select **Proficio** / `16c0:05dc`, install **WinUSB**.

Close **ms-sdr** / any host holding the radio USB before testing.

## Run — Windows GUI

**From Explorer (double-click):**
- `Run-Test-Jig.bat` — recommended  
- or `jig.pyw` (if `.pyw` is associated with Python)

**From a prompt:**

```powershell
python jig.py
```

1. Flash firmware, plug Black Pill USB  
2. **Refresh** → Connected  
3. **Get Version** → expect major **5** (STM port)  
4. **Get Temp**, **Get Freq** / **SET_FREQ**, pin/PTT/key  
5. Optional: **Enter ROM Bootloader (0xFE)** for CubeProgrammer  

## Run — Pi / Linux CLI

Pi OS blocks `pip3 install` system-wide (PEP 668). Prefer **apt** or a **venv**:

**Option A — apt (simplest):**
```bash
sudo apt update
sudo apt install -y python3-usb
cd tools/stm32-test-jig
python3 jig_cli.py all
```

**Option B — venv:**
```bash
cd tools/stm32-test-jig
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python jig_cli.py all
```

```bash
python3 jig_cli.py version
python3 jig_cli.py temp
python3 jig_cli.py set-freq 14074000
python3 jig_cli.py tx on
python3 jig_cli.py dfu
```

Same VID/PID and opcodes as the Windows jig. Uses your existing udev rule.

I/Q and codec paths remain in firmware; this jig simply does not need them.

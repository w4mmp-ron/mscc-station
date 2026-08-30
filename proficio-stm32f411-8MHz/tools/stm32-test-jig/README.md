# Proficio STM32 — Windows USB test jig

GUI for **bare Black Pill** bring-up (no daughter / PCM3060 required).

Talks vendor USB **VID `16C0` / PID `05DC`** like ms-sdr.

## Setup (Windows)

```powershell
cd C:\Users\Ron\.grok\worktrees\proficio-stm32f411-8MHz\tools\stm32-test-jig
py -3 -m pip install -r requirements.txt
```

**Driver:** If Windows does not open the device, run [Zadig](https://zadig.akeo.ie/), select **Proficio** / `16c0:05dc`, install **WinUSB**.

Close **ms-sdr** / any host holding the radio USB before testing.

## Run

**From Explorer (double-click):**
- `Run-Test-Jig.bat` — recommended  
- or `jig.pyw` (if `.pyw` is associated with Python)

**From a prompt:**

```powershell
python jig.py
```

## What to click first

1. Flash firmware, plug Black Pill USB  
2. **Refresh** → Connected  
3. **Get Version** → expect major **5** (STM port)  
4. **Get Temp**, **Get Freq** / **SET_FREQ**, pin/PTT/key  
5. Optional: **Enter ROM Bootloader (0xFE)** for CubeProgrammer  

I/Q and codec paths remain in firmware; this jig simply does not need them.

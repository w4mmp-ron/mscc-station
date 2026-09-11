# Proficio USB access under WSL2 (Debian)

WSL2 does **not** see USB devices until you attach them from Windows with **usbipd**.

## 1. Windows (one-time)

In **elevated** PowerShell (admin once for install):

```powershell
winget install usbipd
```

(Or install [usbipd-win](https://github.com/dorssel/usbipd-win/releases).)

## 2. Plug in Proficio, list devices (Windows PowerShell)

```powershell
usbipd list
```

Find the Multus/Proficio line (VID **16C0**, PID **05DC**). Note the **BUSID** (e.g. `1-4`).

## 3. Bind + attach to WSL

```powershell
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

In Debian:

```bash
sudo apt install -y libusb-1.0-0-dev usbutils
lsusb
# expect something like: ID 16c0:05dc ...
```

## 4. Permissions (Debian)

Either run ms-sdr with `sudo`, or add a udev rule:

```bash
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05dc", MODE="0666"' | sudo tee /etc/udev/rules.d/99-proficio.rules
sudo udevadm control --reload-rules
# re-plug or re-attach device
```

## 5. Build & run with USB

```bash
cd /mnt/c/Users/Ron/.grok/worktrees/ms-sdr-linux
make clean
make                    # default: real USB (MS_SDR_NO_USB=0)
# if permission denied: sudo ./ms-sdr
./ms-sdr
```

Handshake-only (no radio):

```bash
make clean && make MS_SDR_NO_USB=1
```

## 6. Detach when done (Windows)

```powershell
usbipd detach --busid <BUSID>
```

## Notes

- After Windows reboot, re-`attach` the device.
- Client still uses **WSL IP:8888** (`hostname -I` in Debian).
- VID/PID in code: `0x16C0` / `0x05DC` (`main.c`).

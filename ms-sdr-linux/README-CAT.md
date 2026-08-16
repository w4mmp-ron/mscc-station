# Kenwood CAT (comm port) on Linux / WSL

ms-sdr emulates a **Kenwood** rig for digital apps (WSJT-X, etc.).

## Linux path

On start, if `~/comm-port.ini` is missing, ms-sdr creates one with `COMM_PORT_NAME=PTY` and **opens CAT immediately**.  
If the file says `PTY` / `COM0` / `cat` (or a `/dev/...` path), ms-sdr opens that and creates a **pseudo-terminal** for PTY mode:

| Side | Role |
|------|------|
| **Master** | held by ms-sdr (CAT engine) |
| **Slave symlink** | **`$HOME/ms-sdr-cat`** → e.g. `/dev/pts/3` |

Digital apps that run **inside Linux** use:

```text
/home/ron/ms-sdr-cat
```

(or the real `/dev/pts/N` printed at startup).

### Example `~/comm-port.ini`

```text
COMM_PORT_NAME=PTY,COMM_PORT_INDEX=0,BAUD_RATE_INDEX=3,PARITY_INDEX=0,DATA_BITS_INDEX=1,STOP_BITS_INDEX=0,PIN=0;
```

(`BAUD_RATE_INDEX=3` → 9600.)

Delete a bad ini and restart ms-sdr to recreate defaults.

## WSL + Windows digital apps (WSJT-X on Windows)

A Linux PTY is **not** a Windows `COM` port. Options:

1. Run digital app **in WSL/Linux** and point it at `~/ms-sdr-cat`
2. Later: bridge PTY ↔ Windows COM (e.g. `socat` + virtual COM tools) — not set up yet
3. Keep Windows ms-sdr for CAT + Windows WSJT-X

## Build (no USB)

```bash
cd /mnt/c/Users/Ron/.grok/worktrees/ms-sdr-linux
make clean && make MS_SDR_NO_USB=1
./ms-sdr
```

Watch for:

```text
[ms-sdr] Kenwood CAT PTY ready: /home/ron/ms-sdr-cat -> /dev/pts/N
```

## Quick CAT smoke test (Debian)

```bash
# terminal A: ms-sdr running
# terminal B:
echo -n 'ID;' > ~/ms-sdr-cat
# or use minicom / screen on the pts
```

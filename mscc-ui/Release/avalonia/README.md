# Avalonia / Raspberry Pi Linux install set

| Package | Version | Notes |
|---------|---------|--------|
| PortAudio | 19.8.2 | Unchanged (contents built 2026-07-31) |
| Servers `mscc_*` | **1.0.41** in this folder until Pi rebuild | See below for **1.0.42** FM |
| Init GUI | 1.0.13 | |
| Avalonia UI | **0.6.44** | FM power slider + TX IQ freqs + FM UI |

## Install (current folder contents)

```bash
sudo apt install -y ./mscc-portaudio_19.8.2_arm64.deb
sudo apt update
sudo apt install -y ./mscc_1.0.41_arm64.deb
sudo apt install -y ./mscc-init-gui_1.0.13_all.deb
chmod 644 ./mscc-ui_0.6.44_arm64.deb
sudo apt install -y ./mscc-ui_0.6.44_arm64.deb
```

UI **0.6.44** has FM mode + FM Power (RX/TX tab and RF mirror on Main).  
Server **1.0.41** does **not** yet include FM DSP — rebuild on the Pi for on-air FM.

## Building server **1.0.42** with FM (on the Pi)

Sources are ready in the monorepo (`SDRcore-recv-linux`, `SDRcore-trans-linux`, `ms-sdr-linux`).  
`mscc-deb` version is bumped to **1.0.42**; the `.deb` needs **AArch64** binaries in `mscc-binaries/`.

On the Pi (typical):

```bash
# 1) Build arm64: ms-sdr, sdrcore-recv, sdrcore-trans from *-linux trees
# 2) Copy into mscc-binaries/
# 3) On a machine with dpkg-deb (usually the Pi):
cd mscc-deb
./build-deb.sh
# → mscc_1.0.42_arm64.deb
```

Then replace `mscc_1.0.41` with `mscc_1.0.42` in this folder / `pi-install/packages`.

See also `pi-install/INSTALL.md`.

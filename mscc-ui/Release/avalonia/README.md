# Avalonia / Raspberry Pi Linux install set

Copy these `.deb` files to the Pi and install **in order**:

```bash
sudo apt install -y ./mscc-portaudio_19.8.2_arm64.deb
sudo apt update
sudo apt install -y ./mscc_1.0.40_arm64.deb
sudo apt install -y ./mscc-init-gui_1.0.13_all.deb
sudo apt install -y ./mscc-ui_0.6.40_arm64.deb
```

| Package | Role |
|---------|------|
| `mscc-portaudio_*` | PortAudio (install first) |
| `mscc_*` | Servers + tools (`ms-sdr`, sdrcore-*) |
| `mscc-init-gui_*` | Audio / init GUI |
| `mscc-ui_*` | Avalonia operate UI |

See also `pi-install/INSTALL.md` in the repo for the full how-to.

# MSCC Linux release drop (Avalonia + Pi packages)

Split by CPU. **Do not install an `arm64` `.deb` on an x86_64 Ubuntu PC** (and vice versa).  
`mscc-init-gui_*_all.deb` is architecture-independent and is in **both** folders.

| Folder | For | Packages |
|--------|-----|----------|
| **`arm64/`** | Raspberry Pi OS 64-bit | PortAudio, servers `mscc_1.0.42`, init-gui, Avalonia UI |
| **`x86_64/`** | Ubuntu Desktop amd64 | Avalonia UI `mscc-ui_*_amd64.deb`, init-gui |

Pi operator how-to: [`pi-install/INSTALL.md`](../../../pi-install/INSTALL.md).  
Ubuntu laptop how-to: [`INSTALL-UBUNTU.md`](../../../INSTALL-UBUNTU.md).

## Raspberry Pi (`arm64/`)

```bash
cd arm64
sudo apt install -y ./mscc-portaudio_19.8.2_arm64.deb
sudo apt update
sudo apt install -y ./mscc_1.0.42_arm64.deb
sudo apt install -y ./mscc-init-gui_1.0.13_all.deb
sudo apt install -y ./mscc-ui_0.6.44_arm64.deb
```

Servers **1.0.42** include FM (NFM). UI **0.6.44** has FM + FM Power.

## Ubuntu x86_64 (`x86_64/`)

Servers are **built from source** on the laptop (`linux-build/mscc-linux.sh`) — there is no `mscc_*_amd64.deb` yet.

```bash
cd x86_64
sudo apt install -y ./mscc-init-gui_1.0.13_all.deb
sudo apt install -y ./mscc-ui_0.6.44_amd64.deb
```

Then follow [`INSTALL-UBUNTU.md`](../../../INSTALL-UBUNTU.md) for compilers, PortAudio, tty0tty, and `./linux-build/mscc-linux.sh all`.

Rebuild the amd64 UI package (does not touch arm64):

```bash
./linux-build/mscc-ui-x64.sh
./linux-build/build-mscc-ui-deb-amd64.sh
```

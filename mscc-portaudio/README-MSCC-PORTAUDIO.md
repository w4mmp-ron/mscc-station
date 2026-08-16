# mscc-portaudio

Debian package of **PortAudio with Pulse + ALSA** for Multus MSCC on **Raspberry Pi OS arm64**.

## Install

```bash
sudo apt install -y ./mscc-portaudio_19.8.2_arm64.deb
ldconfig -p | grep portaudio
# expect: /usr/local/lib/libportaudio.so.2
```

Then install **mscc** (binaries built with rpath → `/usr/local`).

## Build the .deb

Uses sibling tree **`portaudio/`** (not portaudio-install):

- `portaudio/build/libportaudio.so*` — AArch64 shared libs  
- `portaudio/include/*.h` — headers  
- `portaudio/build/portaudio-2.0.pc` — optional pkg-config  

```bash
cd mscc-portaudio
./build-deb.sh
# → mscc-portaudio_19.8.2_arm64.deb
```

Override: `PORTAUDIO_ROOT=/path/to/portaudio ./build-deb.sh`

## Contents

| Path | Content |
|------|---------|
| `/usr/local/lib/libportaudio.so*` | Shared library |
| `/usr/local/include/portaudio.h` | Headers |
| `/usr/local/lib/pkgconfig/portaudio-2.0.pc` | pkg-config |
| `/etc/ld.so.conf.d/mscc-portaudio.conf` | ldconfig path |

Does **not** remove Debian `libportaudio2`. MSCC apps use **rpath** so they load this build.

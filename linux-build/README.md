# Linux builds (two trees)

**`rpi/`** = Ron’s Raspberry Pi trees (guide only).  
**`linux/`** = Ubuntu x86_64 working copy. Do not edit `rpi/` for laptop fixes.

## Ron / Pi — no special script

```bash
mscc stop
cd rpi/SDRcore-recv-linux  && make clean && make
cd ../SDRcore-trans-linux && make clean && make
cd ../ms-sdr-linux        && make clean && make
# default: PORTAUDIO=mscc → rpath /usr/local (mscc-portaudio)
ldd $HOME/mscc/sdrcore-recv | grep portaudio   # /usr/local/lib
```

Then copy ELFs into `rpi/mscc-binaries/` and run `rpi/mscc-deb/build-deb.sh`.  
Full notes: [`rpi/mscc-deb/BUILD-SERVERS-ON-PI.md`](../rpi/mscc-deb/BUILD-SERVERS-ON-PI.md).

`make` default is still the Pi link. Do not pass `PORTAUDIO=distro` on the Pi.

## This laptop — x86_64 (special)

Ubuntu uses the same MSCC PortAudio layout as the Pi (`/usr/local`, Pulse+ALSA), plus user-session icons:

```bash
./linux-build/build-mscc-portaudio-amd64.sh
sudo apt install -y ./linux/mscc-portaudio/mscc-portaudio_*_amd64.deb
./linux-build/mscc-linux.sh all
```

Writes **`$HOME/mscc`** as **x86_64**. Never copy those into `rpi/mscc-binaries/`.

Operate UI (Avalonia, linux-x64, self-contained). Does **not** rebuild the Pi `linux-arm64` publish or `mscc-ui_*_arm64.deb`:

```bash
./linux-build/mscc-ui-x64.sh
# $HOME/mscc-ui  +  Super, type MSCC UI
# Connect: 127.0.0.1 port 8888 (MSCC Start first)
```

Needs a user-local **.NET 9** SDK (`$HOME/.dotnet`). Ubuntu 26.04’s distro SDK is 10; the project stays **net9.0**.

Pack the UI as `mscc-ui_*_amd64.deb` (also copies into `installers/linux/`):

```bash
./linux-build/build-mscc-ui-deb-amd64.sh
```

Servers as `mscc_*_amd64.deb` (history in `linux/mscc-deb/`, current kit `installers/linux/`):

```bash
./linux-build/mscc-linux.sh build
./linux-build/build-mscc-deb-amd64.sh
```

After any kit update: `./linux-build/drop-installers.sh`. GitHub web grab folder: [`installers/`](../installers/).

## This laptop — arm64 for the Pi (optional)

Cross-compile Pi binaries here, then stage/upload. Output is **`$HOME/mscc-arm64`**, not `$HOME/mscc`.

```bash
./linux-build/cross-arm64.sh prereqs --install
./linux-build/cross-arm64.sh build
./linux-build/cross-arm64.sh stage    # → rpi/mscc-binaries/ (AArch64 only)
./linux-build/cross-arm64.sh deb      # stage + rpi/mscc-deb/build-deb.sh
```

Link is the same as Ron’s: `PORTAUDIO=mscc`, rpath `/usr/local` (uses the repo’s `mscc-portaudio_*_arm64.deb` at link time).  
`rpi/mscc-deb/build-deb.sh` still refuses non-AArch64 ELFs. Cross-build compiles **`rpi/`** sources, not `linux/`.

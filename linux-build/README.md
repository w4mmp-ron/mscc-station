# Linux builds (one source)

Same C trees. **Ron’s Pi path is unchanged.** Extra scripts are only for this Ubuntu laptop.

## Ron / Pi — no special script

```bash
mscc stop
cd SDRcore-recv-linux  && make clean && make
cd ../SDRcore-trans-linux && make clean && make
cd ../ms-sdr-linux        && make clean && make
# default: PORTAUDIO=mscc → rpath /usr/local (mscc-portaudio)
ldd $HOME/mscc/sdrcore-recv | grep portaudio   # /usr/local/lib
```

Then copy ELFs into `mscc-binaries/` and `mscc-deb/build-deb.sh`.  
Full notes: [`BUILD-SERVERS-ON-PI.md`](../mscc-deb/BUILD-SERVERS-ON-PI.md).

`make` default is still the Pi link. Do not pass `PORTAUDIO=distro` on the Pi.

## This laptop — x86_64 (special)

Ubuntu needs distro PortAudio and user-session icons:

```bash
./linux-build/mscc-linux.sh all
```

Writes **`$HOME/mscc`** as **x86_64**. Never copy those into `mscc-binaries/`.

Operate UI (Avalonia, linux-x64, self-contained). Does **not** rebuild the Pi `linux-arm64` publish or `mscc-ui_*_arm64.deb`:

```bash
./linux-build/mscc-ui-x64.sh
# $HOME/mscc-ui  +  Super, type MSCC UI
# Connect: 127.0.0.1 port 8888 (MSCC Start first)
```

Needs a user-local **.NET 9** SDK (`$HOME/.dotnet`). Ubuntu 26.04’s distro SDK is 10; the project stays **net9.0**.

Pack the UI as `mscc-ui_*_amd64.deb` (drop folder `mscc-ui/Release/avalonia/x86_64/`):

```bash
./linux-build/build-mscc-ui-deb-amd64.sh
```

## This laptop — arm64 for the Pi (optional)

Cross-compile Pi binaries here, then stage/upload. Output is **`$HOME/mscc-arm64`**, not `$HOME/mscc`.

```bash
./linux-build/cross-arm64.sh prereqs --install
./linux-build/cross-arm64.sh build
./linux-build/cross-arm64.sh stage    # → mscc-binaries/ (AArch64 only)
./linux-build/cross-arm64.sh deb      # stage + build-deb.sh
```

Link is the same as Ron’s: `PORTAUDIO=mscc`, rpath `/usr/local` (uses the repo’s `mscc-portaudio_*_arm64.deb` at link time).  
`build-deb.sh` still refuses non-AArch64 ELFs.

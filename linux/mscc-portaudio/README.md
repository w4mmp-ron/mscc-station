# mscc-portaudio (Ubuntu x86_64)

PortAudio with **Pulse + ALSA**, same job as the Pi package, Architecture **amd64**.

History of `.deb` files lives here. Current share copy: [`../../installers/linux/`](../../installers/linux/).

```bash
./linux-build/build-mscc-portaudio-amd64.sh
# → linux/mscc-portaudio/mscc-portaudio_19.8.2_amd64.deb
# also copies into installers/linux/
```

Installs to `/usr/local`. Then rebuild/install `mscc_*_amd64.deb` (rpath `/usr/local`).

# Ubuntu amd64 `mscc` package

History of `mscc_*_amd64.deb` lives **here**. The current share copy is [`../../installers/linux/`](../../installers/linux/).

```bash
./linux-build/mscc-linux.sh build
./linux-build/build-mscc-deb-amd64.sh
# also copies the new file into installers/linux/
```

Architecture **amd64**, PortAudio **`mscc-portaudio_*_amd64.deb`** (rpath `/usr/local`). Do not put these ELFs in `rpi/mscc-binaries/`.

# mscc for the Pi Zero 2 W

    bash rpi/mscc-zero/make-zero-deb.sh installers/rpi/mscc_1.0.109_arm64.deb 1

Makes `installers/rpi/zero/mscc_1.0.109+zero1_arm64.deb`: the normal mscc package
plus two dummy sound devices for a Pi with no sound card of its own. The last
number is the Zero build number; raise it to rebuild the same mscc version.

    OperatorNull     speaker (sound goes nowhere)
    OperatorNullMic  microphone (silence)

recv needs an operator speaker; without one it falls back to the radio's own
receive device. The Zero can't run local audio over its one USB line, so the Zero
package always uses the dummies: every install / upgrade writes them into
operator-speaker.ini / operator-microphone.ini, and MSCC Init offers only them.
ms-sdr (3.217+) then reports "no local operator audio" and the client greys out
Phones. Install the Zero package instead of the normal one, then log out and back
in (or reboot) the first time so PipeWire makes the two devices.

    sudo apt install ./mscc_1.0.109+zero1_arm64.deb

The servers and every other file are the same as the normal package, apart from:

- `/etc/pipewire/pipewire.conf.d/50-mscc-operator-null.conf` (new; kept in this
  folder) makes the two devices.
- MSCC Init's `devices.py` gets `devices-zero-append.py` added at the end, so its
  operator lists show only the two dummies (the sound library MSCC Init uses only
  sees real cards; the servers use MSCC's own, which sees them).
- The postinst gets `postinst-zero-insert.sh` at the end of its configure step
  (writes the two operator ini files, and adds `BUFFER_MS=200` to remote-mic.ini
  once: the Zero's WiFi holds remote mic packets back ~100 ms, more than the
  default 100 ms buffer in trans 3.165+; a BUFFER_MS line already there is kept).

The devices have a low priority, so they never become the desktop's default
sound device.

At a release: build the normal mscc .deb first, then run the command above with
it. Run it in WSL (Git Bash: `MSYS_NO_PATHCONV=1 wsl bash /mnt/c/...`) or on a Pi.
The Zero package goes into every release next to the normal one.

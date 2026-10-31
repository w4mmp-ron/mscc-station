# Windows utility scripts: test tools for the Proficio on the Windows PC

These run on the Windows PC. Open a Command Prompt in this folder, or
double-click where it says so. The Python ones need Python 3 and pyusb
(pip install pyusb). The USB ones use libusb0.dll (libusb-win32, the driver
the Windows servers use) or libusb-1.0.dll, from System32 or next to the
script.

The Windows servers keep their files in %LOCALAPPDATA%\MSCC-NET9.

-------------------------------------------------------------------------------

    audio-stats
    audio-stats mark
    audio-stats since
    audio-stats watch 30

Black Pill health counters, read over USB, when the pill is plugged into this
PC. Safe any time, servers up or down. Same output as on the Pi.
- no word: all counters since the pill started
- mark: remember the counters now, as a starting point
- since: what changed since the last mark
- watch 30: print the changes every half second for 30 s

Bad: last reset = WATCHDOG, a hang record, abandoned or ep0_cut above 0.

-------------------------------------------------------------------------------

    copy-pi-ini.vbs        (double-click)
    copy-pi-ini
    copy-pi-ini -WhatIf
    copy-pi-ini -Source X

Copies the radio settings that are safe to share (calibration and radio
settings) from the Pi to the Windows servers. Ports, sound devices, IP
addresses and other per-computer files are left alone.
- double-click the .vbs: no console window; shows what it will do, asks Yes / No
- copy-pi-ini: copy from V:\ini-files (the Pi's ~/.local/mscc share)
- -WhatIf: show what would be copied, change nothing
- -Source X: copy from another folder

Stop the Windows servers first; it refuses while they run. The current Windows
files are backed up to %LOCALAPPDATA%\MSCC-NET9\backup-<date>-<time>\ first.

-------------------------------------------------------------------------------

    python iq-to-radio.py
    python iq-to-radio.py tx
    python iq-to-radio.py rx
    python iq-to-radio.py --dry-run
    python iq-to-radio.py --show
    python iq-to-radio.py --dir V:\ini-files

Copies the I/Q calibration from the servers' files into the radio, so it
travels with the radio. TX comes from iq.ini (offset and phase), RX from
recv-iq.ini (offset). Bands at 0 are skipped. Needs the pill plugged into this
PC.
- no word: TX and RX, files from %LOCALAPPDATA%\MSCC-NET9
- tx / rx: only one of them
- --dry-run: show what would be sent, send nothing
- --show: only show what the radio holds now
- --dir: take the files from another folder (V:\ini-files = the Pi's files)

It prints which folder it used, and ends with a table, file against radio;
every line should say OK. Safe with the servers up, but not while
transmitting.

-------------------------------------------------------------------------------

    python cal-test.py valid
    python cal-test.py dump
    python cal-test.py get tx 2

Tests the radio's calibration store. valid shows which bands hold a value,
dump shows every band, get shows one (rx, tx or pwr, then the band number:
0 = 10 m ... 9 = 160 m, 10 = 630 m, 11 = 2200 m).
It also has set and test, which WRITE test values into the radio; do not use
those on a calibrated radio. Run it with no word for the full list.

-------------------------------------------------------------------------------

    python rx-iq-balance\rx-iq-fill.py --cal 20,30,40,80 --dry-run
    python rx-iq-balance\rx-iq-fill.py --cal 20,30,40,80
    python rx-iq-balance\rx-iq-fill.py --cal 20,30,40,80 --file V:\.local\mscc\recv-iq.ini

Fills the receive I/Q of the bands you could not calibrate, from a straight
line through the ones you did (named after --cal, in metres). Works on the
Windows servers' recv-iq.ini, or the Pi's with --file. --dry-run only shows
the table; without it, it asks before writing and keeps a backup.
Then restart the servers before saving any other I/Q value, or recv writes its
old values back.

-------------------------------------------------------------------------------

    python tx-iq-balance\iqbd-meter.py
    python tx-iq-balance\iq-phase.py --host proficio
    python tx-iq-balance\iq-auto.py --device 33 --host proficio --phase

Transmit I/Q tools. They need the I/Q Balance Detector on the transmit output,
feeding the 96 kHz sound card, and pip install sounddevice.
- iqbd-meter.py: live display of the LO leak and the image
- iq-phase.py: slider window for the TX phase
- iq-auto.py: finds the best TX offset (and phase with --phase) by itself
They talk to trans on the Pi (--host) and save iq.ini there, not in the radio:
run iq-to-radio.py on the Pi afterwards. Details in tx-iq-balance\README.md.

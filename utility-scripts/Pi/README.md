# Pi utility scripts: test tools for the Proficio on the Pi

Copy these to ~/utilities on the Pi (the I/Q ones go in ~/utilities/iq-scripts).
~/utilities is on the PATH (set in ~/.bashrc), so the first four work by name
from a new terminal. Over ssh, put ~/utilities/ in front of the name.
The others: type python3 and the path, as below.

-------------------------------------------------------------------------------

    audio-stats.py
    audio-stats.py mark
    audio-stats.py since
    audio-stats.py watch 30

Black Pill health counters, read over USB. Safe any time, servers up or down.
- no word: all counters since the pill started
- mark: remember the counters now, as a starting point
- since: what changed since the last mark
- watch 30: print the changes every half second for 30 s

Normal: counters stay flat while receiving and while transmitting.
Bad: last reset = WATCHDOG, a hang record, abandoned or ep0_cut above 0.
On a lockup: run audio-stats.py and copy ~/.local/mscc/*.log BEFORE any reset or
power cycle, or the hang record is lost.

-------------------------------------------------------------------------------

    bash ~/utilities/usb-snap.sh bad

Snapshot when something goes wrong. Run it while the fault is showing, before
restarting anything. The word at the end is a label (bad, good, test1, ...).
Saves the pill counters, a 15 s watch, USB / sound card state and the server
logs into ~/usb-snaps/<date>_<time>_<label>/.

-------------------------------------------------------------------------------

    check-usb

Shows USB messages from the kernel as they happen (asks for sudo). Stop with
Ctrl+C.

-------------------------------------------------------------------------------

    drop-caches

Frees memory used for file caching. Harmless; only for memory tests.

-------------------------------------------------------------------------------

    python3 iq-to-radio.py
    python3 iq-to-radio.py tx
    python3 iq-to-radio.py rx
    python3 iq-to-radio.py --dry-run
    python3 iq-to-radio.py --show

Copies the I/Q calibration from the Pi's files into the radio, so it travels
with the radio. TX comes from ~/.local/mscc/iq.ini (offset and phase), RX from
~/.local/mscc/recv-iq.ini (offset). Bands at 0 are skipped.
- no word: TX and RX
- tx / rx: only one of them
- --dry-run: show what would be sent, send nothing
- --show: only show what the radio holds now

Ends with a table, file against radio; every line should say OK.
Safe with the servers up, but not while transmitting. Use it after
rx-iq-auto.py, iq-auto.py or iq-phase.py, which save to the files only.
(APPLY in the client's I/Q tabs saves to the radio by itself.)

-------------------------------------------------------------------------------

    python3 cal-test.py valid
    python3 cal-test.py dump
    python3 cal-test.py get tx 2

Tests the radio's calibration store. valid shows which bands hold a value,
dump shows every band, get shows one (rx, tx or pwr, then the band number:
0 = 10 m ... 9 = 160 m, 10 = 630 m, 11 = 2200 m).
It also has set and test, which WRITE test values into the radio; do not use
those on a calibrated radio. Run it with no word for the full list.

-------------------------------------------------------------------------------

    sudo python3 rx-iq-balance/rx-image-meter.py

Shows the receive image rejection while you move the RX IQ slider in the client.
Needs a carrier 3-20 kHz from the LO point (signal generator or WWV). Only
listens; changes nothing. Ctrl+C to stop.

-------------------------------------------------------------------------------

    sudo python3 rx-iq-balance/rx-iq-auto.py

Sets the receive I/Q offset by itself for one band and saves recv-iq.ini.
1. Signal generator into the rig, strong level.
2. Client: RX IQ tab, START.
3. Set the generator 6 kHz below the frequency shown on the tab.
4. Run the script.
5. When it says "saved", press START again to end. Next band: back to 2.
Do not touch the I/Q slider while it runs, and do not press APPLY afterwards.
Then run iq-to-radio.py rx to put the result in the radio.

-------------------------------------------------------------------------------

    python3 rx-iq-balance/rx-iq-fill.py --cal 20,30,40,80 --dry-run
    python3 rx-iq-balance/rx-iq-fill.py --cal 20,30,40,80

Fills the receive I/Q of the bands you could not calibrate, from a straight
line through the ones you did (named after --cal, in metres). --dry-run only
shows the table; without it, it asks before writing and keeps a backup.
Then restart mscc before saving any other I/Q value, or recv writes its old
values back.

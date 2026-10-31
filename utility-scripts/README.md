# utility-scripts: test and utility scripts for the Proficio

This folder is the one place for every utility and test script: health
counters, snapshots, calibration tools and the like. They are tools for
people working on the radio, not part of the servers, the client or the
firmware, and they are not installed by the MSCC packages.

Ron, 2026-10-31: any NEW utility or test script goes here, nowhere else.

-------------------------------------------------------------------------------

    Pi\         scripts that run on the Raspberry Pi
    Windows\    scripts that run on the Windows PC

Each subfolder has its own README.md: what each script does and how to run it,
command first, then a few plain lines.

A script that runs on both goes in both folders. The Windows copy gets the
Windows USB handling (libusb0 / libusb-1.0, as audio-stats.py has) and reads the
Windows servers' files from %LOCALAPPDATA%\MSCC-NET9; the Pi copy reads
~/.local/mscc.

-------------------------------------------------------------------------------

Rules:
- A new script gets an entry in its folder's README.md.
- Pi scripts are copied to ~/utilities on the Pi (the I/Q ones to
  ~/utilities/iq-scripts). ~/utilities is on the Pi's PATH.
- Pi\ keeps Linux line endings (Pi\.gitattributes): shell scripts with Windows
  line endings do not run on the Pi.
- Build, packaging and flashing scripts stay with their projects; they do not
  go here.

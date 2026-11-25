# MSCC Documentation

Click a guide to open it.

## Using MSCC

| Guide | Read this if |
| --- | --- |
| [Operator's Guide: Windows](MSCC-Operators-Guide-Windows-Release-0.pdf) | you run MSCC on a Windows PC |
| [Operator's Guide: Linux (Ubuntu and Raspberry Pi)](MSCC-Operators-Guide-Linux-Release-0.pdf) | you run MSCC on Ubuntu or a Raspberry Pi |
| [Operator's Guide: Remote Operation](MSCC-Operators-Guide-Remote-Release-0.pdf) | you run the client on one computer and the radio on another, on your home network |
| [Software Versions Addendum](MSCC-Software-Versions-Addendum.pdf) | you want the version numbers in the current release. The guides refer to it. |

## Installing

| Guide | Read this if |
| --- | --- |
| [Windows install guide](../../installers/windows/INSTALL.md) | you are installing or updating MSCC on Windows |
| [Raspberry Pi install guide](../../installers/rpi/INSTALL.md) | you are installing or updating MSCC on a Raspberry Pi |

Download the installers from the [Releases page](https://github.com/w4mmp-ron/mscc-station/releases).

## Radio firmware

The Black Pill is a replacement for the PSoC processor in our transceivers. A radio has one or the other, never both.

| Guide | Read this if |
| --- | --- |
| [Updating Firmware](MSCC-Updating-Firmware.pdf) | you want to know which firmware your radio has, or need to update the keyer |
| [Burning PSoC Firmware](../MSCC-Burning-PSoC-Firmware.md) | your radio has the original PSoC processor |
| [Burning Black Pill Firmware](../MSCC-Burning-Black-Pill-Firmware.md) | your radio has a Black Pill board |

## What changed, and help

- What changed in each release: [Windows change log](../../installers/windows/CHANGELOG.md), or the notes on the [Releases page](https://github.com/w4mmp-ron/mscc-station/releases).
- Questions and problems: the [Multus forum](https://multussdr.groups.io/g/main/topics).

---

*For maintainers:* after editing the Addendum or Updating Firmware `.md` here, rebuild the PDF:
`python make-addendum-pdf.py` (Addendum) or `python make-addendum-pdf.py MSCC-Updating-Firmware.md`.

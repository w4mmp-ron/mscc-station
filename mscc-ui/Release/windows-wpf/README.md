# Windows WPF release drop

Build outputs and Windows installers for standalone MSCC on Windows.

Typical contents:
- `ms-sdr-MKII.exe`, `mscc-recv.exe`, `Mscc-trans.exe` (+ pdb/dlls)
- `mscc-net9-R9-*-install.exe` Advanced Installer packages

Deploy/runtime folder used by day-to-day ops is still `C:\mscc-net9`.
VC++ projects currently post-build into this folder (`OutDir`).

MSCC backend server launcher
============================

Location:  same folder as ms-sdr / recv / trans (e.g. C:\mscc-net9)

  MSCC-Remote.exe                    Preferred: WinForms app + desktop icon
                                     (start/stop/restart, legacy/MKII, shortcut button)
  Start-MsccServers.bat              Menu or silent start/stop (scripts / boot)
  runhidden.vbs                      Helper (used by the bat)
  Install-MsccServers-AtBoot.bat     One-click: start backends at logon

Does NOT start MSCC.Wpf.exe.
CLI for the GUI tool:  MSCC-Remote.exe start|stop|restart|status|legacy|mkii|keyer

Interactive menu
----------------
  Double-click Start-MsccServers.bat
  [1] Start  [2] Stop  [3] Restart  [4] Status
  [5] Legacy CW / external keyer  (PROFICIO-MKII=0 + restart)
  [6] MKII internal keyer         (PROFICIO-MKII=1 + restart)
  [7] Exit

Silent / scripted (no menu, no pause)
-------------------------------------
  Start-MsccServers.bat start      Start backends (skips if already running)
  Start-MsccServers.bat silent     Same as start
  Start-MsccServers.bat auto       Same as start
  Start-MsccServers.bat stop
  Start-MsccServers.bat restart
  Start-MsccServers.bat status     Processes + PROFICIO-MKII line

Legacy / external electronic keyer (host flag)
----------------------------------------------
  ms-sdr reads %LocalAppData%\MSCC-NET9\mscc.ini at process start:

    PROFICIO-MKII=1   Proficio MKII internal PIC keyer (default)
    PROFICIO-MKII=0   Legacy radio / external electronic keyer
                      (HOLD only among keyer USB; no rear PTT sense)

  No full MSCC UI required on the radio PC:

    Start-MsccServers.bat legacy   Write PROFICIO-MKII=0 and restart backends
    Start-MsccServers.bat mkii     Write PROFICIO-MKII=1 and restart backends
    Start-MsccServers.bat keyer    Show current mode only (no restart)

  Remote Windows client: Launch Servers OFF, point Server IP at this PC,
  match CW tab "external electronic keyer" checkbox for UI gray-out, then Start.

  Full local/remote guide:
    docs\EXTERNAL-KEYER-LOCAL-REMOTE.md  (in the MSCC source tree)

Order: Mscc-trans → mscc-recv → ms-sdr

Start when Windows user gets a desktop (recommended)
----------------------------------------------------
  Double-click:  Install-MsccServers-AtBoot.bat
  (UAC → Yes)

  Creates Scheduled Task "MSCC Backend Servers":
    Trigger : At logon (this user)
    Account : Current user — NOT SYSTEM
    Action  : Start-MsccServers.bat start

  Works for:
    - Password login (starts after you sign in)
    - Auto-login / no password (starts when desktop appears)

  Remove:   Install-MsccServers-AtBoot.bat remove
  Status:   Install-MsccServers-AtBoot.bat status

  After changing legacy/mkii, run "legacy" or "mkii" once (includes restart),
  or edit mscc.ini and run "restart".

Client on another PC
--------------------
  On the radio/server machine: install logon task as above; set keyer mode
  with legacy/mkii when needed.
  On the client PC: MSCC.Wpf with "Launch Servers" unchecked;
  point Server IP at the radio machine.

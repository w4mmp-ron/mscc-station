# Overseer intent

**Updated:** 2026-10-02 (merge cmd-062 Windows + cmd-063 firmware)
**Overseer:** Build Commander

## Status
- **Done - cmd-062 (NEW-HP):** Windows ms-sdr CMD_SET_PARK_CAL_SETTINGS (0x29) + WPF Save settings to host park (no client CalPark). WPF 10.2.0 / ms-sdr 3.181. Brief `briefs/cmd-062.md`.

- **Done - cmd-063 (ubuntu-stew):** optional `mscc-firmware` 1.0.0 + Load File default `/usr/share/mscc/firmware`. Commit 54054c1; desktop helper shipped in rebuilt mscc 1.0.49. Brief `briefs/cmd-063.md`.
- **Done - cmd-061 (ubuntu-stew):** host park/restore + Avalonia Save settings 0x29. Commit 1327cd0. mscc 1.0.49 + mscc-ui 0.6.71.
- **Done - cmd-060 (ubuntu-stew):** factory/ in mscc 1.0.48 amd64 + 1.0.51 arm64 packaging. Commit 0f92239 / ACK 4c7a977.
- **Active - cmd-059 (NEW-HP):** cmd-055 follow-up — dead Factory_mirror_live_to_cal / Create_power_cal_file (verify uncalled) + brief visible Save-settings tip on QRP/AMP/TX IQ tabs; shorten ToolTips. No park/restore behavior change. Brief `briefs/cmd-059.md`.
- **Active - cmd-058 (ubuntu-stew):** Avalonia 0.6.70 SPECTRUM RESOLUTION (800/1600/3200) parity with WPF. Brief `briefs/cmd-058.md`. Do not touch Core/WPF/rpi/bling.
- **Active - cmd-056 (NEW-HP):** Windows FREQ CAL second-STOP - port Linux else if (cal_abort_pending) into ms-sdr-MKII/source/calibrate.c. Do not touch `rpi/`. Brief `briefs/cmd-056.md`. After Stew pushes, Ron ports Pi.
- **Active - cmd-057 (NEW-HP):** Remove CW-tab PHONES checkbox (WPF only, BL-001). Keep POTENTIA/QSK. Avalonia later. Brief `briefs/cmd-057.md`.
- **Active - cmd-055 (NEW-HP):** Windows factory_seed drop mirror + live-wins load_iq_or_power (Ron Q4). Brief `briefs/cmd-055.md`. With/after cmd-048 Windows.
- **Manuals:** Ron answered QUESTIONS-FOR-RON; paste-ready in MANUAL-UPDATES-FROM-RON.md (PDFs not regenerated).
- **Backlog BL-001:** WPF = cmd-057; Avalonia still open. Second-STOP Windows = cmd-056; Pi waits for push.

- **Active - cmd-045 (NEW-HP WPF):** Stew round-2 fixes: VFO B saved/restored across restart; FREQ CAL Stop clears the running notice (AUTO/CHECK only), Start with tab open re-applies CW/600/200, closing on the tab restores mode/filter/pitch; CHECK FAILED / after-RESET warning text; "MANUAL steps" label; QRP / Full Power / AMP synced from AmpOn (no rename, pending Ron); Proficio/Geminus button removed (FW gating stays); A8 tooltip + DIG-U slider uses Tune power. See `briefs/cmd-045.md`.
- **Active - cmd-044 (NEW-HP WPF + Windows ms-sdr):** Stew-approved fix list: WPF tooltip/text fixes (A/B/C) + remove dev-notes lines (QRP CAL/AMP CAL/TX IQ); Host/Port change applies on next Start (UdpRadioService.SetRemoteEndpoint); FREQ CAL readable colors, local progress count, CHECK sends LOOSE, tab entry -> CW/600/200 and restore on leave; ms-sdr-MKII calibrate.c progress counter reset + failed cal restores previous mode (not AM). Avalonia + Linux/Pi ms-sdr ride with cmd-043 (reserved, Ron's Linux port). See `briefs/cmd-044.md`.
- **Active - cmd-041 + cmd-042 (DIG-U narrow Hi 1.4k/1.0k; Stew chose A):** cmd-041 WPF adds Hi idx 5=1.4k / 6=1.0k in DIG-U (+ 0xDD default-echo guard). cmd-042 SDRcore-recv 0xD1 cases 5/6 (recv 3.141). Order: NEW-HP (041 + 042 Windows mscc-recv.exe) -> Stew push -> ubuntu-stew mscc 1.0.45 amd64 -> Stew push -> rpi mscc 1.0.45 arm64. ms-sdr: no clamp, no edit. See `briefs/cmd-041.md` + `briefs/cmd-042.md`.
- **Active - cmd-040 (Avalonia, ubuntu-stew then rpi):** remote-audio parity 0.6.60 — independent RemoteDigitalAudio / PathPhones|PathDigital; Remote Digital mic slider (REMOTE_DIGI_MIC_VOL default 100); main Phones/Digital disabled while Remote (stricter than WPF); 0xBC ownership only (no PttOn/TuneMode from TxSetByServer). Orders on NEW-HP. See COMMANDS.yaml + `briefs/cmd-040.md`.
- **Peer - cmd-039 (rpi):** done — remote_mic diagnostic EVENT logging; fill unchanged; cmd-034 still on hold.
- **Peer - cmd-038 (NEW-HP WPF):** pending/orders — client Mic TX EVENT logging (diagnostic only).
- **On hold - cmd-034 (rpi):** remote_mic clear-on-TX + fixed 2:1 + silence on underrun — do not implement while logging / Avalonia parity ship.

## Short list
- **cmd-063 ubuntu-stew** DONE (mscc-firmware + Load File default)
- **cmd-062 NEW-HP** DONE (Windows host park 0x29 + WPF Save to host)
- **cmd-061 ubuntu-stew** DONE (park/restore 1327cd0)
- **cmd-060 ubuntu-stew** DONE (factory packaging 0f92239)
- **cmd-059 NEW-HP** (cmd-055 tip/dead-code follow-up), commit locally, Stew pushes
- **cmd-058 ubuntu-stew** (Avalonia spectrum/pan resolution 0.6.70), commit locally, Stew pushes
- **cmd-056 NEW-HP** (ms-sdr second-STOP), commit locally, Stew pushes then Ron Pi
- **cmd-057 NEW-HP** (WPF remove CW PHONES / BL-001), commit locally, Stew pushes
- **cmd-055 NEW-HP** (factory_seed), with/after cmd-048 Windows
- **cmd-045 NEW-HP** (WPF 9.26.4), commit locally, Stew pushes
- **cmd-044 NEW-HP** (WPF 9.26.x + ms-sdr-MKII), commit locally, Stew pushes
- **cmd-041 + cmd-042 NEW-HP first** (WPF + Windows recv), then Ubuntu 1.0.45, then Pi 1.0.45
- **cmd-040 Avalonia next** (ubuntu-stew amd64 smoke, then rpi arm64 kit)
- Digi mush parked (A/B + EVENT logs; no fill change)
- EVENT logging cleanup later (client cmd-038 / host cmd-039 correlators)
- Unhold cmd-034 later

## Coord workflow

Write orders on the **canonical host** (NEW-HP for shared repo yaml) or **target host** first; push later. Call the user **Stew**.
Avalonia Build prefer **ubuntu-stew**, then **rpi** kit. Do not push unless Stew asks.

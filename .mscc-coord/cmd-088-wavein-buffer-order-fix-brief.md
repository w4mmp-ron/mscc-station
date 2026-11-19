# cmd-088 — Windows mic capture: hand off buffers in fill order (Shack)

**STOP POINT (up front):** Build both projects, then stop. Do NOT commit. Stew tests on the SA first.
Commit only when Stew says "finished". Stew pushes.

Host: Shack. Follow repo-root `CLAUDE.md` shared rules. Pull first. Hands off `rpi/`, `Solidus/`,
`installers/rpi/`, and leave NEW-HP alone (it has uncommitted debug work). Keep it small: no WASAPI,
no format changes, no device-selection changes.

## Why
Background: `.mscc-coord/REPORT-FOR-RON-WINDOWS-CLIENT-DIGITAL-TX-BURSTS-2026-11-17.md` and
`.mscc-coord/NOTE-FOR-RON-DIGITAL-TX-BURSTS-NAUDIO-BUFFER-ORDER-2026-11-18.md`.
NAudio 2.2.1 `WaveInEvent` scans its buffers in array order 0..N-1 on each wake-up. When the thread
wakes late and 2+ buffers are done, a newer buffer is handed out (and re-queued) before older ones.
Whole 10 ms chunks go out of order for about one trip round the 8 buffers (80 ms), which is the
burst on Stew's SA. MSA1 seq numbers are clean because packets are numbered after the mix-up.

## Change
1. New file `OrderedWaveIn.cs` in `MSCC.Wpf/RemoteAudio/` (namespace to match). A copy of NAudio
   2.2.1 `WaveInEvent` (MIT, keep NAudio's license/credit line at the top) with ONE behaviour change:
   - Keep `int _next` (index of the oldest queued buffer, starts at 0 after the buffers are queued in
     order 0..N-1).
   - On each wake: `while (buffers[_next].Done) { raise DataAvailable; Reuse(); _next = (_next+1) % N; handed++; }`
     so buffers are handed out and re-queued strictly oldest first.
   - If `handed > 1`: `Interlocked.Increment(ref _lateWakes)` and track `_maxAtOnce = max(handed)`.
   - Public read-and-reset: `(int lateWakes, int maxAtOnce) TakeTimingStats()`.
   - Same public surface the sender uses today: `DeviceNumber`, `WaveFormat`, `BufferMilliseconds`,
     `NumberOfBuffers`, `DataAvailable`, `RecordingStopped`, `StartRecording`, `StopRecording`,
     `Dispose`, raising the same `WaveInEventArgs` / `StoppedEventArgs`.
   - Use NAudio's public `WaveInterop` / `WaveInBuffer` if they are public in 2.2.1; if not, copy the
     needed P/Invoke and buffer code into the file. Do not change the NAudio package version.
2. `MSCC.Wpf/RemoteAudio/RemoteMicSender.cs`: replace `WaveInEvent` with `OrderedWaveIn`. Nothing
   else changes (8 x 10 ms buffers, ring, send loop, MSA1 all stay).
   Add the stats to the existing periodic log line:
   `Mic TX {n} pkts → {ep} peak=… drops=… underruns=… late={lateWakes} maxq={maxAtOnce}`.
3. `mscc-remote-audio/MsccRemotePhones/Audio/RemoteMicSender.cs`: same swap, using a copy of the
   same `OrderedWaveIn.cs` in that project (or a linked file, whichever builds cleanly). Add
   `late=… maxq=…` to its `Mic TX stopped` log line.
4. Leave `MsccAudioTestSender` alone.
5. Bump the WPF client version (`ClientVersion.txt`) to the next number after what is on main after
   the pull (11.18.4 if Ron has not released past 11.18.3), and the Phones app version if it has one.
   No installer/payload/aip changes in this cmd; that comes at "finished".
6. Build Release for MSCC.Wpf and MsccRemotePhones. Run the WPF client from the build output (or
   copy where Stew runs it, tell him which).

## Test (Stew, SA)
Shack WPF client -> NEW-HP servers, Remote Digital, WSJT-X TUNE about 30 s:
- [ ] Let it run 15 s on its own, watch the SA for bursts.
- [ ] Move the WSJT-X drive slider up and down, fast and slow.
- [ ] Move the Digital/remote MIC gain slider.
- [ ] Check the client debug log `Mic TX … late= maxq=` lines.
Pass = no bursts around the carrier. `late` above 0 with no bursts proves the cause and the fix.
`late` = 0 and bursts still there = something else; stop and report.
Key-down/key-up pulse and the startup comb are separate items, not part of this cmd.

## Report back
Files changed, versions, build result, where to run it from. Update AGENTS.md "Current work"
(cmd-088 active on Shack) and `.mscc-coord/status/` for Shack.

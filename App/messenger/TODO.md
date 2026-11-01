# Messenger: open points

State 2026-11-01: host-tested, not yet run on a radio. Two UV-K1 are on order
for the bench.

## Bench

The bench tests in [README.md](README.md#bench-tests), and in particular:

- **Audio and decoder:** level at the ADC (`LV`), decoder load (`DSP`) and late
  blocks (`OV`) with USB and K5Viewer streaming active.
- **Interop:** receive from and send to a Kenwood (TH-D74/D75), a Yaesu
  (FT3D/FT5D) and APRSdroid; round trip through an iGate and aprs.fi.
- **APRS hold:** ALWAYS keeps the receiver on the APRS VFO with dual watch on;
  with 10/30/60 s it looks at the other VFO and comes back, but not during or
  1.5 s after a burst, nor while an ack waits; dual watch is not stuck after a
  voice reception or PTT on the other VFO; Full Watch too. The status bar shows
  `APRS` while the receiver is on the APRS VFO (hold logic and status mark run
  on the radio only, no host test).
- **TX timing:** interrupts stay enabled while the AFSK bits play (the KISS
  TNC app masks them). Check clean tone changes with K5Viewer streaming over
  USB, decoded by a Yaesu/Kenwood/Direwolf. If needed, mask interrupts around
  the bit loop of `msg_hw_transmit()` as `apps/kisstnc` does.
- **Current draw:** power save is blocked while a VFO sits on the APRS
  frequency. Measure standby with and without the messenger (K1 batteries
  1400 and 2500 mAh).
- FM radio with APRS: bursts over the music, AF mute restored afterwards.
- Battery voltage plausible while the capture runs.
- Position beacon on aprs.fi: position, symbol, comment.
- Lobby: a CQ from a Yaesu/Kenwood group code arrives; ours shows there.
- IS net: join `HOTG` through an iGate, receive the group's messages
  (#APRSThursday).

## Size

On v6.3 only ~200 B of flash are left (CI build, GCC 13.3, multiboot stub
included). Before adding anything: free space first. Candidates: the logo
screen saver `ENABLE_FEAT_F4HWN_LOGO_SAV` (1 KB), the audio scope
`ENABLE_FEAT_F4HWN_AUDIO_SCOPE` (0.2 KB), table-driven setup items in
`msg_ui.c`.

## Platform: F4HWN v7

v7 (upstream branch `feature_update_v7`, not released on 2026-11-01) builds
the overlay apps (APRS RX/TX/DIGI, KISS TNC, ...) into Fusion and moves fonts,
menu texts and QR codes into per-slot firmware assets in the external flash.

- Rebase onto the v7.0.0 tag: Messenger = v7 Fusion minus spectrum and RF log
  plus the messenger, with the overlay apps (the KISS TNC app covers the
  phone-as-terminal case; it is not used together with voice anyway).
- Replace the CMake ban on `ENABLE_MESSENGER` + `ENABLE_FEAT_F4HWN_OVERLAY_APPS`
  by sharing the PA4 capture: stop the messenger capture when an overlay app
  starts (apps are modal, `MSG_TimeSlice10ms` does not run meanwhile); it
  restarts by itself afterwards. Check the launch path in `app_overlay.c`,
  `af_capture_open` and the battery ADC hooks in `board.c`.
- Size: v7 Fusion without spectrum and RF log uses 94 872 B; with the 888 B
  multiboot stub 25 072 B are left, the messenger is ~27.5 KB: ~2.5 KB short.
  Options: messenger strings and tables into the firmware assets (v7's
  `FW_ASSET` mechanism), a smaller UI, dropping `LOGO_SAV` (1 KB) or the
  crypto (1.9 KB).
- The v7 `.bin` is a package (image + asset pack + `FWAT` trailer); UV Studio
  flashes both. CI and the web flasher must publish that package.
- Flash region: keep 0x1BA000-0x1C9FFF (free in v7). Watch upstream
  `App/driver/mb_flash.h` and `App/apps/app_overlay.h`: v7 says "100 slots
  max"; slot 92 would start at 0x1BA000.
- Side-key action `MESSAGES` = 25: upstream will likely give 25 to its next
  action. On the rebase, append ours after upstream's last action. Upstream
  firmware resets an unknown action to its default, so going back is harmless.

## Interop

- Tocall: `APZK5M` works, but `APZ*` is the shared "experimental" range, so
  aprs.fi shows the device as "Experimental" and another experiment could use
  the same code. A registered one is free: a pull request or issue at
  github.com/aprsorg/aprs-deviceid (`tocalls.yaml`, maintained by OH7LZB, for
  aprs.fi), a few YAML lines, then change `MSG_TOCALL`. Owner's decision.
- CHIRP driver (`chirp/`): on each rebase take the new F4HWN driver and add
  `"MESSAGES"` to `KEYACTIONS_LIST` again (also if the action number moves).
- Station list with positions and distances: decode uncompressed, compressed
  and Mic-E positions (Mic-E ~1 KB, see `apps/aprsrx`).
- Send reply-acks (`{MM}AA`) to stations that use them: one frame less.

## Ideas (size first)

- **Native KISS TNC** (no overlay app): the demodulator, AFSK transmitter and
  AX.25 code are already resident; missing are KISS framing on the USB serial
  port, a mode switch (the port also carries UV Studio's protocol and
  K5Viewer, which must stay quiet while a KISS client is connected) and a rule
  for messages (the messenger must not ack what the phone app handles: pause
  the core, or use another SSID). Estimate 0.8-1 KB: on v6.3 only by dropping
  something (the logo screen saver gives 1 KB); otherwise with v7.
- Tune the APRS VFO from the setup (one key: VFO B to the APRS frequency,
  FM, wide, simplex). Not done: it writes VFO settings of the base firmware;
  ask before spending the bytes.
- Tell upstream (an issue at armel/uv-k1-k5v3-firmware-custom) which flash
  region the fork uses, so a future v7.x can leave it free. Owner's decision;
  a draft is in the conversation of 2026-11-01.

## Decisions

- Encryption stays available off the amateur bands, PMR446 preset included:
  the firmware is used worldwide and the rules differ; each user checks them
  (tests into a dummy load). Off by default, refused on 144-146 and 430-440 MHz,
  needs a key.
- Multiboot stays enabled. Moving the region removed the conflict with the v7
  asset zones; disabling multiboot would cost Multiboot and Multiconfig, and
  v7's firmware assets depend on it.
- Upstream changes are merged by the fork itself (no upstreaming).
- Messenger and KISS TNC together wait for v7 (or a native KISS TNC); until
  then F4HWN Labs in a Multiboot slot.
- No reservation requested upstream: the ownership guard keeps the messenger
  from overwriting other data if upstream ever uses the region.

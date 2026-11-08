# Messenger: open points

State 2026-11-01: host-tested, not yet run on a radio. Two UV-K1 are on order
for the bench.

## Next: move to F4HWN v7 (first priority, as soon as v7.0.0 is released)

v7 (upstream branch `feature_update_v7`, not released on 2026-11-01) builds
the overlay apps (KISS TNC, APRS RX, APRS DIGI, ...) into Fusion and moves
fonts, menu texts and QR codes into per-slot firmware assets in the external
flash. That gives the apps (KISS TNC needs no work of ours) and upstream's
fixes. Start right after the v7.0.0 tag:

1. Rebase `messenger` onto v7.0.0. Messenger = v7 Fusion minus spectrum and RF
   log plus the messenger, overlay apps kept. Re-check every hook (`msg_fw.h`)
   against the moved upstream code.
2. Share the PA4 audio capture instead of the CMake ban on `ENABLE_MESSENGER`
   + `ENABLE_FEAT_F4HWN_OVERLAY_APPS`: stop the messenger capture when an
   overlay app starts (apps are modal, `MSG_TimeSlice10ms` does not run
   meanwhile); it restarts by itself afterwards. Check the launch path in
   `app_overlay.c`, `af_capture_open` and the battery ADC hooks in `board.c`.
3. Make room (v7 Fusion without spectrum and RF log: 94 872 B, 25 072 B left
   with the multiboot stub; the messenger is ~27.5 KB, the logo screen saver
   is already gone): move the messenger's texts and tables into v7's firmware
   assets (`FW_ASSET`, `FW_ASSETS_Read`), ~1.5 KB, or trim the UI.
4. CI and web flasher: the v7 `.bin` is a package (image + asset pack +
   `FWAT` trailer) that UV Studio flashes in two steps; publish that package.
5. Side-key actions `MESSAGES` and `APRS ON/OFF`: append them after
   upstream's last action (25 and 26 may be taken by then). Upstream firmware resets an unknown action to its
   default, so going back stays harmless.
6. CHIRP driver: take the v7 F4HWN driver and add `"MESSAGES"` and
   `"APRS ON/OFF"` again.
7. Flash region 0x1BA000-0x1C9FFF stays (free in v7). F4HWN answered
   (2026-11-01): reserved for app slots 92-99, may be used or erased without
   notice, forks handle their own migrations. On every rebase check
   `APP_SLOT_COUNT` and any erase of the app area (`App/apps/app_overlay.c`,
   `App/driver/mb_flash.h`) and UV Studio's app slot count; before upstream
   uses slots 92 and up, move the region with a migration (copy settings and
   mailbox, then erase the old place).
8. Bench again on the radio, then a release.

## Bench

Run [BENCH.md](BENCH.md) with the two UV-K1 and bring the filled-in results
table. The decisions that wait for it (interrupt masking during TX, tone
level and twist) are listed there; *APRS hold* is decided (100 ms default,
2026-11-07).

## Size

On v6.3 about 0.1 KB of flash are left (CI build estimate, GCC 13.3,
multiboot stub included) after *Beacon now* and the monitor (2026-11-08,
~480 B). Kept on purpose; the first thing to drop when a bench fix needs room
is the audio scope `ENABLE_FEAT_F4HWN_AUDIO_SCOPE` (measured: 200 B), then
table-driven setup items in `msg_ui.c`.

## Interop

- Tocall: `APDLPW` requested on 2026-11-01
  (github.com/aprsorg/aprs-deviceid/issues/402). When it is assigned (maybe
  another code): change `MSG_TOCALL`, the tests that expect `APZK5M`, the
  badge and the notes in both READMEs and DESIGN.md, then make a release.
- Send reply-acks (`{MM}AA`) to stations that use them: one frame less.

## Ideas, after v7 (size first)

- **Positions in the station list:** decode uncompressed, compressed and Mic-E
  positions of the stations heard, show distance and direction from the own
  position. Mic-E alone is ~1 KB (see `apps/aprsrx`).
- **Signal strength in the station list** (~150 B): read the RSSI when a frame
  is taken from the decoder (the carrier is still there for the tail flags)
  and show it per station. A passive range check.
- **Answer the standard APRS queries** (~300 B): `?APRSP` (send the position),
  `?APRSS` (the status), `?PING?` / `?APRST` (the path). Other APRS clients
  can then check whether this radio is reachable; ours would offer "ping" by
  sending such a query. GOGUFW's multi-radio Range Check does the same for its
  own FSK protocol.
- Quick messages (a few saved texts to send with two keys), as GOGUFW has.

## Discarded for now

- **Native KISS TNC** (no overlay app): ~0.8-1 KB for KISS framing on USB, a
  port mode switch (UV Studio and K5Viewer share the port) and pausing the
  messenger while a phone app handles messages. Not needed while v7 brings
  the KISS TNC app and KISS is not used together with voice.
- **Tune the APRS VFO from the setup** (VFO B to the APRS frequency, FM, wide,
  simplex): 150-300 B and writes VFO settings of the base firmware for a
  one-time step. The README recommends a memory channel "APRS" instead.

## Decisions

- Encryption stays available off the amateur bands, PMR446 preset included:
  the firmware is used worldwide and the rules differ; each user checks them
  (tests into a dummy load). Off by default, refused on 144-146 and 430-440 MHz,
  needs a key.
- Multiboot stays enabled. Moving the region removed the conflict with the v7
  asset zones; disabling multiboot would cost Multiboot and Multiconfig, and
  v7's firmware assets depend on it.
- Upstream changes are merged by the fork itself (no upstreaming). The owner
  informs upstream about the flash region with an issue (prepared 2026-11-01).
- Messenger and KISS TNC together wait for v7; until then F4HWN Labs in a
  Multiboot slot.
- The ownership guard keeps the messenger from overwriting other data if
  upstream ever uses the region.

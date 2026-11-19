# Messenger: open points

State 2026-11-19: beta.1 ran live on two UV-K1 (F4HWN v6.3, 2026-11-07).
Branch `messenger-v7` is on F4HWN v7.0.0 with the overlay apps, builds and
fits, not yet run on a radio.

## Now: finish the move to F4HWN v7

Done on 2026-11-19 in branch `messenger-v7` (F4HWN v7.0.0 was released on
2026-11-18):

1. **Merged** upstream `main` (the v7.0.0 tag plus "Fix support safe slot
   reordering in Fusion", which the published v7.0.0 image already carries).
   A merge, not a rebase: `messenger` has published release tags. Conflicts
   only in `main.c` (`MSG_Init()` before upstream's power-on resume of an
   app), `CMakePresets.json`, `compile-firmware.sh` and `NOTICE`; the other
   hooks merged unchanged. Messenger = v7 Fusion minus spectrum, RF log, QR
   codes and logo screen saver, plus the messenger; the overlay apps stay.
2. **PA4 shared** with the overlay apps: the CMake ban is gone;
   `app_launch()` calls `MSG_Suspend()` / `MSG_Resume()` around each app (see
   DESIGN.md, *Sharing with the overlay apps*).
3. **Room:** the display texts of `msg_ui.c` are firmware assets (`A()`,
   `AT()`), 2.2 KB less internal flash: image 118 716 B, 2 116 B left; asset
   pack 11 403 B, 4 981 B left in the 16 KiB zone. The audio scope stays: on v7
   its code is built for the apps anyway, and dropping it costs 184 B.
4. **LTO** (on in v7): compiler barriers around the frame slot and the DMA
   buffer (`aprs_demod.c`, `msg_hw.c`).
5. **CI and web flasher:** the CMake build already writes the v7 package as
   `f4hwn.messenger.bin`; UV Studio flashes the image and then the assets.
   The page explains the second step. Pushes to `messenger-*` branches build
   and test without touching GitHub Pages.
6. **Side keys:** actions 25 and 26 are still free in v7, numbers unchanged.
7. **CHIRP:** the F4HWN v7.0.0 driver plus MESSAGES and APRS ON/OFF.
8. **CI:** `messenger-v7` pushed; the CI passes all host tests (the screen
   simulator with the asset trap included) and builds the same image,
   118 716 B (run 44513421628).
9. **Flash region** checked for v7.0.0: 64 app slots end at 0x181FFF, the
   asset zones start at 0x1CA000, the app resume journal is in security
   register 1; 0x1BA000-0x1C9FFF stays unused. Check again on every merge
   (`APP_SLOT_COUNT`, `App/driver/mb_flash.h`, UV Studio's slot count); before
   upstream uses slots 92 and up, move the region with a migration (copy
   settings and mailbox, then erase the old place).

Left:

- Bench on the radio with the v7 build: [BENCH.md](BENCH.md), tests 1-25
  (21-25 are new: assets, apps alongside the messenger, power-on resume,
  KISS TNC).
- Then merge into `messenger` and release (a new minor version: new base).

## Bench

Run [BENCH.md](BENCH.md) with the two UV-K1 and bring the filled-in results
table. The decisions that wait for it (interrupt masking during TX, tone
level and twist) are listed there; *APRS hold* is decided (100 ms default,
2026-11-07).

## Size

On v7 about 2.1 KB of flash are left (image 118 716 B of 120 832 B, multiboot
stub included, Arm GNU Toolchain 13.3.rel1 with LTO as in the CI). Keep a
reserve for bench fixes. Next candidates if needed: the remaining texts of
`messenger.c` and `t9.c` (~0.3 KB, but some are protocol data), table-driven
setup items in `msg_ui.c`.

## Interop

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
- **GPS from K5Viewer** (v7 API level 4): the position a Tab5 K1Viewer or
  another K5Viewer host sends (kept 15 s in RAM) for the beacon, before the
  locator or the fixed position.
- **One callsign:** take it from the APRS Edit profile that v7's APRS apps use
  (APRS DIGI reads it since v7), or write ours there.

## Discarded for now

- **Native KISS TNC** (no overlay app): ~0.8-1 KB for KISS framing on USB, a
  port mode switch (UV Studio and K5Viewer share the port) and pausing the
  messenger while a phone app handles messages. Not needed: v7's KISS TNC
  app runs on this edition, and KISS is not used together with voice.
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
  told upstream about the flash region (issue 758, 2026-11-01).
- Messenger and the overlay apps share the radio in turns: an app owns PA4,
  the ADC and the flash sector cache while it runs, the messenger pauses.
- The ownership guard keeps the messenger from overwriting other data if
  upstream ever uses the region.

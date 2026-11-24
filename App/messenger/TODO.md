# Messenger: open points

State 2026-11-24: messenger-v0.4.0-beta.1 is on F4HWN v7.0.0 with the
overlay apps, host-tested and in daily use on the radio for three days without
a problem. Beta.1 ran live on two UV-K1 (F4HWN v6.3, 2026-11-07).

## Waiting for reports

No bench plan (see *Decisions*): these stay as built until a user reports a
problem.

- The v7 parts: the texts in the firmware assets, the overlay apps alongside
  the messenger (it pauses while one runs), the power-on resume into an app.
- Masking interrupts while the AFSK bits play (other decoders than ours, also
  while K5Viewer streams over USB).
- The tone level and twist of the transmitter (a Kenwood or Yaesu as the
  receiver).
- The unknowns in [DESIGN.md](DESIGN.md), *Not measured on a radio*.

## On every upstream merge

- A merge, not a rebase: `messenger` has published release tags.
- Flash region: in v7.0.0 the 64 app slots end at 0x181FFF, the asset zones
  start at 0x1CA000 and the app resume journal is in security register 1;
  0x1BA000-0x1C9FFF stays unused. Check again (`APP_SLOT_COUNT`,
  `App/driver/mb_flash.h`, UV Studio's slot count); before upstream uses
  slots 92 and up, move the region with a migration (copy settings and
  mailbox, then erase the old place).
- Side-key actions 25 and 26 must stay free upstream (`ACTION_OPT_LEN` in
  `App/settings.h`: 25 in v7.0.0, 27 with ours).
- `main.c`: `MSG_Init()` before upstream's power-on resume of an app;
  `app_launch()` keeps `MSG_Suspend()` / `MSG_Resume()`.
- CHIRP: take the release's driver and add MESSAGES and APRS ON/OFF again
  ([chirp/README.md](../../chirp/README.md)).
- The size: the build prints "Firmware image N B" (limit 120 832 B); the
  asset pack has a 16 KiB zone.

## Size

On v7 about 2.1 KB of flash are left (image 118 716 B of 120 832 B, multiboot
stub included, Arm GNU Toolchain 13.3.rel1 with LTO as in the CI); the
asset pack uses 11 403 B of its 16 KiB. Keep a reserve for fixes. Next
candidates if needed: the remaining texts of `messenger.c` and `t9.c`
(~0.3 KB, but some are protocol data), table-driven setup items in `msg_ui.c`.

## Interop

- Send reply-acks (`{MM}AA`) to stations that use them: one frame less.

## Ideas (size first)

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

- No bench plan (the owner, 2026-11-19): a test plan is far too much for a
  hobby firmware. The betas are tested by their users; on the radio it is
  enough that it sends and receives messages ([BENCH.md](BENCH.md)). The CI
  runs the host tests before every build.
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

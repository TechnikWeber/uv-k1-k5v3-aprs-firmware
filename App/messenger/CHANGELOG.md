# Messenger changelog

Each `## <tag>` section is the text of that GitHub release (the CI takes it
when the tag is pushed).

## Unreleased

## messenger-v0.3.0-beta.3

Third beta for the Quansheng UV-K1 and UV-K5 V3 (PY32F071): send now and
stop in a message, host-tested; not yet run on a radio (bench test 19 in
App/messenger/BENCH.md).

**New**

- Own message still being sent (`SENDING 1/5`): **MENU** sends the next try
  at once (the counter goes on, `2/5`), **F** stops sending (the message then
  shows `!`, not confirmed; MENU sends it again later). F also stops a
  broadcast.

Flash is nearly full (~90 B left). Back up the calibration with UV Studio
before flashing. Not for the UV-K5 V1/V2. User guide:
https://github.com/TechnikWeber/uv-k1-k5v3-aprs-firmware/blob/messenger-v0.3.0-beta.3/App/messenger/README.md

## messenger-v0.3.0-beta.2

Second beta for the Quansheng UV-K1 and UV-K5 V3 (PY32F071): two additions
on top of beta.1, host-tested; not yet run on a radio (bench test 18 in
App/messenger/BENCH.md).

**New**

- Setup item **Beacon now**: MENU sends a beacon at the next clear channel,
  also with *Beacon* OFF.
- **Monitor** (key `3` in the mailbox): the last 8 frames heard, unfiltered
  (positions, weather, telemetry, messages between other stations), as
  `CALL:info` in small print. RAM only, empty after power on.
- The station list (key `2`) already lists every station heard, also those
  that only send beacons, weather or telemetry; the guides say so now.

Flash is nearly full (~0.1 KB left). Back up the calibration with UV Studio
before flashing. Not for the UV-K5 V1/V2. User guide:
https://github.com/TechnikWeber/uv-k1-k5v3-aprs-firmware/blob/messenger-v0.3.0-beta.2/App/messenger/README.md

## messenger-v0.3.0-beta.1

First beta for the Quansheng UV-K1 and UV-K5 V3 (PY32F071). Tested live on
2026-11-07: two UV-K1 exchanged messages and acks on the air, with dual watch
and *APRS hold* 100 ms, and everything tested so far works. The remaining
bench tests are in App/messenger/BENCH.md.

**New**

- New side-key action **APRS ON/OFF** (F1/F2 short or long, M long): switches
  the messenger on and off; the CHIRP driver knows it.
- *APRS hold* 100 ms is the default and no longer marked experimental: on the
  bench it lost no message between two radios. Existing settings stay.
- Status bar: `APRS` stays during the short looks at the voice VFO, so it no
  longer flickers between `APRS` and `DW` with a short *APRS hold*; the dual
  watch mark shows while the receiver stays on the voice VFO.

Back up the calibration with UV Studio before flashing. Not for the UV-K5
V1/V2. The CHIRP driver of this release knows the new side-key action; load
it before uploading. User guide:
https://github.com/TechnikWeber/uv-k1-k5v3-aprs-firmware/blob/messenger-v0.3.0-beta.1/App/messenger/README.md

## messenger-v0.2.0-alpha.2

Third alpha for the Quansheng UV-K1 and UV-K5 V3 (PY32F071), the first one
run on radios: two UV-K1 exchange messages and acks (bench tests 1-3 in
App/messenger/BENCH.md).

**Fixed**

- Receiver noise made the decoder report a busy channel every few hundred ms,
  so messages and acks waited for minutes (`SENDING 0/5`). The channel now
  counts as busy after 4 back-to-back flags or 4 bytes that look like a
  callsign.

**Changed**

- *APRS hold* is now the time on APRS before each 0.1 s look at the voice
  VFO: 100 ms (experimental), 1 s, 2 s (default), 5 s, 10 s, or ALWAYS (APRS
  only). 30 s and 60 s are gone.
- New *Voice hold* (4, 10, 20, 60 s; default 10 s): the receiver stays on the
  voice VFO after you talk or listen there, before it goes back to APRS.
- The logo screen saver is removed to make room (1 KB, kept for fixes after
  the bench tests); the boot logo stays.

Back up the calibration with UV Studio before flashing. Not for the UV-K5
V1/V2. User guide:
https://github.com/TechnikWeber/uv-k1-k5v3-aprs-firmware/blob/messenger-v0.2.0-alpha.2/App/messenger/README.md

## messenger-v0.2.0-alpha.1

Second alpha for the Quansheng UV-K1 and UV-K5 V3 (PY32F071). Still not run on
a radio: the audio capture, the transmitter, dual watch and the FM radio audio
wait for the bench tests.

**New**

- Position beacon from a Maidenhead locator (4, 6 or 8 characters) or a fixed
  position, with a symbol; the status text is the comment. Without a position
  the status beacon stays.
- Lobby: messages to CQ, QST and ALL (the group codes of Yaesu and Kenwood
  radios) are received (on by default, never acknowledged) and sent (twice).
- IS net: ANSRVR groups on APRS-IS, e.g. HOTG for #APRSThursday. Join from the
  setup; in the recipient field F offers ANSRVR with "CQ HOTG " in front.
- Station list (key 2 in the mailbox): the last 8 stations heard; MENU writes
  to the selected one.
- APRS hold: with dual watch on, the receiver stays on the APRS VFO (default),
  or looks at the other VFO every 10, 30 or 60 s, never during or right after
  a burst.
- Status bar: "APRS" in place of the dual watch mark while the receiver
  listens on the APRS VFO.
- A CHIRP driver for this edition: the F4HWN v6.3.0 driver plus the side-key
  action MESSAGES.

**Fixed**

- A sender that retries faster than every 20 s gets acks again after one got
  lost.
- A third-party header that does not parse no longer leaves part of a
  callsign behind.

**Changed**

- Settings and mailbox moved to 0x1BA000-0x1C9FFF in the external flash: free
  in F4HWN v6 and v7 (the old place collides with v7's firmware assets). They
  start empty after the update: enter the callsign again. The messenger never
  overwrites another program's data there ("FLASH IN USE"; Clear mailbox takes
  the region over).
- The QR codes of the system information pages are removed to make room.

**Compared to F4HWN Fusion v6.3** the spectrum analyser, the RF Log and the QR
codes are removed (flash 99.1 % of 118 KiB); the overlay apps are not included
(they are in F4HWN Labs, which can sit in a Multiboot slot).

Back up the calibration with UV Studio before flashing. Not for the UV-K5
V1/V2. User guide:
https://github.com/TechnikWeber/uv-k1-k5v3-aprs-firmware/blob/messenger-v0.2.0-alpha.1/App/messenger/README.md

## messenger-v0.1.0-alpha.1

First alpha: background APRS messaging with a mailbox in the external flash,
acknowledgements and retries, group bulletins (channels) and public bulletins,
multi-tap text entry, optional encryption off the amateur bands, a status
beacon.

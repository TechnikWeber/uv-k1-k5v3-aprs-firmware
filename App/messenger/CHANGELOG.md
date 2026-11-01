# Messenger changelog

Each `## <tag>` section is the text of that GitHub release (the CI takes it
when the tag is pushed).

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
codes are removed (flash 98.9 % of 118 KiB); the overlay apps are not included
(they are in F4HWN Labs, which can sit in a Multiboot slot).

Back up the calibration with UV Studio before flashing. Not for the UV-K5
V1/V2. User guide:
https://github.com/TechnikWeber/uv-k1-k5v3-aprs-firmware/blob/messenger-v0.2.0-alpha.1/App/messenger/README.md

## messenger-v0.1.0-alpha.1

First alpha: background APRS messaging with a mailbox in the external flash,
acknowledgements and retries, group bulletins (channels) and public bulletins,
multi-tap text entry, optional encryption off the amateur bands, a status
beacon.

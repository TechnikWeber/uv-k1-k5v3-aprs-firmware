# APRS messenger: design

Status (2026-11-01): implemented, host-tested, not yet run on a radio.
Target: UV-K1 and UV-K5 V3 (PY32F071 + BK4829), preset `Messenger` = `Fusion`
(F4HWN v6.3) plus `ENABLE_MESSENGER`, minus `ENABLE_SPECTRUM`, the RF log, the
QR codes and the logo screen saver. User guide: [README.md](README.md). Open points: [TODO.md](TODO.md).

## Goal

The radio sits in a pocket and works as usual (voice channel, FM broadcast
radio). In the background it decodes APRS on 144.800 MHz. A message for the own
callsign is acknowledged automatically, stored in a mailbox in the external
flash, announced with a beep and an envelope icon. The user reads it later and
answers with multi-tap. Outgoing messages are retried until acknowledged.

Standard APRS (APRS 1.0.1 messages, `{id` acks, reply-acks, third-party frames
from iGates) keeps it compatible with every APRS station, APRSdroid, aprs.fi and
the digipeater / iGate / APRS-IS network. An own protocol would have none of
that infrastructure, so it could only reach as far as one radio hears another.

## Why resident firmware, not an overlay app

Overlay apps are modal: while one runs, the normal radio is suspended, and the
app is discarded when it exits (`App/apps/app_api.h`). Background reception has
to live in the resident firmware. The overlay apps `aprsrx`, `aprstx` and
`aprsedit` by Armel F4HWN were the reference for the demodulator, the AFSK
keying and the PA4 audio path; the derived code keeps their Apache 2.0 notice
and is credited in `NOTICE`.

Nothing is taken from the stock firmware dumps or the decompiled stock code in
`armel/k1-teardown/openocd/`: that is Quansheng's code, not open source.

## Files

| File | Role | Host test |
|---|---|---|
| `aprs_ax25.c` | AX.25 UI frames, addresses, FCS | `test_proto` |
| `aprs_msg.c` | APRS messages, acks, rejects, reply-acks, third-party, umlauts | `test_proto` |
| `aprs_demod.c` | Bell 202 demodulator + HDLC, block-wise (port of aprsrx) | `test_demod` (simulated audio, loopback) |
| `aprs_afsk.c` | TX bit stream: flags, bit stuffing | `test_demod` |
| `mailbox.c` | message log in the external flash | `test_mailbox` (simulated NOR) |
| `messenger.c` | settings, acks, outbox and retries, beacon, TX scheduler | `test_messenger` |
| `t9.c` | multi-tap text entry | `test_t9` |
| `msg_ui.c` | the messages screens | `ui_sim` (screens as images) |
| `msg_hw.c` | TIM3/ADC/DMA capture, AFSK transmitter | radio only |
| `msg_port.c` | glue: time, flash, carrier sense, beep, hooks | radio only |
| `msg_fw.h` | the only header the F4HWN code includes | |

Hooks in the F4HWN code, all under `#ifdef ENABLE_MESSENGER`: `main.c` (init),
`scheduler.c` (10 ms clock), `app/app.c` (10 ms task, key table, silent RX,
power save), `board.c` (battery ADC), `app/action.c` + `settings.h` +
`ui/menu.c` (actions `MESSAGES` = 25, `APRS ON/OFF` = 26), `ui/ui.c` + `ui/ui.h` (`DISPLAY_MESSENGER`),
`ui/status.c` (envelope, `APRS` mark). The CHIRP driver for the actions is in
`chirp/`.

## Receive path

TIM3 (free; TIM6 is voice, TIM7 backlight) triggers ADC1 on PA4 at 9.6 kHz.
DMA1 channel 1 (free) writes a circular buffer of 256 samples; its half and
full interrupts (priority 3, the lowest) hand 128 samples (13.3 ms) to the
demodulator. The main loop, flash writes and the display may stall for up to
13 ms without losing samples; the interrupt counts it when it was late (`OV`).

PA4 is held at mid-scale by the DAC with its output buffer off, as the EPIRB
406 lab found necessary. The battery measurement borrows the ADC: the capture
stops the timer, switches channel and DMA off for one conversion, and resumes.

The demodulator is the aprsrx one (DC removal, band-pass, sliding 1200/2200 Hz
correlators, per-tone AGC, 3 slicers with DPLL, NRZI, HDLC, FCS, UI check). On
the same simulated audio it decides frame by frame like the Python reference
model (`apps/aprsrx/test/model_rx.py`). Frames are limited to 200 bytes (a
message frame stays below ~170); a one-frame slot hands them to the main loop.

The capture runs while the messenger is on and a VFO is tuned to the APRS
frequency. Two firmware behaviours needed handling:

- **The BK4829 AF output is muted after every key beep** (and after other
  tones) until the next reception. The messenger switches it back to FM every
  100 ms while the radio idles on the APRS frequency; with the FM radio
  playing, only while the squelch is open, so receiver noise stays unheard.
- **An APRS burst opens the squelch**, which normally starts a reception:
  speaker on, FM radio interrupted. `MSG_IsSilentRx()` keeps the radio in the
  foreground instead (`CheckForIncoming`); only dual watch is held on the APRS
  VFO until the burst ends. With *APRS audio* on, nothing changes.

Power save is blocked while a VFO is on the APRS frequency: a sleeping BK4829
hears nothing, and many stations send only 200-300 ms of preamble.

**APRS hold.** One receiver serves both VFOs. Dual watch toggles every
100 ms; the capture restarts (and the demodulator resets) at each return to
the APRS VFO. An APRS packet starts with a preamble of 0.2-0.5 s, longer than
the 100 ms dual watch spends on the other VFO, so a packet always meets a
visit to the APRS VFO during its preamble; holding the receiver there as soon
as the squelch or the decoder hears it keeps the rest of the packet. The
messenger holds dual watch the way the firmware does after a reception
(`gDualWatchCountdown_10ms`, `gScheduleDualWatch`) for 100 ms (default),
1 s, 2 s, 5 s or 10 s per visit; the firmware's 100 ms countdown
follows, then one look at the other VFO. Preambles shorter than about 0.2 s
that start during that look may be missed. ALWAYS holds for good. *Voice hold* (default 10 s) holds dual watch on the
other VFO after a voice TX or RX there (the firmware alone gives 4.2 s and
1 s). On arrival it switches the BK4829 AF output on at once
(not only every 100 ms), so the decoder hears the whole visit. The user's
dual watch setting is never written. It never leaves during a burst or within 1.5 s
after one (`gBusyUntil`: squelch or decoder activity only; the 1.5 s of
listening after each capture start, `gCaptureAt`, only gate the
transmitter), and independently of
the setting it holds for a 6 s listening window before an automatic
transmission (CSMA), so an ack for a message that just came is sent first.

**Status bar.** While the messenger is on and the receiver is on the APRS VFO,
`ui/status.c` draws `APRS` (3x5 font, 16 px) in the 17 px slot of the dual
watch mark (DW / hold / MO); also during dual watch's idle looks at the other
VFO (the APRS VFO can receive, no signal, no *Voice hold* running), so it does
not flicker with a short *APRS hold*. `MSG_TimeSlice10ms` requests a status
redraw when that state changes.

## Transmit path

Like the APRS TX app: `RADIO_SetTxParameters()` on the APRS VFO (gCurrentVfo and
gRxVfo point to it for the call, so frequency, power, bandwidth and compander
are that VFO's; the compander is then forced off and CTCSS/DCS cleared), the
BK4829 tone generator on at level 66, REG_71 switched between 1200 and 2200 Hz
at each NRZI transition. Bit timing: TIM3 at exactly 1200 Hz, polled (a bit
that never comes aborts the frame instead of hanging). 40 flags of preamble
(267 ms), 3 tail flags; a 100-byte frame takes ~1 s, during which the main loop
waits. Then `RADIO_SetupRegisters(true)` restores reception and the FM radio
audio, and the capture restarts.

TX is refused like PTT is: TX lock outside the allowed bands, battery too low
or too high, non-FM modulation, serial configuration in progress.

## Messenger logic

- **Incoming message** for the own call (SSID must match): stored, beep, ack
  queued if it carries a number. A copy within 20 s of the first one (another
  digipeater) is ignored; a repeat later than that (the sender's retry, our ack
  got lost) is acknowledged again but not stored again (15 min window, 16
  entries), and starts a new 20 s window for its own copies. The window is not
  moved by copies, so a sender retrying every 15 s still gets every second
  retry acknowledged.
- **Acks / rejects** mark the outbox entry; a late ack after the last retry
  still marks the record. Acks from another SSID of the same call count.
- **Outbox:** up to 4 messages waiting for an ack. 5 transmissions, 30, 60, 120,
  240 s apart, then 120 s for the last ack, then failed. Without an APRS VFO a
  try is not used up (retried every 30 s). Message numbers 1-999, stored with
  the settings.
- **Broadcasts (channels):** APRS group bulletins `BLNnGROUP` (n = 0-9,
  rotating, so APRS clients that keep one line per number show them all).
  Received when GROUP is the own channel, or without a group when public
  bulletins are on; duplicates within 6 h are dropped (bulletins are often
  repeated for hours). Sent twice, 60 s apart; no ack exists for bulletins.
  Stored as their own record types, the group in the id field.
- **Notifications:** private messages beep with Beep ALL or PRIVATE, channel
  broadcasts with ALL, public bulletins never (envelope only).
- **Lobby:** messages to `CQ`, `QST`, `ALL` (the group codes of Yaesu and
  Kenwood radios) are kept as their own record types (id = the addressee),
  never acknowledged, deduplicated like messages (15 min). Sending works like a
  bulletin: no number, twice, 60 s apart. On by default (setup *Lobby CQ*).
- **IS net:** an ANSRVR group on APRS-IS. "J GROUP" to ANSRVR joins, a text
  to ANSRVR starting "CQ GROUP " posts; nothing else is special, ANSRVR's
  messages are ordinary private messages.
- **Station list:** the last 8 originators heard (any frame; for third-party
  frames the station inside), newest first, in RAM.
- **Monitor:** a ring of the last 8 frames heard (any UI frame, before any
  filter), kept as `SRC:info` already cut into two rows of 32 tiny characters
  (66 bytes each, 528 B RAM); unprintable bytes become `.`. Mailbox key `3`.
- **Beacon:** 20 s after switching on, then every 10/20/30/60 min (default
  30): a position `=DDMM.mmN/DDDMM.mmE` + symbol + the status text as comment
  (`=`: this station takes messages) when a position is set, from a Maidenhead
  locator (centre of the square) or a fixed position; otherwise a status frame
  `>text`. *Beacon now* sends one at the next clear channel, also with Beacon
  OFF; the interval restarts from it.
- **Scheduler:** the channel must be clear (APRS VFO, squelch closed or squelch
  0, no preamble in the decoder, 1.5 s since the last activity, no RX/TX/PTT/
  scan), then a random 0.7-3 s wait. At least 3 s between own frames; a token
  bucket allows 6 at once, then one per 30 s. Order: acks, messages, beacon.
- **Destination** `APZK5M` (APZ = experimental) until the requested tocall
  `APDLPW` is assigned (github.com/aprsorg/aprs-deviceid issue 402); path
  `WIDE1-1,WIDE2-1` by default.

## Encryption

Optional, for bands where encryption is allowed; refused on 144-146 and
430-440 MHz. Encrypt-then-MAC with standard primitives that are small on a
Cortex-M0 (`msg_crypto.c`, 1.9 KB): ChaCha20 (RFC 8439) and SipHash-2-4, both
checked against their published test vectors in `test_crypto`.

- **Key:** 32 bytes from the passphrase: the passphrase as a ChaCha20 key, then
  4096 times key = first half of the next block (a guess costs 4096 blocks).
  Stored in the settings record (now 128 bytes); the passphrase is not.
- **MAC key:** first 16 bytes of a ChaCha20 block under a fixed domain nonce.
- **Nonce:** 4 random bytes (sent) | SipHash(macKey, sender) (8 bytes), so two
  stations with the same key never share a nonce; a fresh one per transmission.
- **Tag:** SipHash-2-4 over sender, addressee, nonce and ciphertext, 32 bits.
- **Wire:** `@@` + base64 (no padding) of nonce | ciphertext | tag. 40 plain
  characters give 66 characters, under the 67 APRS allows. The `{id` and acks
  stay in the clear.
- **Mailbox:** plain text with the flag `MBX_F_ENCRYPTED`; encryption happens
  at each transmission, and a record that can no longer be encrypted is marked
  failed instead of being sent in the clear.

## Flash

External flash (map in `App/driver/mb_flash.h`): **0x1BA000-0x1C9FFF, 64 KiB**.
Sector 0 holds the settings as a log of 128-byte records (an erase every 32
saves); sectors 1-15 the mailbox, 480 records of 128 bytes. A record is
written into erased flash with its commit byte last; state changes (read,
delivered, failed, deleted) only clear bits. When the log wraps, the oldest
sector is erased (32 messages).

Why there: the factory images of both radios (UV Studio's factory reset
files) are blank from 0x197000 up; F4HWN v6 and v7 store 64 overlay apps at
0x102000-0x181FFF; v7 puts firmware asset zones for its 5 multiboot slots at
0x1CA000-0x1DDFFF (the former messenger region 0x1CE000 collided with slots
1-4) and keeps 0x182000-0x1C9FFF as a reserve for app slots 64-99. This
region is the top of that reserve (would-be slots 92-99), static asserts in
`mailbox.c` keep it between the two. Every other part of the 2 MiB is
assigned in the F4HWN map as well. F4HWN confirmed (2026-11-01) that this use
is unofficial, without any guarantee: F4HWN and its tools may use or erase the
range, and forks follow the map and migrate on their own.

**Ownership guard.** At boot every sector must hold only 128-byte units that
start with 0xFF (erased or cut by a power loss) or with the record marker
(0xA5 mailbox, 0xC7 settings). Otherwise the messenger writes, erases and
flags nothing in that part (settings or mailbox) and the mailbox shows *FLASH
IN USE*; *Clear mailbox* (`USED` in the setup) is the user's decision to take
the region over, after a second warning that the other data goes too. So
the messenger never destroys another program's data (a future app slot, a v7
asset pack), and losing its own data to one is detected by the CRCs.
Power-loss cases, a region full of garbage and foreign sectors are in
`test_mailbox` and `test_messenger`.

## Budget

| | Fusion v6.3 | Messenger |
|---|---|---|
| Flash | 106 388 B (88 %) | 119 564 B (98.9 %) |
| RAM | 12 160 B (74 %) | 14 432 B (88 %) |

(GCC 14.3 with *Beacon now* and the monitor; the CI's arm-none-eabi-gcc 13.3,
which the releases use, builds ~260 B larger. The multiboot RAM stub needs
another 888 B of flash, which leaves ~0.1 KB in the CI build. Beta.1 before
them: 119 196 B in the CI build, 748 B left.) The messenger is ~27.5 KB of code (screens 9.6 KB, logic
6 KB, protocol 2.9 KB, crypto 1.9 KB, demodulator 1.2 KB); the spectrum
analyser (~9 KB), the RF log (~5 KB), the QR codes (0.5 KB) and the logo
screen saver (1 KB) were dropped to make room. Overlay apps (+3.6 KB for the platform) do not fit on v6.3; see
[TODO.md](TODO.md) for F4HWN v7. RAM: 3 slicers with
200-byte buffers, 512-byte DMA buffer, one frame slot. Worst stack depth of the
messenger ~700 bytes (`-fstack-usage`).

## Open points for the bench

See the bench plan in [BENCH.md](BENCH.md). The unknowns:
FM radio audio on PA4, decoder load with USB and display active, the battery
reading during capture, current draw, the level of the AF output at volume
settings, and whether the BK4829 mutes its AF output below the squelch.

# APRS Messenger: quick start

*Deutsch: [QUICKSTART.de.md](QUICKSTART.de.md). Full guide: [README.md](README.md).*

Alpha firmware: the radio parts are not tested on a radio yet. Transmitting
needs an amateur radio licence; the radio sends acks and beacons by itself,
so keep an antenna connected.

## 1. Flash

1. With the official F4HWN Fusion on the radio, back up the calibration in
   [UV Studio](https://armel.github.io/uvstudio/#dump-calib) (once per radio).
2. Flash the Messenger edition from the
   [web flasher](https://technikweber.github.io/uv-k1-k5v3-aprs-firmware/).
   UV-K1 and UV-K5 V3 use the same file (not for the UV-K5 V1/V2).
3. UV-K1: the arrow keys are LEFT / RIGHT; everywhere below, UP = LEFT and
   DOWN = RIGHT.

## 2. The APRS channel on VFO B

The messenger listens and transmits on a VFO that you tune to the APRS
frequency; it never tunes a VFO itself. Set VFO B like this (or store it once
as memory channel `APRS` with **ChSave** / **ChName**, also possible with the
CHIRP driver of this fork):

| Menu | Value | Why |
|---|---|---|
| frequency | **144.800** (Europe/Africa; N. America 144.390, Australia 145.175) | must equal *Freq* in the messenger setup |
| **Mode** | FM | AM/USB cannot carry APRS |
| **W/N** | WIDE | APRS uses wide FM |
| **TxODir** | OFF | simplex: with an offset the messenger refuses to send |
| **RxCTCS / RxDCS** | OFF | a tone squelch would mute the packets |
| **TxCTCS / TxDCS** | OFF | no tone under the data |
| **Compnd** | OFF | the compander distorts the tones |
| **Power** | MID or HIGH | to reach the next digipeater (LOW for tests) |
| **Sql** | 1 or 2 | the messenger waits for a free channel |

**F + 2** switches between VFO A and B, **F + 3** between frequency and memory
mode; a frequency is typed on the keypad.

## 3. Voice and APRS together

- VFO A: your voice channel, selected as the main VFO (**F + 2**).
- **RxMode** = `MAIN TX / DUAL RX`: PTT always transmits on VFO A. Not
  `DUAL RX / RESPOND` (PTT could go out on 144.800) and not `CROSS BAND`.
- Only APRS: **RxMode** = `MAIN ONLY` with VFO B as the main VFO.
- The radio has one receiver. *APRS hold* in the messenger setup decides how
  it is shared (see below).

## 4. The messenger

1. Menu → **F1Shrt** / **F1Long** / **F2Shrt** / **F2Long** / **M Long** →
   **MESSAGES**: that key opens the messenger.
2. First start: type your callsign without SSID, MENU. Then *Messenger* → MENU
   = ON.
3. Optional: *Channel*, *Locator*, *IS net* (below).

### Settings (F in the mailbox)

| Item | What it does | Recommended |
|---|---|---|
| Messenger | background reception on/off | ON |
| Call / SSID | your station; SSID 7 = handheld, 9 = mobile, 0 = none | 7 |
| Channel | your group (1-5 letters/digits); broadcasts to it reach every radio with the same channel (APRS group bulletins) | as agreed |
| Public BLN | also keep public bulletins (club news, nets) | OFF |
| Lobby CQ | keep messages to CQ, QST, ALL (Yaesu/Kenwood group codes) | ON |
| IS net | ANSRVR group on the Internet (e.g. `HOTG`); MENU edits, **F joins** | empty |
| Beep | ALL (private, channel, lobby), PRIVATE, OFF | ALL |
| Freq | the frequency the messenger works on; MENU types it in, F steps through presets | = VFO B |
| APRS hold | ALWAYS: the receiver stays on VFO B. 10/30/60 s: then a short look at VFO A | ALWAYS |
| Encrypt / Key | encryption, only off the amateur bands (refused on 144-146 / 430-440 MHz) | OFF |
| Path | DIRECT, WIDE1, WIDE1+2 (digipeater hops) | WIDE1+2 |
| Beacon | OFF, 10, 20, 30, 60 min; iGates deliver Internet messages only to stations they heard | 30 min |
| Status | beacon text (also the comment of the position) | free text |
| Position | OFF, LOCATOR, FIXED: the position in the beacon | LOCATOR |
| Locator / Lat / Lon | e.g. `JO62QM`; or `485090` = 48°50.90' (F switches N/S, E/W) | your locator |
| Symbol | map symbol: PERSON, HOUSE, CAR, BIKE, BOAT, PORTABLE | PERSON |
| APRS audio | ON: hear the packets on the speaker | OFF |
| Clear mailbox | erase all messages (asks first) | |

The bottom line shows the bench figures: frames heard (RX) and sent (TX),
decoder load (DSP), audio level (LV).

## 5. Keys

| Screen | Keys |
|---|---|
| Mailbox | UP/DOWN select · MENU read · `*` new · `1` chat with this station/channel · `2` stations heard · F setup · EXIT back |
| Reading | UP/DOWN scroll · MENU reply (own message without ack: send again) · F private reply to a group message · `0` delete |
| Writing | `0`-`9` multi-tap (`2` = a b c 2) · `*` Abc/abc/ABC/123 · UP/DOWN cursor · EXIT delete, held: cancel · MENU next / send |
| Recipient | a callsign, or **F**: your channel → `#ALL` (public bulletin) → `CQ` (lobby) → `ANSRVR` (IS net, text starts with "CQ group ") |

## 6. What the display tells you

- Status bar **`APRS`**: the messenger listens on VFO B right now. Envelope:
  unread messages.
- Marks: `*` new · `>` sending (retries after 30, 60, 120, 240 s) · `+`
  delivered · `!` no ack or rejected (MENU sends again) · `#` group or lobby.
- Warnings at the bottom of the mailbox:

| Warning | Fix |
|---|---|
| `NO VFO ON 144.800` | tune a VFO to *Freq* |
| `SELECT APRS VFO OR DUAL WATCH` | dual watch is off (or the FM radio is on) and VFO B is not the main VFO |
| `TX BLOCKED: CHECK FM/SIMPLEX/LOCK` | Mode, TxODir or TX lock of VFO B |
| `TX BLOCKED: BATTERY` | charge |
| `FLASH IN USE: CLEAR MAILBOX` | another program's data in the messenger's flash area; *Clear mailbox* takes it over |

## 7. Good to know

- **One receiver:** while you talk or listen on VFO A, or scan, APRS is deaf.
  Missed messages usually arrive with the sender's next retry.
- **APRS hold ALWAYS** keeps the receiver on VFO B: you still transmit on
  VFO A but hear it only for about 4 s after your own transmission. Choose
  10/30/60 s to hear VFO A as well, with gaps in APRS. The messenger never
  leaves during a packet or while an ack waits to go out.
- **FM radio:** the firmware pauses dual watch; select VFO B as the main VFO
  first, or APRS stays deaf.
- **Battery:** power save stays off while a VFO is on the APRS frequency.
- **Automatic transmissions:** acks 0.7-3 s after the channel is free, beacons,
  retries; at most 6 frames at once, then one per 30 s.
- **Limits:** 67 characters per message (40 encrypted); umlauts become
  ae/oe/ue/ss; up to 480 messages, the oldest 32 go when full; no time stamps
  (the radio has no clock).
- **Messages from the Internet** (aprs.fi, APRSdroid via APRS-IS) need an
  iGate that heard you recently: keep the beacon on.
- **CHIRP:** use the driver of this fork, `chirp/f4hwn.messenger.chirp.py`;
  the F4HWN driver resets the MESSAGES key.
- **Not in this edition:** spectrum analyser, RF Log, QR codes, logo screen
  saver, overlay apps (F4HWN Labs can sit in a Multiboot slot).

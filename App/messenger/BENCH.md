# Bench test plan

Everything that runs without a radio has host tests; the parts that drive
the hardware (audio capture, transmitter, dual watch, beeps, flash on the
radio) have never run on a radio. This plan checks them in an order where
each step builds on the one before. Fill in the results table at the end and
bring it to the next session.

## What you need

- Two radios with this firmware, **A** and **B** (two UV-K1 are fine).
- For the network tests: a third APRS station (Kenwood, Yaesu, APRSdroid with
  a TNC) or an iGate in range, and aprs.fi.
- Optional: a current meter, a PC receiver with Direwolf, UV Studio with
  K5Viewer for screenshots.

## Before you start

1. Back up the calibration of both radios with UV Studio.
2. Flash the latest build from the web flasher and note the build number
   shown on that page.
3. Both radios: different SSIDs (e.g. `DL4PW-7` on A, `DL4PW-8` on B),
   *Messenger* ON, *Beacon* OFF, *Path* DIRECT, *Beep* ALL.
4. First tests on a free simplex frequency, not 144.800: power LOW 1 or LOW 2,
   antennas on, radios a few metres apart. Set *Freq* to that frequency on
   both radios and tune VFO B to it (FM, wide, no offset, no tones,
   compander off). RxMode `MAIN ONLY` with VFO B as the main VFO until test 6.
5. What to record when something looks odd: the bottom line of the setup
   (*RX*, *TX*, *DSP*, *LV*, *OV*), as a photo or a K5Viewer screenshot.

## The tests

| # | Test | How | Expected |
|---|---|---|---|
| 1 | Capture runs | open the setup on A, no signal | *LV* above 0 (noise), *DSP* a few %, no *OV* |
| 2 | Audio level | send a message from B to A, watch *LV* on A | *LV* several hundred during the burst; 0 = the capture does not run |
| 3 | Receive and ack | message from B to A | A: two tones, envelope, message in the mailbox; B: `+` within a few seconds |
| 4 | Reliability | 20 messages from B to A, one after the other | at least 19 delivered at the first try (`+` without a retry) |
| 5 | Key beeps | press keys on A with key beep on, then send from B | still decoded (the messenger switches the audio back on) |
| 6 | Dual watch | B: VFO A on a voice channel as main, VFO B on the test frequency, RxMode `MAIN TX / DUAL RX`. For *APRS hold* 100 ms, 2 s and ALWAYS: 20 messages from A, and a voice call to B's VFO A | count first-try deliveries per setting; voice heard within about the hold time (never with ALWAYS); status bar `APRS` also during the looks, `DW` while the receiver stays on VFO A |
| 7 | Voice hold | B transmits on its VFO A, then waits | B stays on VFO A for the *Voice hold* (10 s), then `APRS` again |
| 8 | FM radio | B: VFO B as main, FM radio on, message from A | message arrives; note how loud the burst is over the music; music goes on |
| 9 | Battery reading | compare the battery voltage with the messenger on and off | plausible, no jumps |
| 10 | Current draw | standby current, messenger on and off | note both (power save is off while a VFO is on the APRS frequency) |
| 11 | Our transmissions | decode A's packets with a third receiver (Kenwood/Yaesu/Direwolf), also while K5Viewer streams over USB | decoded; if not, try with K5Viewer off (interrupts during the bits) |
| 12 | Channel, lobby, bulletins | same *Channel* on A and B: broadcast from A; a CQ; `#ALL` with *Public BLN* off and on | channel message with `#` and a beep, no ack; CQ kept; bulletin only with *Public BLN* ON |
| 13 | Flash | power off and on; *Clear mailbox*; calibration dump in UV Studio | messages and settings kept; clearing works; calibration dump unchanged |
| 14 | 144.800 and the map | *Beacon* 10 min, *Locator* set, *Path* WIDE1+2, power MID, on 144.800 | position with symbol and status on aprs.fi |
| 15 | Network | message from aprs.fi or APRSdroid (APRS-IS) to A; A answers | arrives through an iGate; the answer shows as delivered online |
| 16 | IS net | *IS net* `HOTG`, join, post on a Thursday | ack from ANSRVR; posts of the group arrive |
| 17 | Encryption | only where allowed, or into dummy loads: same *Key*, *Freq* off the amateur bands, *Encrypt* ON on both | text decrypted on B; another key on B shows "(encrypted, other key)" |

## Decisions that wait for these results

- *APRS hold*: decided 2026-11-07, 100 ms is the default (no message lost
  between two radios). Test 6 still compares the settings.
- Masking interrupts while the AFSK bits play (test 11).
- The tone level and twist of the transmitter (test 11, with a Kenwood or
  Yaesu as the receiver).
- Anything in [TODO.md](TODO.md) under *Bench*.

## Results

Build: ______  Date: 2026-11-07  Radios: 2x UV-K1 (A = QK1826060015218, B = QK1826060015232)

| # | Result | Numbers / notes |
|---|---|---|
| 1 | pass | LV = 400-800 (noise, no signal), DSP = 12 %, no OV |
| 2 | | LV peak = |
| 3 | partly | first try: A decoded (RX 1) and sent the ack (TX 1), B did not decode it (RX 0), `>` stayed; after a manual resend `-` for a while, then `+`. Sql 1, green LED off. After the busy fix (decoder busy on noise): messages both ways and acks work |
| 4 | | __ / 20 |
| 5 | | |
| 6 | | 100 ms: __/20, voice after __ s · 2 s: __/20, voice after __ s · ALWAYS: __/20 |
| 7 | | |
| 8 | | |
| 9 | | |
| 10 | | on: __ mA, off: __ mA |
| 11 | | |
| 12 | | |
| 13 | | |
| 14 | | |
| 15 | | |
| 16 | | |
| 17 | | |

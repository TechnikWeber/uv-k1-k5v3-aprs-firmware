# Radio check

There is no bench plan: the betas are tested by their users, and problems go
to the [issues](https://github.com/TechnikWeber/uv-k1-k5v3-aprs-firmware/issues).
Everything that runs without a radio has host tests (`test/run.sh`; the CI
runs them before every build). On the radio it is enough that the messenger
sends and receives messages: the check below, when two radios are at hand.

## The check

Two radios with this firmware, **A** and **B** (two UV-K1 are fine), with
different SSIDs (e.g. `DL4PW-7` and `DL4PW-8`), *Messenger* ON and *Path*
DIRECT. A free simplex frequency, not 144.800: *Freq* and VFO B on it on both
radios (FM, wide, no offset, no tones), power LOW, a few metres apart.

| # | Test | How | Expected |
|---|---|---|---|
| 1 | Capture runs | open the setup on A, no signal | *LV* above 0 (noise), no *OV* |
| 2 | Messages both ways | a message from B to A, then the answer from A to B | two tones, the envelope, the message in the mailbox; the sender shows `+` within a few seconds |
| 3 | With dual watch | the same with a voice channel on VFO A as the main VFO, RxMode `MAIN TX / DUAL RX`, *APRS hold* 100 ms | the same |

When something looks odd, note the bottom line of the setup (*RX*, *TX*,
*DSP*, *LV*, *OV*), as a photo or a K5Viewer screenshot, and open an issue.

## Results

| Build | Date | Capture (1) | Both ways (2) | Dual watch (3) |
|---|---|---|---|---|
| v0.3.0-beta.1 (F4HWN v6.3), 2x UV-K1 | 2026-11-07 | pass: *LV* 400-800 on noise, *DSP* 12 % | pass, after the fix for receiver noise that kept the channel busy | pass: no message lost |
| v0.4.0-beta.1 (F4HWN v7.0.0) | | not run: released for the users to test | | |

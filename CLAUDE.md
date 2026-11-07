# Notes for Claude

A fork of the F4HWN firmware for the Quansheng UV-K1 / UV-K5 V3 (PY32F071,
BK4829). Branch `messenger` adds background APRS messaging; `main` follows
upstream (remote `upstream` = armel/uv-k1-k5v3-firmware-custom). The owner,
Philipp DL4PW, writes German in chat. Docs, UI texts and comments are English
only: no translations (the radio UI is English only for lack of flash).

- Code: `App/messenger/`. Pure C core (protocol, mailbox, logic, UI) plus the
  hardware glue `msg_hw.c` / `msg_port.c`. Hooks in upstream files stay short,
  under `#ifdef ENABLE_MESSENGER`, all declared in `msg_fw.h`.
- Docs: `App/messenger/README.md` (user guide), `QUICKSTART.md` (short
  manual with examples), `HOW_IT_WORKS.md` (how messages travel, Mermaid
  diagrams), `BENCH.md` (test plan for the radio, results table), `DESIGN.md`
  (as built), `TODO.md` (open points, the F4HWN v7 plan), `CHANGELOG.md`
  (release notes per tag). Keep them current when settings or keys change.
- State (2026-11-07): alpha.2, bench on two UV-K1 started (A = ...15218,
  B = ...15232, calibration backups in ~/radio-calib); tests 1-3 pass after
  the decoder busy fix. Next: BENCH.md from test 4, fixes; tocall APDLPW requested
  (aprsorg/aprs-deviceid issue 402); move to F4HWN v7 when v7.0.0 is
  released (first item in TODO.md, with the monitor view).
- Host tests: `App/messenger/test/run.sh [OUTDIR]` (gcc, python3). Screens:
  `test/render_lcd.py OUTDIR/screens 4` gives `overview.png` =
  `App/messenger/screens.png`; `render_lcd.py OUTDIR/screens 3 DIR` and
  `render_lcd.py OUTDIR/screens/extra 3 DIR` give the quick-start images
  (`App/messenger/img/`, only the ones QUICKSTART.md uses). Sample texts in
  the simulator are English.
- Firmware: `cmake --preset Messenger && cmake --build --preset Messenger`.
  Flash is 98.7 % full (~0.7 KB left): check the size after every change; the
  multiboot RAM stub needs 888 B more than the FLASH figure the linker prints,
  and the CI's GCC 13.3 (the reference) builds ~280 B larger than GCC 14.3.
- External flash: the messenger owns 0x1BA000-0x1C9FFF only (ownership guard
  in `mailbox.c`); never write elsewhere.
- One `.bin` serves both radios; the K1's arrow keys are LEFT/RIGHT
  (`SetNav`), so navigation follows the F4HWN menus (raw UP/DOWN in lists).

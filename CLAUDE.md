# Notes for Claude

A fork of the F4HWN firmware for the Quansheng UV-K1 / UV-K5 V3 (PY32F071,
BK4829). Branch `messenger` adds background APRS messaging; `main` follows
upstream (remote `upstream` = armel/uv-k1-k5v3-firmware-custom). The owner,
Philipp DL4PW, writes German; the repository docs are English.

- Code: `App/messenger/`. Pure C core (protocol, mailbox, logic, UI) plus the
  hardware glue `msg_hw.c` / `msg_port.c`. Hooks in upstream files stay short,
  under `#ifdef ENABLE_MESSENGER`, all declared in `msg_fw.h`.
- Docs: `App/messenger/README.md` (user guide), `DESIGN.md` (as built),
  `TODO.md` (open points, bench topics, the F4HWN v7 plan). Keep them current.
- Host tests: `App/messenger/test/run.sh [OUTDIR]` (gcc, python3). Screens:
  `test/render_lcd.py OUTDIR/screens`; `App/messenger/screens.png` is its
  `overview.png`.
- Firmware: `cmake --preset Messenger && cmake --build --preset Messenger`.
  Flash is ~99 % full: check the size after every change; the multiboot RAM
  stub needs 888 B more than the FLASH figure the linker prints.
- External flash: the messenger owns 0x1BA000-0x1C9FFF only (ownership guard
  in `mailbox.c`); never write elsewhere.
- One `.bin` serves both radios; the K1's arrow keys are LEFT/RIGHT
  (`SetNav`), so navigation follows the F4HWN menus (raw UP/DOWN in lists).

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
  diagrams), `BENCH.md` (the short radio check and its results), `DESIGN.md`
  (as built), `TODO.md` (open points), `CHANGELOG.md`
  (release notes per tag). Keep them current when settings or keys change.
- State (2026-11-19): `messenger` = messenger-v0.4.0-beta.1: F4HWN v7.0.0
  (upstream main) merged, overlay apps kept, messenger suspended around each
  app, display texts in the firmware assets; the CI passes all host tests,
  not yet run on a radio (released for the users to test). Before, on F4HWN
  v6.3: beta.1 tested live on two UV-K1 (A = ...15218, B = ...15232,
  calibration backups in ~/radio-calib): messages and acks work, APRS hold
  100 ms default, side-key APRS ON/OFF (action 26); beta.2 Beacon now and the
  monitor (mailbox key 3), beta.3 send now (MENU) / stop (F), beta.4 Path
  ARISS, beta.5 tocall APDLPW (aprsorg/aprs-deviceid issue 402). Example
  channel list: chirp/examples/ (CSV from the image). Channel image for both
  radios: ~/Schreibtisch/K1_Kanaele_*.img.
- No bench plan (the owner, 2026-11-19): betas go out for the users to test;
  on the radio it is enough that it sends and receives messages (BENCH.md,
  three steps). Don't write test plans or hold a release for radio tests.
  Release: rename `## Unreleased` in CHANGELOG.md to `## <tag>`, then push
  `messenger` and the tag `messenger-vX.Y.Z-beta.N` (the CI publishes the
  pre-release; a push to `messenger` updates the web flasher).
- Host tests: `App/messenger/test/run.sh [OUTDIR]` (gcc, python3). Screens:
  `test/render_lcd.py OUTDIR/screens 4` gives `overview.png` =
  `App/messenger/screens.png`; `render_lcd.py OUTDIR/screens 3 DIR` and
  `render_lcd.py OUTDIR/screens/extra 3 DIR` give the quick-start images
  (`App/messenger/img/`, only the ones QUICKSTART.md uses). Sample texts in
  the simulator are English. On the owner's Windows PC Smart App Control
  blocks the self-built test programs (they compile; never turn SAC off): run
  them in the CI or on Fedora.
- Firmware: `cmake --preset Messenger && cmake --build --preset Messenger`
  (Arm GNU Toolchain 13.3.rel1 = the CI's reference; on Windows it is in
  ~/tools, cmake and ninja come from `pip install --user`). On v7 the build
  prints "Firmware image N B" (multiboot stub included, limit 120 832 B):
  118 716 B on 2026-11-19, 2 116 B left; check it after every change. LTO is
  on: ISR/DMA-shared data needs volatile or compiler barriers.
- Display texts in `msg_ui.c` are firmware assets: `A("...")` / `AT(table[i])`,
  read only through the helpers (`print`, `title`, `tiny`, `popup`,
  `label_value`, `acpy`, `acat`). Texts that become data (recipients,
  prefixes, compared strings) stay plain. The simulator (`MSG_SIM_ASSETS`)
  crashes on a direct read.
- Overlay apps run from the flash driver's sector cache: no external-flash
  write while one runs (`MSG_Suspend()` in `app_launch()`).
- External flash: the messenger owns 0x1BA000-0x1C9FFF only (ownership guard
  in `mailbox.c`); never write elsewhere. On every upstream merge check the
  app slot count and the asset zones (`App/driver/mb_flash.h`).
- One `.bin` serves both radios; the K1's arrow keys are LEFT/RIGHT
  (`SetNav`), so navigation follows the F4HWN menus (raw UP/DOWN in lists).

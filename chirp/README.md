# CHIRP driver for the Messenger edition

`f4hwn.messenger.chirp.py` is the F4HWN v6.3.0 CHIRP driver with one change:
the side-key action **MESSAGES** (number 25), which the F4HWN driver shows as
NONE and resets when it writes the settings. Everything else, including the
memory layout, is the F4HWN driver's: images saved with either driver open
with the other.

In CHIRP: *File → Load Module*, choose this file (newer CHIRP versions show
the entry after *Help → Developer Mode*), then *Radio → Download From Radio*
with the model *Quansheng UV-K1 & UV-K5 V3 (F4HWN)*.

The driver is licensed under the GNU General Public License, version 2 or
later (see its header): Jacek Lipkowski SQ5BPF, EGZUMER, JOC2, Armel F4HWN;
the MESSAGES entry by Philipp Weber DL4PW. On a rebase onto a new F4HWN
release, take that release's driver and add the entry again.

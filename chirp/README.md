# CHIRP driver for the Messenger edition

`f4hwn.messenger.chirp.py` is the F4HWN v7.0.0 CHIRP driver with one change:
the side-key actions **MESSAGES** (number 25) and **APRS ON/OFF** (26), which
the F4HWN driver shows as NONE and resets when it writes the settings.
Everything else, including the memory layout, is the F4HWN driver's: images
saved with either driver open with the other.

In CHIRP: *File → Load Module*, choose this file (newer CHIRP versions show
the entry after *Help → Developer Mode*), then *Radio → Download From Radio*
with the model *Quansheng UV-K1 & UV-K5 V3 (F4HWN)*.

[`examples/`](examples/) has my channel list as a CHIRP CSV, with the ISS
channels and a short note on each channel.

The driver is licensed under the GNU General Public License, version 2 or
later (see its header): Jacek Lipkowski SQ5BPF, EGZUMER, JOC2, Armel F4HWN;
the two entries by Philipp Weber DL4PW. On a merge of a new F4HWN release,
take that release's driver and add the entries again.

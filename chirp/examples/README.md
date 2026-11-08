# Example channel list (DL4PW, UV-K1)

`k1-dl4pw.csv` is the channel list I use on my UV-K1 with the Messenger
firmware, near Balingen (locator JN48). It is a CHIRP CSV export: in CHIRP,
open your radio's image, then *File → Import* this file. The *Comment*
column names the scan list each channel had on the radio. CSV files carry no
scan lists, so set them again in CHIRP after the import.

The radio image itself (`.img`) is not here, on purpose: it holds the
radio's settings and calibration. Never write another radio's image to your
own.

## Channels

| Ch | Name | What | Scan list |
|---|---|---|---|
| 1-2 | ANR2M, ANR70 | FM calling channels, 145.500 and 433.500 | HAM |
| 3 | APRS | 144.800, the APRS frequency for the messenger (2 W) | - |
| 4 | P30 2M | 145.275, a local simplex net | HAM |
| 5-41 | DB0..., DK0TEN, HB9SP | FM repeaters around Balingen, Stuttgart and the Lake Constance area, with their shift and access tone | HAM |
| 42-57 | PMR 01-16 | PMR446, lowest power. Transmitting on PMR446 needs an approved radio, so these are for listening | PMR |
| 58-63 | Freenet 1-6 | Freenet (149 MHz), receive only (TX locked) | PMR |
| 64 | ISS Voice | ISS crew radio: RX 145.800, TX 145.200 (Region 1 uplink). Answer the crew only when they call CQ, never during a school contact | - |
| 65 | ISS APRS | 145.825, the ISS APRS digipeater. Messenger: *Freq* 145.825, *Path* ARISS | - |
| 66 | ISS Rptr | ISS FM repeater: RX 437.800, TX 145.990 with 67.0 Hz tone | - |
| 67-71 | ISS AB -10 ... ISS AN +10 | The same repeater, RX in 5 kHz steps for the Doppler shift (see below) | ISS |
| 72-77 | AIR 121.5, EDDS ..., EDNY ... | Airband AM: emergency, Stuttgart and Friedrichshafen tower and ATIS; receive only (TX locked) | AIR |
| 78-80 | Marine 16, 10, 06 | Marine VHF, receive only (TX locked) | SEA |

## ISS repeater and Doppler

The ISS moves at about 7.7 km/s, so its 70 cm downlink arrives up to 10 kHz
higher while it approaches and up to 10 kHz lower while it leaves. On 2 m the
shift is only about 3.4 kHz, which FM tolerates, so the uplink and the other
ISS channels need one channel each.

| Ch | Name | RX (MHz) | When |
|---|---|---|---|
| 71 | ISS AN +10 | 437.810 | rising (AN = approach) |
| 70 | ISS AN +5 | 437.805 | |
| 69 | ISS MITTE | 437.800 | highest point (same as channel 66) |
| 68 | ISS AB -5 | 437.795 | |
| 67 | ISS AB -10 | 437.790 | setting (AB = departure) |

All five transmit on 145.990 with the 67.0 Hz tone; the radio switches band
when PTT is pressed. Start a pass on channel 71 and step one channel down
(the UV-K1's left/right arrow keys, as set in *SetNav*) when the voice gets
worse. Most of the change comes in the two or three minutes
around the highest point, not evenly over the pass. RxMode MAIN ONLY;
crossband (XB) is not needed, as each channel holds both frequencies.

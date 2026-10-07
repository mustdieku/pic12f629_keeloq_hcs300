# PIC12F629 HCS300-compatible KeeLoq transmitter

This project implements a two-button HCS300-compatible rolling-code encoder for PIC12F629.

## Hardware mapping

PIC12F629 (8-pin):

| Pin | PIC signal | Function |
|---|---|---|
| 2 | GP5 | LED, active high |
| 3 | GP4 | RF data output (logic 0/1 only; external RF modulator follows this signal) |
| 6 | GP1 | Button 1, active high, button code `0x2` |
| 7 | GP0 | Button 2, active high, button code `0x8` |
| 1 | VDD | +5 V nominal |
| 8 | VSS | GND |

The firmware does not generate an RF carrier. GP4 outputs the HCS300 PWM baseband waveform.

## Behaviour

* Button 1 -> button code `0x2`.
* Button 2 -> button code `0x8`.
* Both buttons -> button code `0xA`.
* Debounce is performed in software (10 ms stable sample).
* A new transmission state is created only when the debounced button state changes from no-button to a pressed state, or when another button is added while a button is already held.
* Counter is incremented exactly once for each such press-state transition.
* Releasing a button never increments the counter.
* The same 66-bit code word is repeated while the debounced button state remains unchanged; the counter is not changed between repetitions.
* LED is ON while any button is pressed. The first word has RPT=0; subsequent repeated words have RPT=1, as in HCS300 code-word repetition.

## KeeLoq / HCS300 format

The firmware uses the standard 528-round KeeLoq block cipher with NLF `0x3A5C742E`.
For normal learning, the 64-bit encoder key is derived from the 28-bit serial number and the 64-bit manufacturer code using the standard two-decryption construction used by the referenced Unleashed firmware.

The encrypted 32-bit input is:

`button[4] | OVR[2] | DISC[10] | counter[16]`

with `DISC = serial & 0x3FF` and OVR=0. The fixed 34-bit field contains button status and the 28-bit serial number. Data is emitted LSB-first in HCS300 order.

PWM uses `TE = 400 us`, 23 TE preamble, 10 TE header and a 39 TE guard interval. A logic 0 is encoded as 1 TE high + 2 TE low; a logic 1 as 2 TE high + 1 TE low.

## Configuration

Edit `src/config.h`:

* `MANUFACTURER_CODE`
* `SERIAL_NUMBER`
* `INITIAL_COUNTER`

The manufacturer code and serial number are intentionally compile-time constants.

## EEPROM counter storage

PIC12F629 has 128 bytes of data EEPROM. The firmware uses 16 rotating records of 7 bytes (112 bytes total):

`MAGIC | SEQ | COUNTER | ~COUNTER | CHECK`

The next slot is invalidated first, then its data is written, and the final magic byte is written last. At startup all records are scanned and the newest valid sequence is selected using an 8-bit wrap-aware comparison. This spreads writes over all EEPROM locations instead of repeatedly writing one address.

The firmware verifies the record after writing before allowing RF transmission.

No EEPROM scheme can guarantee preservation of a not-yet-committed increment if supply power collapses during the EEPROM write itself. For a practical transmitter, brown-out protection and sufficient supply hold-up are recommended.

## Build

The project is prepared for Microchip XC8 and MPLAB X. The intended compiler is XC8 for PIC12F629. A local XC8 installation is required to produce the device HEX file.

`make` invokes `xc8-cc` when it is available.

## Source provenance

The KeeLoq primitive follows the implementation in DarkFlippers/unleashed-firmware `lib/subghz/protocols/keeloq_common.c` and its associated header. The project does not copy the Flipper runtime; the cipher is adapted to a freestanding PIC12F629 environment.

Reference: https://github.com/DarkFlippers/unleashed-firmware
Microchip HCS300 datasheet: DS21137F.

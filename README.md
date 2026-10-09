# PIC12F629 HCS300-compatible KeeLoq transmitter

This project implements a two-button HCS300-compatible KeeLoq rolling-code
transmitter for the PIC12F629.

The PIC does not generate an RF carrier. It generates the HCS300 digital
baseband waveform on GP4; an external RF stage is responsible for modulation
and transmission.

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
* Debounce requires 10 ms of stable input.
* A new counter value is consumed whenever the debounced button state
  changes to another non-zero state.
* Releasing a button never increments the counter.
* The same rolling-code counter is reused for all repetitions while the
  button state remains unchanged.
* `RPT=0` is transmitted for the first word after a state transition.
* `RPT=1` is transmitted for subsequent repetitions.
* The LED is continuously ON while any button is pressed.
* The LED is OFF when no button is pressed.

Examples:

```text
00 -> 2   counter++
2  -> 00  no counter change

00 -> 8   counter++
8  -> 00  no counter change

2  -> A   counter++
A  -> 2   counter++

8  -> A   counter++
A  -> 8   counter++

A  -> 00  no counter change
```

Holding a button does not increment the counter:

```text
button 2 pressed
    counter = N
    send(N, 2, RPT=0)
    send(N, 2, RPT=1)
    send(N, 2, RPT=1)
    ...
```

## KeeLoq / HCS300 format

The firmware uses the standard 528-round KeeLoq block cipher with NLF `0x3A5C742E`.
This firmware uses Simple Learning. The 64-bit manufacturer code is used directly as the KeeLoq encryption key. No Normal Learning key derivation is performed.

The encrypted 32-bit input is:

`button[4] | OVR[2] | DISC[10] | counter[16]`

with:

`DISC = serial & 0x3FF`

and:

`OVR = 0`.

The fixed 34-bit field contains the 28-bit serial number followed by:

`S3 | S0 | S1 | S2 | VLOW | RPT`

Data is emitted in HCS300 order.

PWM uses `TE = 400 us`, a 23 TE HIGH 50% duty cycle preamble, a 10 TE LOW header and a guard interval after each word. A logic 0 is encoded as 2 TE HIGH + 1 TE LOW; a logic 1 as 1 TE HIGH + 2 TE LOW.

## Configuration

Edit `src/config.h`:

* `MANUFACTURER_CODE_LO`
* `MANUFACTURER_CODE_HI`
* `SERIAL_NUMBER`
* `INITIAL_COUNTER`

The manufacturer code and serial number are compile-time constants.
`DISC_VALUE` is automatically derived from the ten least significant
serial-number bits.

## EEPROM counter storage

PIC12F629 has 128 bytes of data EEPROM. The firmware uses 16 rotating records of 5 bytes (80 bytes total):

`MAGIC | SEQ | COUNTER_LO | COUNTER_HI | CHECK`

The records form a circular journal:

```text
slot 0 -> slot 1 -> ... -> slot 15 -> slot 0 -> ...
```

The next slot is invalidated first. Its sequence number, counter, complement
and checksum are then written. The final `MAGIC` byte is written last.

At startup all slots are scanned and the newest valid sequence number is
selected using an 8-bit wrap-aware comparison.

This spreads EEPROM writes over 16 records instead of repeatedly writing
the same EEPROM cells.

The firmware verifies the record after writing before allowing RF transmission.

If power is lost while a new EEPROM record is being written, the previous
valid record remains available. No EEPROM-only scheme can guarantee a
transaction that has not yet completed.

The firmware therefore follows this order:

```text
counter++
    |
EEPROM commit
    |
verify
    |
RF transmission
```

This prevents transmission of a new rolling code before its counter value
has been durably stored.

## Build

The project is prepared for Microchip XC8 and MPLAB X. The intended compiler is XC8 for PIC12F629. A local XC8 installation is required to produce the device HEX file.

`make` invokes `xc8-cc` when it is available.

Host-side KeeLoq tests can be run with:

```sh
cd tests
make
```

## Source provenance

The KeeLoq primitive follows the implementation in DarkFlippers/unleashed-firmware `lib/subghz/protocols/keeloq_common.c` and its associated header. The project does not copy the Flipper runtime; the cipher is adapted to a freestanding PIC12F629 environment.

Reference: https://github.com/DarkFlippers/unleashed-firmware
Microchip HCS300 datasheet: DS21137F.

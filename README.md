# Roomba Virtual Wall v3
(in a tiny package)

This is a virtual wall emitter for your Roomba, packaged on a 25mm-diameter PCB,
designed to be mounted in a 1-inch hole drilled in a doorframe or wall. I would
like to get the PCB (and mounting hole) even smaller, but the battery size is
the limiting factor, and a smaller coin cell would have much shorter life.

This project is not affiliated with iRobot Corporation in any way.
The "Roomba" name is a trademark of iRobot and used here only for clarity on the
purpose of this hardware and software.

![Front of PCB](https://644db4de3505c40a0444-327723bce298e3ff5813fb42baeefbaa.ssl.cf1.rackcdn.com/1f0f042e650c3d6e857f8a47daa28382.png)

This revised version of the project is driven by an ATtiny10. On board is the IR
emitter that signals the Roomba, an indicator LED and a push-button switch for
turning the wall on or off. Power is supplied by a CR2032 coin cell mounted on
the back side of the PCB.

To reduce power consumption, this iteration has a 1.8V DC-DC converter and runs
the tiny10 on an external 38.4 kHz crystal, much slower (and lower power consumption)
than the internal 8/1 MHz clock. The oscillator size was chosen because it is also
the carrier frequency of the IR signal, letting the crystal do double duty and
off-loading that work from the MCU.

### Project Structure

* main.c includes the main loop, controls power states, and includes several of
the interrupt service routines.
* pulse.c is the timing of the carrier and pulse signals
* pwm_ramp.c is the fade-on and fade-off control for the indicator LED

### Build and Install

Requires avr-gcc and avrdude in your $PATH. Installation with avrdude is configured
for a usbtiny, replace it with your programmer device as needed. Useful targets:

* `make all` - build and flash to the board
* `make compile` - compile to an installable binary (.hex) in ./build/
* `make clean` - removes the hex and intermediate files

Notice that there is no ICSP header on the v3 board. The tiny10 will need to be
flashed using pads on a board with header pins (see https://oshpark.com/shared_projects/2gqu3qVQ)
before soldering onto the in-wall PCB.

## IR signal details

The Roomba-brand virtual wall emits an IR pulse train at 940nm, on a
38.4 kHz carrier. The specific signal is 0.5 ms on, followed by 7.5 ms off,
three times in a row. This sequence is triggered every 150 ms. This sequence
was determined by pointing an OEM wall at an IR-sensitive phototransistor and
connecting the circuit to an oscilloscope.

## Schematic and BOM

PCBs are available from OSHPark at https://oshpark.com/shared_projects/ZjEGFO3S/

Board schematic and layout was built on Upverter. The project is available to download or
fork at https://upverter.com/design/xpctr8/2f7132ff9f9e39a7/.

**NOTE:** as of 2019-07-05,
there is a bug on Upverter's Gerber generator that exports the battery negative
pad on the front of the board. Use the corrected Gerber zip file in this repo if
if you plan to send them to another fab house.

**NOTE:** as of 2023-02-20, I no longer have a local copy of the BOM or schematic and Upverter's "Classic" design mode is not allowing me to login and the link above gets a bad gateway error. When and if I can login, I'll pull and post the files.

~~BOM csv file is in the repo.~~

(BOM TODO)

## Licenses

Open Source hardware. You are free to use, fork, modify, etc.

This is open source software, released under the MIT license:

Copyright 2019 Ben Bredesen

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

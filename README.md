# Roomba Virtual Wall
(in a tiny package)

This is a virtual wall emitter for your Roomba, packaged on a 25mm-diameter PCB,
designed to be mounted in a 1-inch hole drilled in a doorframe or wall. I would
like to get the PCB (and mounting hole) even smaller, but the battery size is
the limiting factor, and a smaller coin cell would have much shorter life.

(PCB images)

The initial version of the project is driven by an attiny85. On board is the IR
emitter that signals the Roomba, an indicator LED and a push-button switch for
turning the wall on or off. Power is supplied by a CR2032 coin cell mounted on
the back side of the circuit board.

### File Structure

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

## Version 2:

This is a functional project and I have several of them installed at home
already. However, battery life (if left on full time) is about 2 months, which is much shorter than I would like. The next hardware revision will explore using an attiny10 (which can have significantly lower power consumption) and a voltage regulator to drop the operating voltage for the whole circuit down to 1.8V.

Reducing the voltage will mean lower consumption by the MCU, as well as lower
power dissipation in the constant-current circuit that drives the IR emitter.
The emitter itself is, by far, the highest current consumer on the board (about
20 mA peak current, 0.5% duty cycle), so I also intend to let the MCU sleep
longer between pulses.

I'll also be looking at using an external crystal,
instead of the attiny's internal RC oscillator, which surely consumes more
power. We may be able to push the tiny's frequency down to the 100 kHz range,
as it is overpowered and left idle at the stock 1 MHz.

## IR signal details

The Roomba stock virtual wall emits an IR pulse train at 940nm, on a
~38.5 kHz carrier. The specific signal is 0.5 ms on, followed by 7.5 ms off,
three times in a row. This sequence is triggered every 150 ms.

Timer0 on the tiny85 is used to create the 38.5 kHz carrier wave, and Timer1 is
used to time the on/off pulses.

## Schematic and BOM

Board schematic was built on Upverter, the project is available to download or
fork at (URL TODO).

(BOM TODO)

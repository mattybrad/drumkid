# DrumKid Hackers' Manual
## Introduction
This manual is designed for advanced users of DrumKid who are already familiar with the instrument's basic features and would like to customise DrumKid. Most ideas described in this manual will require some skills in programming and/or electronics, but don't let that put you off - even if you don't currently have those skills, this might be a good way to learn!

## What's inside DrumKid?
The front panel of DrumKid is a printed circuit board (PCB), with some components mounted on top (buttons, knobs, LEDs, power switch, headphone output) and some hidden underneath. The stuff underneath is what we're mainly concerned with in this hackers' manual.

If you're only modifying DrumKid's software/code, you won't even need to do any disassembly, but it's useful to see and understand what you're modifying before you get stuck in. Using a cross-head screwdriver, unscrew the six machine screws on the back of DrumKid, and remove the rear cover. You should now be able to see all the circuitry, including the Arduino Nano board which controls everything.

The Arduino Nano is a microcontroller - basically a small, low-powered computer. This computer runs a single "sketch", which is the name for a piece of software running on an Arduino. This sketch monitors DrumKid's buttons and knobs, controls its LEDs, sends and receives data via the MIDI ports, and (most importantly) generates an audio signal.

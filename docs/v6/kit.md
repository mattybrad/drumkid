# Building DrumKid from a kit
Thanks for buying a DrumKid DIY kit! This guide will take you through the process of building your DrumKid step by step.

## Overview
Here's a quick list of the steps you're going to go through:
- Check parts
- Solder components to the underside of the PCB
- Solder components to the top side of the PCB
- Insert the chips
- Upload the test program
- Use the test program to check everything is soldered properly
- Upload the DrumKid firmware
- Solder the battery box leads
- Insert batteries (if using them)
- Screw everything together

## Parts
Please check that your kit contains all of the following parts:
- PCB (printed circuit board)
- Write this list later
If you are missing anything, please send me an email.

## Soldering
DrumKid is designed to be relatively easy to solder. All of the components are "through-hole", which means that you poke the leads through the holes and then solder them in place on the other side. The only aspect of DrumKid that is a little counter-intuitive is that some components are soldered on the front of the board, and some on the back. Make sure to double-check the pictures before you solder each component.

One other thing that is potentially slightly confusing about DrumKid is that I have included some spaces on the PCB for extra components in case you want to customise your DrumKid in the future. Don't worry if you get to the end and find there are still holes with no components in them - that's normal!

If you're unsure about soldering, this online guide is a great place to start: https://learn.adafruit.com/adafruit-guide-excellent-soldering

I've found that the best way to assemble DrumKid is to start by soldering the components on the underside of the PCB (resistors, capacitors, etc) and then solder the components on the top side (buttons, LEDs, etc). It's also much easier if you solder everything in order of size, starting with the smallest components. Here's the order in which I solder everything:
- 1N4148 diode (be careful of the polarity - match the stripe to the strip on the PCB)
- Resistors
- 1N5817 diode (again, careful of the polarity - match the stripe)
- IC socket (8-pin socket for the 6N138 chip)
- MIDI/Arduino switch (make sure it)
- Headers (15-pin sockets) for the Arduino (start by soldering just one or two pins, and get the headers properly aligned before soldering all the pins)
- Capacitors
- Flip the board over...
- Buttons
- Audio output
- Power switch
- LEDs (careful of the polarity - the long legs are positive) (also, it's a bit fiddly to get the LEDs aligned with each other - solder the positive legs first to make it easier to align them with each other)
- Potentiometers
- MIDI sockets

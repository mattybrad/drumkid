# DrumKid Hackers' Manual
## Introduction
This manual is designed for advanced users of DrumKid who are already familiar with the instrument's basic features and would like to customise DrumKid. Most ideas described in this manual will require some skills in programming and/or electronics, but don't let that put you off - even if you don't currently have those skills, this might be a good way to learn!

## What's possible?
Here are a few examples of things that you could do with DrumKid using this guide:
- Load a different set of drum samples
- Change the drone waveform
- Allow DrumKid to play even more time signatures
- Replace one of the parameters with an effect that you program yourself
- Add a light sensor to control certain parameters
- Add Eurorack CV control
- Mount DrumKid to a pedalboard or similar
- Build an enclosed wooden case for DrumKid

## What's inside DrumKid?
The front panel of DrumKid is a printed circuit board (PCB), with some components mounted on top (buttons, knobs, LEDs, power switch, headphone output) and some hidden underneath. The stuff underneath is what we're mainly concerned with in this hackers' manual.

If you're only modifying DrumKid's software/code, you won't even need to do any disassembly, but it's useful to see and understand what you're modifying before you get stuck in. Using a cross-head screwdriver, unscrew the six machine screws on the back of DrumKid, and remove the rear cover. You should now be able to see all the circuitry, including the Arduino Nano board which controls everything.

The Arduino Nano is a microcontroller - basically a small, low-powered computer. This computer runs a single "sketch", which is the name for a piece of software running on an Arduino. This sketch monitors DrumKid's buttons and knobs, controls its LEDs, sends and receives data via the MIDI ports, and (most importantly) generates an audio signal.

Each of the Arduino's pins (the metal bits along the edges) corresponds to a different signal either going into or out of the board, and there is a spare set of "breakout" holes that you can use to solder extra components which will connect to any of the pins. To access the other side of the board for soldering, simply unscrew the four machine screws on the front of DrumKid which hold the plastic cover in place.

## Reprogramming DrumKid
The most common modification is probably to upload new code to the Arduino Nano, either to upgrade the firmware (software) to the most recent version, to add your own drum samples, or to edit some other part of the code. To do this, you will need a computer running the latest Arduino IDE software (download from https://www.arduino.cc/) and a USB "mini-B" cable to connect the computer to the Arduino Nano.

Before you upload anything to the Arduino, make sure the switch on the underside of the PCB is set to "Arduino" rather than "MIDI" (and remember to switch it back to "MIDI" when you're done). To test that you're able to upload code, try uploading the "Blink" example sketch (there are various tutorials online to help you with this). If successful, one of the LEDs should start flashing on and off. Then try uploading the latest DrumKid code from https://github.com/mattybrad/drumkid - start by downloading the whole project (repository) as a ZIP file, then unzip it to a folder on your computer and open the v6/arduino/drumkid/drumkid.ino file and upload this sketch to the Arduino.

If you encounter any problems uploading code to the Arduino, check the following:
- The switch on the underside of the PCB should be set to "Arduino"
- Detach any MIDI cables from DrumKid
- Try (carefully) removing the Arduino Nano from DrumKid and uploading the "Blink" example sketch
- Try a different USB cable

## Hacking Drumkid's circuit
...

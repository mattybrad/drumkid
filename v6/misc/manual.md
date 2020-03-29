# DrumKid manual (work in progress, i.e. not gospel)
## Introduction
Welcome to the DrumKid manual! DrumKid creates rhythms using random numbers. To get started with DrumKid, unscrew the six screws on the back of the unit, and carefully remove the back plate. Insert two AA batteries, replace the back plate, and tighten the screws. Slide the power switch to "on" - you should see the lights flash briefly. After this, DrumKid is ready to use. Plug some headphones or a 3.5mm aux cable into the socket, and start playing.
## Philosophy
DrumKid is designed around a very basic computer chip with limited memory and processing power. My idea was to squeeze as much cool sound out of this chip as possible, but it has its limits. Rather than try and hide from these limits, I decided to embrace them. This means that it is absolutely possible to make horrible, distorted, broken noises using DrumKid. Some of these horrible noises will sound cool, others will not. DrumKid is a playable instrument like any other - experiment with it and see what happens.
## Basics
DrumKid has six buttons and four knobs. The basic functions are as follows:
 - Press the start/stop button to start or stop the rhythm
 - Press buttons A/B/C/D to select different knob functions (see table below)
 - Turn any of the four knobs to alter the parameters in the current selected group (see table below)
 - Tap the tap tempo button repeatedly to set the tempo
 - Press buttons A+B together to load a session, then choose which session to load by pressing any of the buttons
 - Press buttons C+D together to save a session, then choose which slot to save your session in by pressing any of the buttons
 - Press buttons B+C together to change the current active memory bank, then choose a specific memory bank by pressing any of the buttons
 - Press buttons A+D together to reset all parameters to their defaults (this will not overwrite any sessions you have saved) THIS FEATURE HAS NOT YET BEEN IMPLEMENTED
## Parameters
There are 16 different parameters which can be adjusted on DrumKid, split into five groups, with each group's four parameters controlled by the four knobs. The parameters are grouped as follows:

### Group A
 1. Chance
 2. Zoom
 3. Range
 4. Midpoint
### Group B
 1. Pitch
 2. Crush
 3. Crop
 4. Drop
### Group C
 1. Drone mix
 2. Drone modulation
 3. Drone tuning
 4. Drone root note
### Group D
 1. Beat
 2. Time signature
 3. Swing
 4. Tempo

You can try out each parameter by starting a rhythm (using the start/stop button), then selecting a group and turning the different knobs. For example, try selecting group B (by pressing the B button) then turning the first knob, which will now control "pitch". You should hear the sound change. Below are descriptions of exactly what each parameter does, and how it can be used.

**Chance** - The probability of extra drum hits being generated. At zero, the beat will be completely unchanged, while for higher values there will be lots of extra hits, creating a busier, messier beat. This can be used in conjunction with zoom, midpoint, and range to create drum fills that transition organically from a simple beat.

**Zoom** - Chooses which subdivision of the beat should be affected by the randomly generated extra hits. When zoom is at zero, beats will only be generated for "whole notes" (i.e. the first beat of the bar). Higher values will affect half notes, quarter notes, eighth notes, and so on. A medium value is a good place to start for zoom, but turning it up can make a beat or fill sound more "urgent". Some of the smaller subdivisions will vary depending on the current "swing" setting.

**Range** - This is the range of velocities which will be assigned to the random hits, and works in conjunction with "midpoint" (see below). For all the random hits to have the same velocity as each other, keep this value at zero. For a wide range of velocities, turn this up high.

**Midpoint** - This is the average velocity of the randomly generated hits, and works in conjunction with "range". If you set this to maximum and have range set at zero, all the randomly generated hits will be at max velocity. If you have this set in the middle with range set to max, you will get a wide range of velocities, from zero to max. Try keeping midpoint at a constant low-to-medium setting to add a bustling undercurrent to your beat, or turn it up briefly to create a drum fill.

**Pitch** - Alters the playback speed of the samples. Will play samples backwards if you turn the knob below halfway.

**Crush** - Reduces the number of bits used to calculate the audio output, creating a digital distortion effect. Higher values are clean, lower values are more distorted (fewer bits). CHECK THIS BEFORE RELEASE, MIGHT BE REVERSED

**Crop** - Crops the end of the samples, creating a staccato effect. Lower values are more cropped. CHECK THIS BEFORE RELEASE, MIGHT BE REVERSED

**Drop** - This control mutes some or all of the drum channels, allowing you to quickly "drop" everything except the hi-hat and snare, for example, or only retain the kick drum. Broadly, this control has "treble-y" channels at one end and "bass-y" channels at the other. The setting corresponding to all the channels being audible is somewhere in the middle.

**Beat** - Chooses between a series of basic underlying drum patterns.

**Time signature** - Alters the time signature of the beat, i.e. the number of steps in the pattern. The possible values are 4/4, 5/4, 6/4 and 7/4.

**Swing** - Creates a swing feel to a beat by altering the timing of certain hits. There are three settings: straight, partial swing, and full (triplet) swing.

**Tempo** - Alters the tempo (BPM) of the beat. Will override any tempo previously set using the tap tempo function.

**Drone modulation** - DrumKid generates a drone which can be used to modulate the audio signal from the drums, creating a robotic effect. When set at halfway, no effect is heard. Turning the knob to the left modulates the drums with a single drone, while turning it to the right modulates the drums with two tones, a fifth apart. Turning the knob all the way left or right creates an extreme effect, while values closer to halfway will be more subtle.

**Drone mix** - Controls the amount of the raw drone signal added to the mix. As with the modulation effect, turning to the left adds a single tone, while turning to the right adds two tones, a fifth apart. A halfway value mutes the drone.

**Drone tuning** - Controls the overall tuning of the drones (both the single and fifth tones).

**Drone root note** - Alter the root note of the drones, in semitone steps over an octave range.

## Other info
Because DrumKid uses a lo-fi method (known as pulse width modulation) to generate its audio signal, there is a trace, high-frequency noise present in the output. While this signal is above most adults' range of hearing, and has been largely filtered out by DrumKid's own circuitry anyway, you may want to filter it out further if you are recording DrumKid in a studio, especially if you intend to pitch-shift the recording downwards (since this could bring the noise within human hearing range). To remove the noise, use a band stop filter with a frequency of 16384Hz.
## Hacking DrumKid
DrumKid is an open source project, based on the Arduino Uno, and is designed in such a way that it can be modified and repaired. The source files for DrumKid are available from [https://github.com/mattybrad/drumkid/](https://github.com/mattybrad/drumkid/) - you will find the schematics, CAD files, parts list, source code and more. Here is a list of ways in which you could modify DrumKid:
 - If you have a spare Arduino Uno, you can edit DrumKid's source code and upload it to DrumKid's ATmega328 chip by replacing the Arduino's chip with DrumKid's, then doing an upload, then replacing the chip in DrumKid.
 - If you would like to connect other components to DrumKid, such as a MIDI output or additional LEDs, there are "breakout" connections for all of the Arduino pins.
 - If you would prefer a different colour or material for the front and rear panels, the DXF files for these parts are available to download - you can either print them out as a template to make your own new panels at home, or use them directly in a laser cutter or 3D printer.

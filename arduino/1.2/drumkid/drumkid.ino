/*  DrumKid firmware V1.2
 *  This code should work for DrumKid boards from V6 onwards.
 *  It might work with earlier versions with a bit of tweaking :) 
 */

// include debouncing library to make buttons work reliably
#include "src/Bounce2/src/Bounce2.h"

// include Mozzi library files for generating audio (custom version to allow reverse playback)
#include "src/MozziDK/src/MozziGuts.h"
#include "src/MozziDK/src/Sample.h"
#include "src/MozziDK/src/mozzi_rand.h"
#include "src/MozziDK/src/Oscil.h"
#include "src/MozziDK/src/tables/saw256_int8.h"

// include drum beat pattern definitions from separate file
#include "beats.h"

// include sample data (generated via Python script)
#include "sample0.h"
#include "sample1.h"
#include "sample2.h"
#include "sample3.h"
#include "sample4.h"

// declare samples as Mozzi sample objects
Sample <sample0_NUM_CELLS, AUDIO_RATE> sample0(sample0_DATA);
Sample <sample1_NUM_CELLS, AUDIO_RATE> sample1(sample1_DATA);
Sample <sample2_NUM_CELLS, AUDIO_RATE> sample2(sample2_DATA);
Sample <sample3_NUM_CELLS, AUDIO_RATE> sample3(sample3_DATA);
Sample <sample4_NUM_CELLS, AUDIO_RATE> sample4(sample4_DATA);

// define oscillators for drone
Oscil<SAW256_NUM_CELLS, AUDIO_RATE> droneOscillator1(SAW256_DATA);
Oscil<SAW256_NUM_CELLS, AUDIO_RATE> droneOscillator2(SAW256_DATA);

// include EEPROM library for saving data
#include <EEPROM.h>

// declare buttons as Bounce objects
Bounce buttons[6];

// Mozzi control rate, measured in Hz, must be power of 2
// try to keep this value as high as possible (256 is good) but reduce if having performance issues
#define CONTROL_RATE 128

// define various values
#define NUM_KNOBS 4
#define NUM_LEDS 5
#define NUM_BUTTONS 6
#define NUM_SAMPLES 5
#define NUM_PARAM_GROUPS 4
#define SAVED_STATE_SLOT_BYTES 24
#define NUM_TAP_TIMES 8
#define MAX_BEATS_PER_BAR 13

// define pin numbers
// keep pin 9 for audio, but others can be changed to suit your breadboard layout
const byte ledPins[5] = {2,3,11,12,13};
const byte buttonPins[6] = {4,5,6,7,8,10};
const byte analogPins[4] = {A0,A1,A2,A3};

// various other global variables
float nextPulseTime = 0.0; // the time, in milliseconds, of the next pulse
float msPerPulse = 20.8333; // time for one "pulse" (there are 24 pulses per quarter note, as defined in MIDI spec)
byte pulseNum = 0; // 0 to 23 (24ppqn, pulses per quarter note)
unsigned int stepNum = 0; // 0 to 192 (max two bars of 8 beats, aka 192 pulses)
unsigned int numSteps; // number of steps used - dependent on time signature
unsigned int specialOffset = 0; // number of steps to offset pattern in random time signature mode
bool beatPlaying = false; // true when beat is playing, false when not
byte noteDown = B00000000; // keeps track of whether a MIDI note is currently down or up
bool syncReceived = false; // has a sync/clock signal been received? (IMPROVE THIS LATER)
byte midiNotes[NUM_SAMPLES]; // MIDI note numbers
const byte defaultMidiNotes[NUM_SAMPLES] = {36,42,38,37,43}; // default MIDI note numbers
byte midiNoteCommands[NUM_SAMPLES]; // MIDI note on commands
byte sampleVolumes[NUM_SAMPLES] = {0,0,0,0,0}; // current sample volumes
byte storedValues[NUM_PARAM_GROUPS*NUM_KNOBS]; // analog knob values, one for each parameter (so the value can be remembered even after switching groups)
byte firstLoop = true; // allows special actions on first loop
byte secondLoop = false; // allows special actions on second loop
bool newMidiDroneRoot = false;
byte controlSet = 0; // current active set of parameters
byte knobLocked = B11111111; // record whether a knob's value is currently "locked" (i.e. won't alter effect until moved by a threshold amount)
byte analogValues[NUM_KNOBS]; // current analog (knob) values
byte initValues[NUM_KNOBS]; // initial parameter values
byte specialLedDisplayNum; // binary number to display on LEDs (when needed)
unsigned long specialLedDisplayTime = 0; // the last time the LEDs were told to display a number

// save/load variables
bool readyToChooseSaveSlot = false;
bool readyToChooseLoadSlot = false;
bool readyToChooseBank = false;
bool newStateLoaded = false;
byte activeBank = 0;
byte activeMidiSettingsNum = 0;

// tempo variables
float tapTimes[NUM_TAP_TIMES];
byte nextTapIndex = 0;

// parameter variables, each with a range of 0-255 unless otherwise noted
byte paramChance;
byte paramZoom;
int paramMidpoint; // -255 to 255
int paramRange; // 0 to 510
byte paramPitch;
byte paramCrush;
byte paramCrop;
byte paramDrop;
byte paramBeat; // 0 to 23 (24 beats in total)
byte paramTimeSignature; // 1 to 13 (3/4, 4/4, 5/4, 6/4, 7/4)
float paramTempo; 
byte paramSwing = 0; // 0 to 2 (0=straight, 1=approx quintuplet swing, 2=triplet swing)
byte paramDroneRoot; // 0 to 11 (all 12 possible semitones in an octave)
float paramDronePitch;
byte paramDroneMod;

// special global variables needed for certain parameters
byte crushCompensation;
byte previousBeat;
byte previousDroneRoot;
byte activeDroneRoot;
byte previousTimeSignature;
byte oscilGain1 = 255;
byte oscilGain2 = 255;
bool droneMod2Active = false;
#define MIN_TEMPO 40
#define MAX_TEMPO 295

#if MIN_TEMPO + 255 != MAX_TEMPO
#error "Tempo must have a range of exactly 255 otherwise I'll have to write more code"
#endif

// define which knob controls which parameter
// e.g. 0 is group 1 knob 1, 6 is group 2 knob 3
#define CHANCE 0
#define ZOOM 1
#define RANGE 2
#define MIDPOINT 3

#define PITCH 4
#define CRUSH 5
#define CROP 6
#define DROP 7

#define DRONE 8
#define DRONE_MOD 9
#define DRONE_PITCH 10
#define DRONE_ROOT 11

#define BEAT 12
#define TIME_SIGNATURE 13
#define SWING 14
#define TEMPO 15

// define root notes
float rootNotes[13] = {
  138.5913155f,
  146.832384f,
  155.5634919f,
  164.8137785f,
  174.6141157f,
  184.9972114f,
  195.997718f,
  207.6523488f,
  220.0f,
  233.0818808f,
  246.9416506f,
  261.6255653f,
  277.182631f,
};

// determines whether each sample is played or muted in a particular drop mode
byte dropRef[NUM_SAMPLES] = {
  B00011110, // kick
  B11111000, // hat
  B00110000, // snare
  B01111000, // rim
  B00011100, // tom
};

void setup(){
  byte i;

  checkEepromScheme(); // write defaults to memory if not previously done
  initialiseSettings(false);
  
  for(i=0;i<NUM_LEDS;i++) {
    pinMode(ledPins[i],OUTPUT); // set LED pins as outputs
  }

  randSeed((long)analogRead(4)*analogRead(5)); // A4 and A5 should be floating, use to seed random numbers
  startMozzi(CONTROL_RATE);

  // initialise all buttons using Bounce2 library
  for(i=0; i<NUM_BUTTONS; i++) {
    buttons[i].interval(25);
    buttons[i].attach(buttonPins[i], INPUT_PULLUP);
  }

  // set starting values for each parameter
  resetSessionToDefaults();
  
  Serial.begin(31250); // begin serial communication for MIDI input/output
  //Serial.begin(9600);

  flashLeds(); // do a brief LED light show to show the unit is working
}

byte buttonsPressed = 0;
byte buttonGroup = 0;
byte lastButtonsPressed = 0;
byte menuState = 0;

void updateControl(){
  byte i;
  bool controlSetChanged = false;
  newStateLoaded = false;
  byte prevControlSet = controlSet;
  
  for(i=0; i<6; i++) {
    buttons[i].update();
  }
  for(i=0; i<6; i++) {
    bitWrite(buttonsPressed,i,!buttons[i].read());
    buttonGroup = buttonsPressed | buttonGroup;
  }
  if(buttonsPressed != lastButtonsPressed) {
    if(menuState == 0) {
      if(buttons[0].fell()) {
        if(!beatPlaying) startBeat();
        else stopBeat();
      }
      if(buttons[5].fell()) {
        tapTimes[nextTapIndex] = (float) millis();

        float firstTime;
        float timeTally = 0.0;
        byte validTimes = 0;
        bool keepChecking = true;
        for(i=0;i<NUM_TAP_TIMES-1 && keepChecking;i++) {
          byte thisTapIndex = (nextTapIndex + NUM_TAP_TIMES - i) % NUM_TAP_TIMES;
          byte lastTapIndex = (nextTapIndex + NUM_TAP_TIMES - i - 1) % NUM_TAP_TIMES;

          float thisTime = tapTimes[thisTapIndex] - tapTimes[lastTapIndex];
          if(i==0) firstTime = thisTime;
          float timeCompare = firstTime/thisTime;
          if(tapTimes[lastTapIndex] > 0.0 && timeCompare > 0.8 && timeCompare < 1.2) {
            timeTally += thisTime;
            validTimes ++;
          } else {
            keepChecking = false;
          }
        }
        if(validTimes>=2) {
          float newPulseLength = (timeTally / validTimes) / 24;
          paramTempo = 2500.0 / newPulseLength;
          storedValues[TEMPO] = tempoToByte(paramTempo);
        }
        nextTapIndex = (nextTapIndex + 1) % NUM_TAP_TIMES;
        
        if(controlSet==TEMPO/NUM_KNOBS) {
          byte tempoKnobNum = TEMPO%NUM_KNOBS;
          bitWrite(knobLocked, tempoKnobNum, true);
          initValues[tempoKnobNum] = analogValues[tempoKnobNum];
        }
      }
      if(buttonsPressed == 0) {
        if(buttonGroup == B00000010) {
          controlSet = 0;
        } else if(buttonGroup == B00000100) {
          controlSet = 1;
        } else if(buttonGroup == B00001000) {
          controlSet = 2;
        } else if(buttonGroup == B00010000) {
          controlSet = 3;
        } else if(buttonGroup == B00000110) {
          // load...
          menuState = 1;
          specialLedDisplay(B00000011,true);
        } else if(buttonGroup == B00011000) {
          // save...
          menuState = 2;
          specialLedDisplay(B00001100,true);
        } else if(buttonGroup == B00001100) {
          // change bank...
          menuState = 3;
          specialLedDisplay(B00000110,true);
        } else if(buttonGroup == B00001110) {
          // reset
          resetSessionToDefaults();
          specialLedDisplay(B00011111,true);
        } else if(buttonGroup == B00010110) {
          createRandomSession();
          specialLedDisplay(B00011111,true);
        } else if(buttonGroup == B00011100) {
          // MIDI settings
          menuState = 4;
          specialLedDisplay(B00001110,true);
        }
        buttonGroup = 0;
      }
    } else if(menuState==1) {
      if(buttonsPressed == 0) {
        if(buttonGroup == B00000001) loadParams(0);
        else if(buttonGroup == B00000010) loadParams(1);
        else if(buttonGroup == B00000100) loadParams(2);
        else if(buttonGroup == B00001000) loadParams(3);
        else if(buttonGroup == B00010000) loadParams(4);
        else if(buttonGroup == B00100000) loadParams(5);
        menuState = 0;
        buttonGroup = 0;
      }
    } else if(menuState==2) {
      if(buttonsPressed == 0) {
        if(buttonGroup == B00000001) saveParams(0);
        else if(buttonGroup == B00000010) saveParams(1);
        else if(buttonGroup == B00000100) saveParams(2);
        else if(buttonGroup == B00001000) saveParams(3);
        else if(buttonGroup == B00010000) saveParams(4);
        else if(buttonGroup == B00100000) saveParams(5);
        menuState = 0;
        buttonGroup = 0;
      }
    } else if(menuState==3) {
      if(buttonsPressed == 0) {
        if(buttonGroup == B00000001) chooseBank(0);
        else if(buttonGroup == B00000010) chooseBank(1);
        else if(buttonGroup == B00000100) chooseBank(2);
        else if(buttonGroup == B00001000) chooseBank(3);
        else if(buttonGroup == B00010000) chooseBank(4);
        else if(buttonGroup == B00100000) chooseBank(5);
        menuState = 0;
        buttonGroup = 0;
      }
    } else if(menuState==4) {
      if(buttonsPressed == 0) {
        menuState = 5; // single button presses for buttons 1-5 take you to menuState 5
        if(buttonGroup == B00000001) activeMidiSettingsNum = 0;
        else if(buttonGroup == B00000010) activeMidiSettingsNum = 1;
        else if(buttonGroup == B00000100) activeMidiSettingsNum = 2;
        else if(buttonGroup == B00001000) activeMidiSettingsNum = 3;
        else if(buttonGroup == B00010000) activeMidiSettingsNum = 4;
        else if(buttonGroup == B00100000) menuState = 0; // tap tempo button exits to main menu state
        else if(buttonGroup == B00111111) {
          initialiseSettings(true);
          menuState = 0;
          specialLedDisplay(B00011111,true);
        }
        if(menuState==5) specialLedDisplay(activeMidiSettingsNum,false); // show which button was pressed
        buttonGroup = 0;
      }
    } else if(menuState==5) {
      if(buttonsPressed == 0) {
        int newMidiNote = midiNotes[activeMidiSettingsNum];
        int newMidiChannel = midiNoteCommands[activeMidiSettingsNum] - 0x90;
        if(buttonGroup == B00000001) {
          if(newMidiNote>0) newMidiNote --;
          specialLedDisplay(newMidiNote, false);
        } else if(buttonGroup == B00000010) {
          if(newMidiNote<127) newMidiNote ++;
          specialLedDisplay(newMidiNote, false);
        } else if(buttonGroup == B00000100) {
          if(newMidiChannel>0) newMidiChannel --;
          specialLedDisplay(newMidiChannel, false);
        } else if(buttonGroup == B00001000) {
          if(newMidiChannel<15) newMidiChannel ++;
          specialLedDisplay(newMidiChannel, false);
        } else if(buttonGroup == B00100000) {
          EEPROM.write(872+activeMidiSettingsNum, newMidiNote);
          EEPROM.write(880+activeMidiSettingsNum, newMidiChannel);
          menuState = 0;
          specialLedDisplay(B00011111,true);
        } else if(buttonGroup == B00111111) {
          initialiseSettings(true);
          menuState = 0;
          specialLedDisplay(B00011111,true);
        }
        if(menuState==5) {
          midiNotes[activeMidiSettingsNum] = newMidiNote;
          midiNoteCommands[activeMidiSettingsNum] = 0x90 + newMidiChannel;
        }
        buttonGroup = 0;
      } else if(buttonsPressed == B00010000) {
        // play test MIDI note
        triggerNote(activeMidiSettingsNum,255);
      }
    }
  }
  
  lastButtonsPressed = buttonsPressed;
  controlSetChanged = (prevControlSet != controlSet);

  msPerPulse = 2500.0 / paramTempo;

  // perform pulse actions inside loop - stop beat if everything gets super overloaded
  byte numLoops = 0;
  while(!syncReceived && beatPlaying && millis()>=nextPulseTime) {
    if(numLoops<5) doPulseActions();
    nextPulseTime = nextPulseTime + msPerPulse;
    if(numLoops>=100) beatPlaying = false;
    numLoops ++;
  }

  for(i=0;i<NUM_KNOBS;i++) {
    if(firstLoop) {
      byte dummyReading = mozziAnalogRead(analogPins[i]); // need to read pin once because first reading is not accurate
    } else {
      analogValues[i] = mozziAnalogRead(analogPins[i])>>2;
    }
  }
  if(controlSetChanged||secondLoop||newStateLoaded) {
    for(i=0;i<NUM_KNOBS;i++) {
      bitWrite(knobLocked, i, true);
      initValues[i] = analogValues[i];
    }
  } else {
    for(i=0;i<NUM_KNOBS;i++) {
      if(bitRead(knobLocked, i)) {
        int diff = initValues[i] - analogValues[i];
        if(diff<-5||diff>5) bitWrite(knobLocked, i, false);
      }
      if(!bitRead(knobLocked, i)) {
        storedValues[NUM_KNOBS*controlSet+i] = analogValues[i];
      }
    }
  }
  if(secondLoop) {
    for(i=0;i<NUM_PARAM_GROUPS;i++) {
      updateParameters(i);
    }
  } else {
    updateParameters(controlSet);
  }

  if(firstLoop) {
    firstLoop = false;
    secondLoop = true;
  } else {
    secondLoop = false;
  }

  if(!beatPlaying) updateLeds();
}

void doPulseActions() {
  Serial.write(0xF8); // MIDI clock
  cancelMidiNotes();
  if(pulseNum%24==0) {
    //if(stepNum==0||(paramTimeSignature==4&&stepNum==96)||(paramTimeSignature==6&&stepNum==72)) {
    if(stepNum==0) {
      if(paramTimeSignature==MAX_BEATS_PER_BAR+1) {
        numSteps = rand(1,8) * 24;
        specialOffset = rand(1,8) * 24;
      } else {
        numSteps = paramTimeSignature * 24; // 24 pulses per beat
        specialOffset = 0;
      }
      activeDroneRoot = paramDroneRoot;
      //if(numSteps == 96) numSteps = 192; // allow 4/4 signature to use two bars of defined beat
    }
  } 
  playPulseHits();
  updateLeds();
  incrementPulse();
}

void updateParameters(byte thisControlSet) {
  switch(thisControlSet) {
    case 0:
    paramChance = storedValues[CHANCE];
    paramZoom = storedValues[ZOOM];
    paramRange = 2 * storedValues[RANGE];
    paramMidpoint = 2*storedValues[MIDPOINT] - 255;
    break;

    case 1:
    {
      // pitch alteration (could do this in a more efficient way to reduce RAM usage)
      float thisPitch = fabs((float) storedValues[PITCH] * (8.0f/255.0f) - 4.0f);
      float newKickFreq = thisPitch * (float) sample0_SAMPLERATE / (float) sample0_NUM_CELLS;
      float newHatFreq = thisPitch * (float) sample1_SAMPLERATE / (float) sample1_NUM_CELLS;
      float newSnareFreq = thisPitch * (float) sample2_SAMPLERATE / (float) sample2_NUM_CELLS;
      float newRimFreq = thisPitch * (float) sample3_SAMPLERATE / (float) sample3_NUM_CELLS;
      float newTomFreq = thisPitch * (float) sample4_SAMPLERATE / (float) sample4_NUM_CELLS;
      sample0.setFreq(newKickFreq);
      sample1.setFreq(newHatFreq);
      sample2.setFreq(newSnareFreq);
      sample3.setFreq(newRimFreq);
      sample4.setFreq(newTomFreq);
      bool thisDirection = storedValues[PITCH] >= 128;
      sample0.setDirection(thisDirection);
      sample1.setDirection(thisDirection);
      sample2.setDirection(thisDirection);
      sample3.setDirection(thisDirection);
      sample4.setDirection(thisDirection);
      
      // bit crush! high value = clean (8 bits), low value = dirty (1 bit?)
      paramCrush = 7-(storedValues[CRUSH]>>5);
      crushCompensation = paramCrush;
      if(paramCrush >= 6) crushCompensation --;
      if(paramCrush >= 7) crushCompensation --;

      // crop - a basic effect to chop off the end of each sample for a more staccato feel
      paramCrop = storedValues[CROP];
      sample0.setEnd(thisDirection ? map(paramCrop,0,255,100,sample0_NUM_CELLS) : sample0_NUM_CELLS);
      sample1.setEnd(thisDirection ? map(paramCrop,0,255,100,sample1_NUM_CELLS) : sample1_NUM_CELLS);
      sample2.setEnd(thisDirection ? map(paramCrop,0,255,100,sample2_NUM_CELLS) : sample2_NUM_CELLS);
      sample3.setEnd(thisDirection ? map(paramCrop,0,255,100,sample3_NUM_CELLS) : sample3_NUM_CELLS);
      sample4.setEnd(thisDirection ? map(paramCrop,0,255,100,sample4_NUM_CELLS) : sample4_NUM_CELLS);
      sample0.setStart(!thisDirection ? map(paramCrop,255,0,100,sample0_NUM_CELLS) : 0);
      sample1.setStart(!thisDirection ? map(paramCrop,255,0,100,sample1_NUM_CELLS) : 0);
      sample2.setStart(!thisDirection ? map(paramCrop,255,0,100,sample2_NUM_CELLS) : 0);
      sample3.setStart(!thisDirection ? map(paramCrop,255,0,100,sample3_NUM_CELLS) : 0);
      sample4.setStart(!thisDirection ? map(paramCrop,255,0,100,sample4_NUM_CELLS) : 0);
      
      //paramDrop = storedValues[DROP] >> 5; // range of 0 to 7
      paramDrop = map(storedValues[DROP],0,256,0,9);
    }
    break;

    case 2:
    // using values of 305 and 205 (i.e. 255±50) to give a decent "dead zone" in the middle of the knob
    oscilGain2 = constrain(2*storedValues[DRONE]-305, 0, 255);
    if(storedValues[DRONE] < 128) {
      oscilGain1 = constrain(205-2*storedValues[DRONE], 0, 255);
    } else {
      oscilGain1 = oscilGain2;
    }
    // do same thing for drone modulation gains
    if(storedValues[DRONE_MOD] < 128) {
      droneMod2Active = false;
      paramDroneMod = constrain(205-2*storedValues[DRONE_MOD], 0, 255);
    } else {
      droneMod2Active = true;
      paramDroneMod = constrain(2*storedValues[DRONE_MOD]-305, 0, 255);;
    }
    paramDroneRoot = storedValues[DRONE_ROOT]/20;
    if(paramDroneRoot != previousDroneRoot) {
      if(!newStateLoaded && !newMidiDroneRoot) specialLedDisplay(paramDroneRoot, false);
      previousDroneRoot = paramDroneRoot;
    }
    if(!beatPlaying||newStateLoaded||newMidiDroneRoot) activeDroneRoot = paramDroneRoot;
    paramDronePitch = rootNotes[activeDroneRoot] * (0.5f + (float) storedValues[DRONE_PITCH]/170.0f);// * 0.768f + 65.41f;
    droneOscillator1.setFreq(paramDronePitch);
    droneOscillator2.setFreq(paramDronePitch*1.5f);
    break;

    case 3:
    paramBeat = (NUM_BEATS * (int) storedValues[BEAT]) / 256;
    if(paramBeat != previousBeat) {
      if(!newStateLoaded) specialLedDisplay(paramBeat, false); // display current beat number using LEDs
      previousBeat = paramBeat;
    }
    paramTempo = byteToTempo(storedValues[TEMPO]);
    paramTimeSignature = map(storedValues[TIME_SIGNATURE],0,256,1,MAX_BEATS_PER_BAR+2);
    if(paramTimeSignature != previousTimeSignature) {
      if(!newStateLoaded) {
        if(paramTimeSignature==MAX_BEATS_PER_BAR+1) specialLedDisplay(B11111111, true); // display current time signature using LEDs
        else specialLedDisplay(paramTimeSignature - 1, false); // display current time signature using LEDs
      }
      previousTimeSignature = paramTimeSignature;
    }
    paramSwing = storedValues[SWING] / 86; // gives range of 0 to 2
    break;
  }
}

void startBeat() {
  beatPlaying = true;
  pulseNum = 0;
  stepNum = 0;
  nextPulseTime = millis();
  Serial.write(0xFA); // MIDI clock start
}

void stopBeat() {
  beatPlaying = false;
  Serial.write(0xFC); // MIDI clock stop
  syncReceived = false;
}

void playPulseHits() {
  for(byte i=0; i<5; i++) {
    if(bitRead(dropRef[i],paramDrop)) calculateNote(i);
  }
}

void calculateNote(byte sampleNum) {
  
  long thisVelocity = 0;
  // first, check for notes which are defined in the beat
  unsigned int effectiveStepNum = stepNum;
  if(paramSwing==1) {
    if(stepNum%12==6) effectiveStepNum = stepNum - 1; // arbitrary...
    if(stepNum%12==7) effectiveStepNum = stepNum - 1;
  } else if(paramSwing==2) {
    if(stepNum%12==6) effectiveStepNum = stepNum - 1; // arbitrary...
    if(stepNum%12==8) effectiveStepNum = stepNum - 2;
  }
  effectiveStepNum = (specialOffset + effectiveStepNum) % 192;
  if(effectiveStepNum%6==0) {
    // beats only defined down to 16th notes not 32nd, hence %2 (CHANGE COMMENT)
    byte beatByte = pgm_read_byte(&beats[paramBeat][sampleNum][effectiveStepNum/48]); // 48...? was 16
    if(bitRead(beatByte,7-((effectiveStepNum/6)%8))) thisVelocity = 255;
  }
  byte testZoomMult = getZoomMultiplier();
  byte yesNoRand = rand(0,256);
  long randomVelocity = 0;
  if(yesNoRand < paramChance) {
    int lowerBound = paramMidpoint - paramRange/2;
    int upperBound = paramMidpoint + paramRange/2;
    randomVelocity = rand(lowerBound, upperBound);
    randomVelocity = constrain(randomVelocity,-255,255);
    if(testZoomMult < 42) {
      randomVelocity = randomVelocity * 6 * testZoomMult / 255;
    }
  }
  thisVelocity += randomVelocity;
  thisVelocity = constrain(thisVelocity, 0, 255);
  triggerNote(sampleNum, thisVelocity);
}

// zoom stuff is hard to explain
byte getZoomMultiplier() {
  byte zoomValueIndex = paramZoom / 43; // gives value from 0 to 6, denoting which zoom region we are in
  byte zoomVelocity = paramZoom % 43; // gives value from 0 to 42 - how much velocity the "zoomed" note will have
  byte returnVal = 0;
  switch(zoomValueIndex) {
    case 99:
    if(stepNum%96==0) returnVal = zoomVelocity;
    break;
    case 0:
    if(stepNum%96==0) returnVal = 42;
    else if(stepNum%48==0) returnVal = zoomVelocity;
    break;
    case 1:
    if(stepNum%48==0) returnVal = 42;
    else if(stepNum%24==0) returnVal = zoomVelocity;
    break;
    case 2:
    if(stepNum%24==0) returnVal = 42;
    else if(stepNum%12==0) returnVal = zoomVelocity;
    break;
    case 3:
    // at this zoom level, the return value depends on the swing setting
    if(stepNum%12==0) returnVal = 42;
    else {
      if(paramSwing==0) {
        // straight
        if(stepNum%6==0) returnVal = zoomVelocity;
      } else if(paramSwing==1) {
        // partial swing
        if(stepNum%12==7) returnVal = zoomVelocity;
      } else if(paramSwing==2) {
        // full swing
        if(stepNum%12==8) returnVal = zoomVelocity;
      }
    }
    break;
    case 4:
    // at this zoom level, the return value depends on the swing setting
    if(paramSwing==0) {
      // straight
      if(stepNum%6==0) returnVal = 42;
      else if(stepNum%3==0) returnVal = zoomVelocity;
    } else if(paramSwing==1) {
      // partial swing
      if(stepNum%12==0||stepNum%12==7) returnVal = 42;
      else if(stepNum%12==5) returnVal = zoomVelocity;
    } else if(paramSwing==2) {
      // full swing
      if(stepNum%12==0||stepNum%12==8) returnVal = 42;
      else if(stepNum%12==4) returnVal = zoomVelocity;
    }
    break;
    case 5:
    // at this zoom level, the return value depends on the swing setting
    if(paramSwing==0) {
      // straight
      if(stepNum%3==0) returnVal = 42;
      else returnVal = zoomVelocity;
    } else if(paramSwing==1) {
      // partial swing
      if(stepNum%12==0||stepNum%12==5||stepNum%12==7) returnVal = 42;
      else returnVal = zoomVelocity;
    } else if(paramSwing==2) {
      // full swing
      if(stepNum%4==0) returnVal = 42;
      returnVal = zoomVelocity;
    }
    break;
  }
  return returnVal;
}

void triggerNote(byte sampleNum, byte velocity) {
  if(velocity>8) { // don't bother with very quiet notes
    switch(sampleNum) {
      case 0:
      sample0.start();
      break;
      case 1:
      sample1.start();
      break;
      case 2:
      sample2.start();
      break;
      case 3:
      sample3.start();
      break;
      case 4:
      sample4.start();
      break;
    }
    sampleVolumes[sampleNum] = velocity;
    bitWrite(noteDown,sampleNum,true);
    playMidiNote(sampleNum, velocity);
  }
}

void playMidiNote(byte sampleNum, byte velocity) {
  Serial.write(midiNoteCommands[sampleNum]); // note down command
  Serial.write(midiNotes[sampleNum]); // note number
  Serial.write(velocity>>1); // velocity (scaled down to MIDI standard, 0-127)
}

void incrementPulse() {
  pulseNum ++;
  if(pulseNum == 24) {
    pulseNum = 0;
  }
  stepNum ++;
  if(stepNum == numSteps) {
    stepNum = 0;
  }
}

void cancelMidiNotes() {
  byte i;
  for(i=0; i<NUM_SAMPLES; i++) {
    if(bitRead(noteDown,i)) {
      Serial.write(midiNoteCommands[i]); // note down command (zero velocity is equivalent to note up)
      Serial.write(midiNotes[i]); // note number
      Serial.write(0x00); // zero velocity
      bitWrite(noteDown,i,false);
    }
  }
}

const byte atten = 9;
char d1Next, d2Next;
int updateAudio(){
  d1Next = droneOscillator1.next();
  d2Next = droneOscillator2.next();
  char droneSig = ((oscilGain1*d1Next)>>10)+((oscilGain2*d2Next)>>10);
  char droneModSig = (d1Next>>2)+(droneMod2Active?(d2Next>>2):0);
  char asig = ((sampleVolumes[0]*sample0.next())>>atten)+
    ((sampleVolumes[1]*sample1.next())>>atten)+
    ((sampleVolumes[2]*sample2.next())>>atten)+
    ((sampleVolumes[3]*sample3.next())>>atten)+
    ((sampleVolumes[4]*sample4.next())>>atten)+
    (droneSig>>2);
  asig = (asig * ((255-paramDroneMod)+((paramDroneMod*droneModSig)>>6)))>>8;
  asig = (asig>>paramCrush)<<crushCompensation;
  return asig;
}

byte thisMidiByte;
byte midiBytes[3];
byte currentMidiByte = 0;
void loop(){
  while(Serial.available()) {
    thisMidiByte = Serial.read();
    if(bitRead(thisMidiByte,7)) {
      // status byte
      midiBytes[0] = thisMidiByte;
      if(thisMidiByte==0xFA) {
        // clock start signal received
        startBeat();
      } else if(thisMidiByte==0xFB) {
        // clock continue signal received
        startBeat();
      } else if(thisMidiByte==0xFC) {
        // clock stop signal received
        stopBeat();
      } else if(thisMidiByte==0xF8) {
        // clock signal received
        syncReceived = true;
        if(beatPlaying) {
          doPulseActions();
        }
      }
      currentMidiByte = 1;
    } else {
      // data byte
      midiBytes[currentMidiByte] = thisMidiByte;
      if((midiBytes[0]>>4)==0xB && currentMidiByte == 2) {
        // CC message (any channel)
        if(midiBytes[1]>=16&&midiBytes[1]<32) {
          byte paramNum = midiBytes[1] - 16;
          byte paramSet = paramNum / 4;
          byte paramSetNum = paramNum % 4;
          storedValues[paramNum] = midiBytes[2] * 2;
          if(controlSet==paramSet) {
            bitWrite(knobLocked, paramSetNum, true);
            initValues[paramSetNum] = analogValues[paramSetNum];
          }
          updateParameters(paramSet);
        }
      } else if((midiBytes[0]>>4)==0x9 && currentMidiByte == 2) {
        // Note on (any channel)
        if(midiBytes[2] > 0) {
          newMidiDroneRoot = true;
          byte paramSet = DRONE_ROOT / 4;
          byte paramSetNum = DRONE_ROOT % 4;
          storedValues[DRONE_ROOT] = (midiBytes[1] % 12) * 20;
          if(controlSet==paramSet) {
            bitWrite(knobLocked, paramSetNum, true);
            initValues[paramSetNum] = analogValues[paramSetNum];
          }
          updateParameters(paramSet);
          newMidiDroneRoot = false;
        }
      }
      // MIDI thru, experimental, only certain message types
      bool useMidiThru = false;
      if(useMidiThru) {
        if(currentMidiByte == 2) {
          switch(midiBytes[0]>>4) {
            case 0xB:
            // CC
            case 0x9:
            // note on
            case 0x8:
            // note off
            Serial.write(midiBytes[0]);           
            Serial.write(midiBytes[1]);
            Serial.write(midiBytes[2]);
            break;
          }
        }
      }
      currentMidiByte ++;
    }
  }
  audioHook(); // main Mozzi function, calls updateAudio and updateControl
}

void flashLeds() {
  byte i, randByte;
  for(i=0;i<30;i++) {
    randByte = rand(0,256);
    displayLedNum(randByte);
    while(millis()<(i+1)*25) {
      // do nothing here
      // this is a silly hack so that the delay() function doesn't get compiled
      // this is what happens when your program takes up 99%+ of the storage space
    }
  }
  displayLedNum(0);
}

void updateLeds() {
  if(millis() < specialLedDisplayTime + 750UL && specialLedDisplayTime > 0) {
    // show desired binary number for certain parameters where visual feedback is helpful
    displayLedNum(specialLedDisplayNum);
  } else if(beatPlaying && menuState == 0) {
    //if(stepNum==0||(paramTimeSignature==4&&stepNum==96)||(paramTimeSignature==6&&stepNum==72)) displayLedNum(B00000011);
    if(stepNum==0) displayLedNum(B00000011);
    else if(stepNum%24==0) displayLedNum(B00000010);
    else if(stepNum%24==3) displayLedNum(B00000000);
  } else {
    displayLedNum(B00000000);
  }
}

void displayLedNum(byte displayNum) {
  byte i;
  for(i=0;i<NUM_LEDS;i++) {
    digitalWrite(ledPins[i], bitRead(displayNum,i));
  }
}

void specialLedDisplay(byte displayNum, bool isBinary) {
  if(!firstLoop&&!secondLoop) {
    if(isBinary) {
      specialLedDisplayNum = displayNum;
    } else {
      specialLedDisplayNum = B00000000;
      bitWrite(specialLedDisplayNum,displayNum%NUM_LEDS,HIGH);
    }
    specialLedDisplayTime = millis();
  }
}

float byteToTempo(byte tempoByte) {
  float tempoFloat;
  if(tempoByte<=192) {
    tempoFloat = 10.0 + tempoByte;
  } else {
    tempoFloat = 202.0 + 12.66667 * (tempoByte - 192.0);
  }
  return tempoFloat;
}

byte tempoToByte(float tempoFloat) {
  byte tempoByte;
  if(tempoFloat<=202.0) {
    tempoByte = ((byte) tempoFloat) - 10;
  } else {
    tempoByte = (byte) ((tempoFloat - 202.0) / 12.66667) + 192;
  }
  return tempoByte;
}

void chooseBank(byte newBank) {
  activeBank = newBank;
  readyToChooseBank = false;
}

void resetSessionToDefaults() {
  // set starting values for each parameter
  storedValues[CHANCE] = 128;
  storedValues[ZOOM] = 150;
  storedValues[RANGE] = 0;
  storedValues[MIDPOINT] = 128;
  
  storedValues[PITCH] = 160;
  storedValues[CRUSH] = 255;
  storedValues[CROP] = 255;
  storedValues[DROP] = 128;

  storedValues[DRONE_MOD] = 127;
  storedValues[DRONE] = 127;
  storedValues[DRONE_ROOT] = 0;
  storedValues[DRONE_PITCH] = 127;

  storedValues[BEAT] = 27;
  storedValues[TIME_SIGNATURE] = 64; // equates to 4/4
  storedValues[SWING] = 0;
  storedValues[TEMPO] = 110; // actually equates to 120BPM

  newStateLoaded = true;
  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    updateParameters(i); // set parameters to initial values defined above
  }
}

void createRandomSession() {
  storedValues[CHANCE] = rand(64,192);
  storedValues[ZOOM] = rand(100,190);
  storedValues[RANGE] = rand(0,128);
  storedValues[MIDPOINT] = rand(80,190);
  
  storedValues[PITCH] = rand(135,256);
  if(rand(0,6)==0) storedValues[PITCH] = rand(0,120);
  storedValues[CRUSH] = rand(160,256);
  storedValues[CROP] = rand(128,256);
  storedValues[DROP] = 128;
  if(rand(0,3)==0) storedValues[DROP] = rand(29,226);

  storedValues[DRONE_MOD] = 127;
  storedValues[DRONE] = 127;
  storedValues[DRONE_ROOT] = rand(0,256);
  storedValues[DRONE_PITCH] = 127;
  if(rand(0,6)==0) storedValues[DRONE_MOD] = rand(0,256);
  if(rand(0,4)==0) storedValues[DRONE] = rand(64,192);

  storedValues[BEAT] = rand(0,240);

  if(!beatPlaying) {
    // don't randomise time signature and tempo unless stopped
    storedValues[TIME_SIGNATURE] = 64; // equates to 4/4
    if(rand(0,3)==0) storedValues[TIME_SIGNATURE] = rand(0,256);
    storedValues[TEMPO] = rand(70,160);
    if(rand(0,4)==0) storedValues[TEMPO] = rand(0,256);
  }
  storedValues[SWING] = rand(0,256);

  newStateLoaded = true;
  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    updateParameters(i); // set parameters to initial values defined above
  }
}

void loadParams(byte slotNum) {
  readyToChooseLoadSlot = false;
  newStateLoaded = true;
  for(byte i=0; i<NUM_PARAM_GROUPS*NUM_KNOBS; i++) {
    if(!(i==TEMPO&&beatPlaying)) {
      byte thisValue = EEPROM.read((slotNum+6*activeBank)*SAVED_STATE_SLOT_BYTES+i);
      storedValues[i] = thisValue;
    }
  }
  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    updateParameters(i);
  }
}

void saveParams(byte slotNum) {
  // 1024 possible byte locations in EEPROM
  // currently need 16 bytes per saved state (4 groups of 4 params, 1 byte per param)
  // 36 possible save slots (6 banks of 6 slots)
  // need to leave space for some possible extra saved state data in future
  // allot 24 bytes per saved state (vaguely round number in binary), taking up total of 864 bytes
  readyToChooseSaveSlot = false;
  for(byte i=0; i<NUM_PARAM_GROUPS*NUM_KNOBS; i++) {
    EEPROM.write((slotNum+6*activeBank)*SAVED_STATE_SLOT_BYTES+i, storedValues[i]);
  }
}

// prior to firmware version 1.1, no way of knowing whether older firmware was previously installed
// from V1.1 onwards, new special address beyond the 864 bytes reserved for storing sessions
// Slot 864-865: if these bytes have values 119 and 206 respectively, we are using the new memory system (1 in 65,025 chance of these being randomly correct)
// Slot 866: current version (0 = V1.1, 1 = V1.2)
// Slots 867-871: reserved for other version stuff if needed
// Slots 872-879: MIDI note numbers for channels 1 through 5 (and another 3 reserved)
// Slots 880-887: MIDI channel numbers for channels 1 through 5 (and another 3 reserved)
// Slots 888+: free for future features

void checkEepromScheme() {
  byte i;
  byte check1 = EEPROM.read(864);
  byte check2 = EEPROM.read(865);
  if(check1==119&&check2==206) {
    // all good, already using the new EEPROM scheme

    // update firmware version in memory if needed
    if(EEPROM.read(866) != 1) EEPROM.write(866, 1);
  } else {
    // prepare EEPROM for new scheme
    EEPROM.write(864, 119);
    EEPROM.write(865, 206);
    EEPROM.write(866, 1);
    // write default MIDI note/channel settings
    for(i=0; i<NUM_SAMPLES; i++) {
      EEPROM.write(872+i, defaultMidiNotes[i]);
      EEPROM.write(880+i, 9); // channel 10 (actually 9 because zero-indexed) is default channel to use
    }
  }
}

void initialiseSettings(bool isReset) {
  // must run checkEepromScheme() BEFORE this function
  byte i;
  for(i=0; i<NUM_SAMPLES; i++) {
    if(isReset) {
      EEPROM.write(872+i, defaultMidiNotes[i]);
      EEPROM.write(880+i, 9);
    }
    midiNotes[i] = EEPROM.read(872+i);
    midiNoteCommands[i] = 0x90 + EEPROM.read(880+i);
  }
}

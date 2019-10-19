#define DEBUGGING false
#define BREADBOARD true // switch to false if compiling code for PCB

// when in debugging mode you can see the current memory usage
// you'll need to download and install this library from https://github.com/McNeight/MemoryFree
#if DEBUGGING
#include <MemoryFree.h>
#endif

// include custom Mozzi library files - copied from a specific version of Mozzi and tweaked to give extra functionality
// original Mozzi library found at https://github.com/sensorium/Mozzi
#include "MozziDK/MozziGuts.h"
#include "MozziDK/Sample.h"
#include "MozziDK/EventDelay.h"
#include "MozziDK/mozzi_rand.h"
#include "MozziDK/mozzi_fixmath.h"
#include "MozziDK/Oscil.h"
#include "MozziDK/tables/saw256_int8.h"

// include debouncing library
#include <Bounce2.h>

// include EEPROM library for saving data
#include <EEPROM.h>

// include/initialise tap tempo library - copied into repo for now because I added some functions
// original ArduinoTapTempo library found at https://github.com/dxinteractive/ArduinoTapTempo
#include "ArduinoTapTempoDK/src/ArduinoTapTempo.cpp"
ArduinoTapTempo tapTempo;

// include audio data files
#include "kick.h"
#include "closedhat.h"
#include "snare.h"
#include "rim.h"
#include "tom.h"

// define pins
byte breadboardLedPins[5] = {5,6,7,8,13};
byte breadboardButtonPins[6] = {2,3,4,10,11,12};
byte breadboardAnalogPins[4] = {0,1,2,3};
byte pcbLedPins[5] = {6,5,4,3,2};
byte pcbButtonPins[6] = {13,12,11,10,8,7};
byte pcbAnalogPins[4] = {3,2,1,0};
byte (&ledPins)[5] = BREADBOARD ? breadboardLedPins : pcbLedPins;
byte (&buttonPins)[6] = BREADBOARD ? breadboardButtonPins : pcbButtonPins;

// define buttons
Bounce buttonA = Bounce();
Bounce buttonB = Bounce();
Bounce buttonC = Bounce();
Bounce buttonX = Bounce();
Bounce buttonY = Bounce();
Bounce buttonZ = Bounce();

#define CONTROL_RATE 256 // tweak this value if performance is bad, must be power of 2 (64, 128, etc)
#define NUM_BEATS 3
#define MAX_BEAT_STEPS 32
#define NUM_PARAM_GROUPS 5
#define NUM_KNOBS 4
#define NUM_LEDS 5
#define NUM_BUTTONS 6
#define NUM_SAMPLES_TOTAL 5 // total number of samples including those only triggered by chance
#define NUM_SAMPLES_DEFINED 5 // number of samples defined in the preset drumbeats (kick, hat, snare, rim, tom)
#define SAVED_STATE_SLOT_BYTES 32
#define MIN_TEMPO 40
#define MAX_TEMPO 295

#if MIN_TEMPO + 255 != MAX_TEMPO
#error "Tempo must have a range of exactly 255 otherwise I'll have to write more code"
#endif

bool sequencePlaying = false; // false when stopped, true when playing
byte numSteps;
bool beatLedsActive = true;
bool firstLoop = true;
bool secondLoop = false;
byte sampleVolumes[NUM_SAMPLES_TOTAL];
bool noteDown[NUM_SAMPLES_TOTAL];
bool readyToSave = true;
bool readyToChooseSaveSlot = false;
bool readyToLoad = true;
bool readyToChooseLoadSlot = false;
bool newStateLoaded = false;

// variables relating to knob values
byte controlSet = 0;
bool knobLocked[NUM_KNOBS] = {true,true,true,true};
byte analogValues[NUM_KNOBS] = {0,0,0,0};
byte initValues[NUM_KNOBS] = {0,0,0,0};

// parameters
byte paramChance;
byte paramMidpoint;
byte paramRange;
byte paramZoom;
byte paramPitch;
byte paramCrush;
byte paramCrop;
unsigned int paramGlitch;
byte crushCompensation;
int paramSlop;
byte paramSwing;
byte paramDelayMix;
unsigned int paramDelayTime;
byte paramBeat;
byte paramTimeSignature;
byte paramDrift;
byte paramDrop;
byte oscilGain1;
byte oscilGain2;
float paramDronePitch;
byte paramDroneMod;
bool droneMod2Active = false;
#define PARAM_CHANCE 0
#define PARAM_ZOOM 1
#define PARAM_MIDPOINT 2
#define PARAM_RANGE 3
#define PARAM_PITCH 4
#define PARAM_CRUSH 5
#define PARAM_CROP 6
#define PARAM_GLITCH 7
#define PARAM_SLOP 8
#define PARAM_SWING 9
#define PARAM_DELAY_TIME 10
#define PARAM_DELAY_MIX 11
#define PARAM_BEAT 12
#define PARAM_TEMPO 13
#define PARAM_TIME_SIGNATURE 14
#define PARAM_DRIFT 15
#define PARAM_DROP 16
#define PARAM_DRONE_MOD 17
#define PARAM_DRONE 18
#define PARAM_DRONE_PITCH 19
byte storedValues[NUM_PARAM_GROUPS*NUM_KNOBS];

// define samples
Sample <kick_NUM_CELLS, AUDIO_RATE> kick(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare(snare_DATA);
Sample <rim_NUM_CELLS, AUDIO_RATE> rim(rim_DATA);
Sample <tom_NUM_CELLS, AUDIO_RATE> tom(tom_DATA);

// define oscillators
Oscil<SAW256_NUM_CELLS, AUDIO_RATE> droneOscillator1(SAW256_DATA);
Oscil<SAW256_NUM_CELLS, AUDIO_RATE> droneOscillator2(SAW256_DATA);

// could just use bools instead of bytes to save space
// order of samples: kick, hat, snare, rim, tom
// USE PROGMEM LATER TO REDUCE SIZE
const byte beats[NUM_BEATS][NUM_SAMPLES_DEFINED][MAX_BEAT_STEPS] = {
  {
    {255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,},
    {255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,},
    {0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
  },
  {
    {255,0,0,0,0,0,0,0,0,0,255,0,0,0,0,0,255,0,0,0,0,0,0,0,0,0,255,0,0,0,0,0,},
    {255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,},
    {0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
  },
  {
    {255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,},
    {0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
    {0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,},
  },
};  

float nextPulseTime = 0;
int pulseNum = 0;

void setup() {
  startMozzi(CONTROL_RATE);
  randSeed();
  #if DEBUGGING
  Serial.begin(9600);
  #else
  Serial.begin(31250);
  #endif
  for(byte i=0;i<NUM_LEDS;i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  buttonA.interval(25);
  buttonB.interval(25);
  buttonC.interval(25);
  buttonX.interval(25);
  buttonY.interval(25);
  buttonZ.interval(25);
  buttonA.attach(buttonPins[0], INPUT_PULLUP);
  buttonB.attach(buttonPins[1], INPUT_PULLUP);
  buttonC.attach(buttonPins[2], INPUT_PULLUP);
  buttonX.attach(buttonPins[3], INPUT_PULLUP);
  buttonY.attach(buttonPins[4], INPUT_PULLUP);
  buttonZ.attach(buttonPins[5], INPUT_PULLUP);

  // prevent samples from playing on startup
  for(int i=0;i<20000;i++) {
    kick.next();
    closedhat.next();
    snare.next();
    rim.next();
    tom.next();
  }

  // add more default values here
  storedValues[PARAM_CHANCE] = 0;
  storedValues[PARAM_ZOOM] = 150;
  storedValues[PARAM_MIDPOINT] = 128;
  storedValues[PARAM_RANGE] = 255;
  storedValues[PARAM_CRUSH] = 255;
  storedValues[PARAM_PITCH] = 160;
  storedValues[PARAM_TIME_SIGNATURE] = 120; // this means 4/4, it's confusing...
  storedValues[PARAM_TEMPO] = 100 - MIN_TEMPO; // not BPM!
  storedValues[PARAM_DRONE] = 128;
  storedValues[PARAM_DRONE_PITCH] = 128;
  storedValues[PARAM_DRONE_MOD] = 128;
  storedValues[PARAM_CROP] = 255;

  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    //updateParameters(i);
  }

  tapTempo.setMinBPM((float) MIN_TEMPO);
  tapTempo.setMaxBPM((float) MAX_TEMPO);

  startupLedSequence();
}

void loop() {
  audioHook();
}

void toggleSequence() {
  if(sequencePlaying) stopSequence();
  else startSequence();
  digitalWrite(ledPins[1],sequencePlaying);
}

void startSequence() {
  Serial.write(0xFA); // MIDI clock start
  sequencePlaying = true;
  nextPulseTime = millis();
  pulseNum = 0;
}

void stopSequence() {
  Serial.write(0xFC); // MIDI clock stop
  sequencePlaying = false;
}

void updateControl() {

  bool controlSetChanged = false;
  newStateLoaded = false;
  
  buttonA.update();
  buttonB.update();
  buttonC.update();
  buttonX.update();
  buttonY.update();
  buttonZ.update();

  if(!buttonB.read() && !buttonC.read()) {
    readyToLoad = false;
    readyToSave = false;
    readyToChooseLoadSlot = true;
    readyToChooseSaveSlot = false;
    tapTempo.update(false);
  } else if(!buttonX.read() && !buttonY.read()) {
    readyToSave = false;
    readyToLoad = false;
    readyToChooseSaveSlot = true;
    readyToChooseLoadSlot = false;
    tapTempo.update(false);
  } else {
    readyToSave = true;
    readyToLoad = true;
    if(!readyToChooseLoadSlot&&!readyToChooseSaveSlot) {
      tapTempo.update(!buttonA.read());
    } else {
      tapTempo.update(false);
    }

    // handle button presses
    byte prevControlSet = controlSet;
    if(buttonA.fell()) {
      if(readyToChooseLoadSlot) loadParams(0);
      else if(readyToChooseSaveSlot) saveParams(0);
      else {
        controlSet = 0;
        storedValues[PARAM_TEMPO] = tapTempo.getBPM() - MIN_TEMPO;
      }
    } else if(buttonB.fell()) {
      if(readyToChooseLoadSlot) loadParams(1);
      else if(readyToChooseSaveSlot) saveParams(1);
      else controlSet = 1;
    } else if(buttonC.fell()) {
      if(readyToChooseLoadSlot) loadParams(2);
      else if(readyToChooseSaveSlot) saveParams(2);
      else controlSet = 2;
    } else if(buttonX.fell()) {
      if(readyToChooseLoadSlot) loadParams(3);
      else if(readyToChooseSaveSlot) saveParams(3);
      else controlSet = 3;
    } else if(buttonY.fell()) {
      if(readyToChooseLoadSlot) loadParams(4);
      else if(readyToChooseSaveSlot) saveParams(4);
      else controlSet = 4;
    }
    controlSetChanged = (prevControlSet != controlSet);
    
    if(buttonZ.fell()) {
      digitalWrite(ledPins[0],HIGH);
      if(readyToChooseLoadSlot) loadParams(5);
      else if(readyToChooseSaveSlot) saveParams(5);
      else toggleSequence();
    }

    // messing everything up to make a glitchy effect
    if(paramGlitch>0) {
      byte minDelay = paramGlitch/4;
      byte maxDelay = (paramGlitch*paramGlitch)>>6;
      delay(rand(minDelay, maxDelay));
    }
  }
  
  if(sequencePlaying) {
    byte tempoReading = mozziAnalogRead(breadboardAnalogPins[0])>>2;
    tapTempo.setBPM((float) MIN_TEMPO + ((float) tempoReading));
    float msPerPulse = tapTempo.getBeatLength() / 24.0;
    if(millis()>=nextPulseTime) {
      if(pulseNum==0) playTestNote();
      Serial.write(0xF8);
      if(pulseNum%3==0) triggerNotes();
      if(pulseNum%24==0) digitalWrite(ledPins[3],HIGH);
      else digitalWrite(ledPins[3],LOW);
      nextPulseTime = nextPulseTime + msPerPulse;
      pulseNum = (pulseNum + 1) % 24;
    }
  }
}

void triggerNotes() {
  byte i;
  for(i=0; i<NUM_SAMPLES_TOTAL; i++) {
    if(beats[0][i][pulseNum/3]>0) playMidiNote(40+5*i);
  }
}

int updateAudio() {
  return 0;
}

void startupLedSequence() {
  byte seq[15] = {0,1,2,3,4,3,2,1,0,1,2,3,4,3,2};
  for(byte i=0; i<15; i++) {
    digitalWrite(ledPins[seq[i]], HIGH);
    delay(100);
    digitalWrite(ledPins[seq[i]], LOW);
  }
}

void playMidiNote(byte noteNum) {
  Serial.write(0x90);
  Serial.write(noteNum);
  Serial.write(0x00);
  Serial.write(0x90);
  Serial.write(noteNum);
  Serial.write(100);
}

void playTestNote() {
  Serial.write(0x90);
  Serial.write(50);
  Serial.write(0x00);
  Serial.write(0x90);
  Serial.write(50);
  Serial.write(100);
}

void saveParams(byte slotNum) {
  // 1024 possible byte locations
  // currently need 20 bytes per saved state
  // currently planning choice of 6 slots for saved states (1 per button)
  // need to leave space for some possible extra saved state data
  // allot 32 bytes per saved state (nice round number), taking up total of 192 bytes
  readyToChooseSaveSlot = false;
  for(byte i=0; i<NUM_PARAM_GROUPS*NUM_KNOBS; i++) {
    EEPROM.write(slotNum*SAVED_STATE_SLOT_BYTES+i, storedValues[i]);
  }
}

void loadParams(byte slotNum) {
  readyToChooseLoadSlot = false;
  newStateLoaded = true;
  for(byte i=0; i<NUM_PARAM_GROUPS*NUM_KNOBS; i++) {
    byte thisValue = EEPROM.read(slotNum*SAVED_STATE_SLOT_BYTES+i);
    storedValues[i] = thisValue;
  }
  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    //updateParameters(i);
  }
}

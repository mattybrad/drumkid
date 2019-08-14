#define IS_BREADBOARD true // switch to false if compiling code for PCB

// include custom mozzi library files - copied from a specific version of mozzi and tweaked to give extra functionality
#include "MozziDK/MozziGuts.h"
#include "MozziDK/Sample.h"
#include "MozziDK/EventDelay.h"
#include "MozziDK/mozzi_rand.h"
#include "MozziDK/mozzi_fixmath.h"
#include "MozziDK/Oscil.h"
#include "MozziDK/tables/square_analogue512_int8.h"

// include debouncing library
#include <Bounce2.h>

// include EEPROM library for saving data
#include <EEPROM.h>

// include/initialise tap tempo library
#include <ArduinoTapTempo.h>
ArduinoTapTempo tapTempo;

// include audio data files
#include "kick.h"
#include "closedhat.h"
#include "snare.h"
#include "rim.h"
#include "clap.h"

// define pins
byte breadboardLedPins[5] = {5,6,7,8,13};
byte breadboardButtonPins[6] = {2,3,4,10,11,12};
byte pcbLedPins[5] = {6,5,4,3,2};
byte pcbButtonPins[6] = {13,12,11,10,8,7};
byte (&ledPins)[5] = IS_BREADBOARD ? breadboardLedPins : pcbLedPins;
byte (&buttonPins)[6] = IS_BREADBOARD ? breadboardButtonPins : pcbButtonPins;

// define buttons
Bounce buttonA = Bounce();
Bounce buttonB = Bounce();
Bounce buttonC = Bounce();
Bounce buttonX = Bounce();
Bounce buttonY = Bounce();
Bounce buttonZ = Bounce();

#define CONTROL_RATE 256 // tweak this value if performance is bad, must be power of 2 (64, 128, etc)
#define MAX_BEAT_STEPS 32
#define NUM_PARAM_GROUPS 5
#define NUM_KNOBS 4
#define NUM_LEDS 5
#define NUM_BUTTONS 6
#define NUM_SAMPLES_TOTAL 5 // total number of samples including those only triggered by chance
#define NUM_SAMPLES_DEFINED 5 // number of samples defined in the preset drumbeats (kick, hat, snare, rim, clap)
#define SAVED_STATE_SLOT_BYTES 32
#define MIN_TEMPO 40 // and max tempo is 40+255 (295) for nice easy byte maths

bool sequencePlaying = false; // false when stopped, true when playing
byte currentStep;
byte numSteps = 4 * 16; // 64 steps = 4/4 time signature, 48 = 3/4 or 6/8, 80 = 5/4, 112 = 7/8 etc
float nextNoteTime;
float scheduleAheadTime = 100; // ms
float lookahead = 25; // ms
const byte numEventDelays = 20;
EventDelay schedulerEventDelay;
EventDelay eventDelays[numEventDelays];
bool delayInUse[numEventDelays];
byte delayChannel[numEventDelays];
byte delayVelocity[numEventDelays];
byte eventDelayIndex = 0;
bool beatLedsActive = true;
bool firstLoop = true;
byte sampleVolumes[NUM_SAMPLES_TOTAL];
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
byte paramChance = 0;
byte paramMidpoint = 0;
byte paramRange = 0;
byte paramZoom = 0;
byte paramPitch = 0;
byte paramCrush = 0;
byte paramCrop = 0;
unsigned int paramGlitch = 0;
byte crushCompensation = 0;
int paramSlop = 0;
byte paramBeat = 0;
float paramTempo = 120.0;
byte paramTimeSignature = 4;
byte oscilGain1 = 0;
byte oscilGain2 = 0;
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
#define PARAM_BLEND 9
#define PARAM_10 10
#define PARAM_11 11
#define PARAM_BEAT 12
#define PARAM_TEMPO 13
#define PARAM_TIME_SIGNATURE 14
#define PARAM_15 15
#define PARAM_16 16
#define PARAM_DRONE_MOD 17
#define PARAM_DRONE 18
#define PARAM_DRONE_PITCH 19
byte storedValues[NUM_PARAM_GROUPS*NUM_KNOBS];

// define samples
Sample <kick_NUM_CELLS, AUDIO_RATE> kick(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare(snare_DATA);
Sample <rim_NUM_CELLS, AUDIO_RATE> rim(rim_DATA);
Sample <clap_NUM_CELLS, AUDIO_RATE> clap(clap_DATA);

// define oscillators
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> droneOscillator1(SQUARE_ANALOGUE512_DATA);
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> droneOscillator2(SQUARE_ANALOGUE512_DATA);

// could just use bools instead of bytes to save space
// order of samples: kick, hat, snare
const byte beat1[NUM_SAMPLES_DEFINED][MAX_BEAT_STEPS] PROGMEM = {
  {255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,},
  {255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,},
  {0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
};
const byte beat2[NUM_SAMPLES_DEFINED][MAX_BEAT_STEPS] PROGMEM = {
  {255,0,0,0,0,0,0,0,0,0,255,0,0,0,0,0,255,0,0,0,0,0,0,0,0,0,255,0,0,0,0,0,},
  {255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,},
  {0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
};
const byte beat3[NUM_SAMPLES_DEFINED][MAX_BEAT_STEPS] PROGMEM = {
  {255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,},
  {255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},
  {0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,},
};           

void setup() {
  startMozzi(CONTROL_RATE);
  randSeed();
  kick.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  closedhat.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  snare.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  rim.setFreq((float) rim_SAMPLERATE / (float) rim_NUM_CELLS);
  clap.setFreq((float) clap_SAMPLERATE / (float) clap_NUM_CELLS);
  Serial.begin(9600);
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

  for(int i=0;i<20000;i++) {
    kick.next();
    closedhat.next();
    snare.next();
    rim.next();
    clap.next();
  }

  // add more default values here
  storedValues[PARAM_CRUSH] = 255;
  storedValues[PARAM_PITCH] = 160;
  storedValues[PARAM_TIME_SIGNATURE] = 120; // this means 4/4, it's confusing...
  storedValues[PARAM_TEMPO] = 100; // not BPM!
  storedValues[PARAM_DRONE] = 128;
  storedValues[PARAM_DRONE_PITCH] = 128;
  storedValues[PARAM_DRONE_MOD] = 128;
  storedValues[PARAM_CROP] = 255;

  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    updateParameters(i);
  }

  tapTempo.setMinBPM((float) MIN_TEMPO);
  tapTempo.setMaxBPM((float) (MIN_TEMPO+255));

  startupLedSequence();
}

void loop() {
  audioHook();
}

void nextNote() {
  float msPerBeat = (60.0 / paramTempo) * 1000.0;
  nextNoteTime += 0.25 * msPerBeat / 4;
  currentStep ++;
  currentStep = currentStep % numSteps;
}

void scheduleNote(byte channelNumber, byte beatNumber, byte velocity, float delayTime) {
  // schedule a drum hit to occur after [delayTime] milliseconds
  eventDelays[eventDelayIndex].set(delayTime);
  eventDelays[eventDelayIndex].start();
  delayInUse[eventDelayIndex] = true;
  delayChannel[eventDelayIndex] = channelNumber;
  delayVelocity[eventDelayIndex] = velocity;
  eventDelayIndex = (eventDelayIndex + 1) % numEventDelays; // replace with "find next free slot" function
}

void scheduler() {
  byte thisBeatVelocity;
  while(nextNoteTime < (float) millis() + scheduleAheadTime) {
    byte thisStep = currentStep/16;
    byte stepLED = constrain(thisStep, 0, 4);
    if(beatLedsActive) {
      if(currentStep%16==0) digitalWrite(ledPins[stepLED], HIGH);
      else if(currentStep%4==0) digitalWrite(ledPins[stepLED], LOW);
    }
    for(byte i=0;i<NUM_SAMPLES_TOTAL;i++) {
      int slopRand = rand(0,paramSlop) - paramSlop / 2;
      thisBeatVelocity = (i<NUM_SAMPLES_DEFINED)?pgm_read_byte(&beat1[i][currentStep/4]):0;
      if(currentStep%4==0&&thisBeatVelocity>0) scheduleNote(i, currentStep, thisBeatVelocity, nextNoteTime + slopRand - (float) millis());
      else {
        // temp, playing around
        if(currentStep%4==0) {
          byte yesNoRand = rand(0,255);
          if(yesNoRand < paramChance) {
            long velocityRand = rand(0,255); // is long necessary?
            long velocity = paramMidpoint - paramRange/2 + ((velocityRand * paramRange)>>8); // is long necessary?
            if(velocity > 0) scheduleNote(i, currentStep, velocity, nextNoteTime - (float) millis());
          }
        }
      }
    }
    nextNote();
  }
  schedulerEventDelay.set(lookahead);
  schedulerEventDelay.start();
}

void toggleSequence() {
  if(sequencePlaying) stopSequence();
  else startSequence();
}

void startSequence() {
  sequencePlaying = true;
  currentStep = 0;
  nextNoteTime = millis();
  scheduler();
}

void stopSequence() {
  sequencePlaying = false;
  for(byte i=0; i<NUM_LEDS; i++) {
    digitalWrite(ledPins[i], LOW);
  }
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
        paramTempo = tapTempo.getBPM();
        storedValues[PARAM_TEMPO] = paramTempo - MIN_TEMPO;
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
      if(readyToChooseLoadSlot) loadParams(5);
      else if(readyToChooseSaveSlot) saveParams(5);
      else toggleSequence();
    }

    if(paramGlitch>0) delay((paramGlitch*paramGlitch)>>6);
  }

  byte i;
  if(sequencePlaying) {
    if(schedulerEventDelay.ready()) scheduler();
    for(i=0;i<numEventDelays;i++) {
      if(eventDelays[i].ready() && delayInUse[i]) {
        delayInUse[i] = false;
        if(delayVelocity[i] > 8) { // don't bother with very quiet notes
          switch(delayChannel[i]) {
            case 0:
            kick.start();
            break;
            case 1:
            closedhat.start();
            break;
            case 2:
            snare.start();
            break;
            case 3:
            rim.start();
            break;
            case 4:
            clap.start();
            break;
          }
          sampleVolumes[delayChannel[i]] = delayVelocity[i];
        }
      }
    }
  }

  for(i=0;i<NUM_KNOBS;i++) {
    analogValues[i] = mozziAnalogRead(i)>>2; // read all analog values
  }
  if(controlSetChanged || firstLoop) {
    // "lock" all knobs when control set changes
    for(byte i=0;i<NUM_KNOBS;i++) {
      knobLocked[i] = !firstLoop;
      initValues[i] = analogValues[i];
    }
  } else {
    for(i=0;i<NUM_KNOBS;i++) {
      if(knobLocked[i]) {
        int diff = initValues[i]-analogValues[i];
        if(diff<-5||diff>5) knobLocked[i] = false;
      }
      if(!knobLocked[i]) {
        storedValues[NUM_KNOBS*controlSet+i] = analogValues[i]; // store new analog value if knob active
      }
    }
  }

  // do logic based on params
  if(!newStateLoaded) updateParameters(controlSet);
  
  firstLoop = false;
}

void updateParameters(byte thisControlSet) {
  switch(thisControlSet) {
    case 0:
    paramChance = storedValues[PARAM_CHANCE];
    paramMidpoint = storedValues[PARAM_MIDPOINT];
    paramRange = storedValues[PARAM_RANGE];
    paramZoom = storedValues[PARAM_ZOOM];
    break;

    case 1:
    {
      // pitch alteration (could do this is a more efficient way to reduce RAM usage)
      float thisPitch = fabs((float) storedValues[PARAM_PITCH] * (8.0f/255.0f) - 4.0f);
      float newKickFreq = thisPitch * (float) kick_SAMPLERATE / (float) kick_NUM_CELLS;
      float newHatFreq = thisPitch * (float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS;
      float newSnareFreq = thisPitch * (float) snare_SAMPLERATE / (float) snare_NUM_CELLS;
      float newRimFreq = thisPitch * (float) rim_SAMPLERATE / (float) rim_NUM_CELLS;
      float newClapFreq = thisPitch * (float) clap_SAMPLERATE / (float) clap_NUM_CELLS;
      kick.setFreq(newKickFreq);
      closedhat.setFreq(newHatFreq);
      snare.setFreq(newSnareFreq);
      rim.setFreq(newRimFreq);
      clap.setFreq(newClapFreq);
      bool thisDirection = storedValues[PARAM_PITCH] >= 128;
      kick.setDirection(thisDirection);
      closedhat.setDirection(thisDirection);
      snare.setDirection(thisDirection);
      rim.setDirection(thisDirection);
      clap.setDirection(thisDirection);
      
      // bit crush! high value = clean (8 bits), low value = dirty (1 bit?)
      paramCrush = 7-(storedValues[PARAM_CRUSH]>>5);
      crushCompensation = paramCrush;
      if(paramCrush >= 6) crushCompensation --;
      if(paramCrush >= 7) crushCompensation --;

      // crop - a basic effect to chop off the end of each sample for a more staccato feel
      paramCrop = storedValues[PARAM_CROP];
      kick.setEnd(thisDirection ? map(paramCrop,0,255,100,kick_NUM_CELLS) : kick_NUM_CELLS);
      closedhat.setEnd(thisDirection ? map(paramCrop,0,255,100,closedhat_NUM_CELLS) : closedhat_NUM_CELLS);
      snare.setEnd(thisDirection ? map(paramCrop,0,255,100,snare_NUM_CELLS) : snare_NUM_CELLS);
      rim.setEnd(thisDirection ? map(paramCrop,0,255,100,rim_NUM_CELLS) : rim_NUM_CELLS);
      clap.setEnd(thisDirection ? map(paramCrop,0,255,100,clap_NUM_CELLS) : clap_NUM_CELLS);
      kick.setStart(!thisDirection ? map(paramCrop,255,0,100,kick_NUM_CELLS) : 0);
      closedhat.setStart(!thisDirection ? map(paramCrop,255,0,100,closedhat_NUM_CELLS) : 0);
      snare.setStart(!thisDirection ? map(paramCrop,255,0,100,snare_NUM_CELLS) : 0);
      rim.setStart(!thisDirection ? map(paramCrop,255,0,100,rim_NUM_CELLS) : 0);
      clap.setStart(!thisDirection ? map(paramCrop,255,0,100,clap_NUM_CELLS) : 0);

      // experimental glitch effect
      paramGlitch = storedValues[PARAM_GLITCH];
    }
    break;

    case 2:
    paramSlop = storedValues[PARAM_SLOP]; // between 0ms and 255ms?
    break;

    case 3:
    paramBeat = storedValues[PARAM_BEAT];
    paramTempo = (float) MIN_TEMPO + ((float) storedValues[PARAM_TEMPO]);
    paramTimeSignature = (storedValues[PARAM_TIME_SIGNATURE]>>5)+1;
    numSteps = paramTimeSignature * 16;
    break;

    case 4:
    // using values of 270 and 240 (i.e. 255±15) to give a decent "dead zone" in the middle of the knob
    oscilGain2 = constrain(2*storedValues[PARAM_DRONE]-270, 0, 255);
    if(storedValues[PARAM_DRONE] < 128) {
      oscilGain1 = constrain(240-2*storedValues[PARAM_DRONE], 0, 255);
    } else {
      oscilGain1 = oscilGain2;
    }
    // do same thing for drone modulation gains
    if(storedValues[PARAM_DRONE_MOD] < 128) {
      droneMod2Active = false;
      paramDroneMod = constrain(240-2*storedValues[PARAM_DRONE_MOD], 0, 255);
    } else {
      droneMod2Active = true;
      paramDroneMod = constrain(2*storedValues[PARAM_DRONE_MOD]-270, 0, 255);;
    }
    paramDronePitch = (float) storedValues[PARAM_DRONE_PITCH] * 0.768f + 65.41f; // gives range between "deep C" and "middle C"
    droneOscillator1.setFreq(paramDronePitch);
    droneOscillator2.setFreq(paramDronePitch*1.5f);
    break;
  }
}

const byte atten = 10;
char d1Next, d2Next;
int updateAudio() {
  d1Next = droneOscillator1.next();
  d2Next = droneOscillator2.next();
  char droneSig = ((oscilGain1*d1Next)>>10)+((oscilGain2*d2Next)>>11);
  char droneModSig = (d1Next>>2)+(droneMod2Active?(d2Next>>2):0);
  char asig = ((sampleVolumes[0]*kick.next())>>atten)+
    ((sampleVolumes[1]*closedhat.next())>>atten)+
    ((sampleVolumes[2]*snare.next())>>atten)+
    ((sampleVolumes[3]*rim.next())>>atten)+
    ((sampleVolumes[4]*clap.next())>>atten)+
    (droneSig>>2);
  asig = (asig * ((255-paramDroneMod)+((paramDroneMod*droneModSig)>>6)))>>8;
  asig = (asig>>paramCrush)<<crushCompensation;
  return (int) asig;
}

void startupLedSequence() {
  byte seq[15] = {0,4,3,1,4,2,0,3,1,0,4,3,0,4,2};
  for(byte i=0; i<15; i++) {
    digitalWrite(ledPins[seq[i]], HIGH);
    delay(25);
    digitalWrite(ledPins[seq[i]], LOW);
  }
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
    updateParameters(i);
  }
}

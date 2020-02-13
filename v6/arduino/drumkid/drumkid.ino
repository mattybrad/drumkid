/*  This code is for the version 6 (V6) DrumKid PCB and breadboard designs.
 *  It might work with other versions with a bit of tweaking   
 */

#define DEBUGGING true
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

// include beats file
#include "beats.h"

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

#define CONTROL_RATE 64 // tweak this value if performance is bad, must be power of 2 (64, 128, etc)
#define NUM_BEATS 32
#define MAX_BEAT_STEPS 32
#define NUM_PARAM_GROUPS 5
#define NUM_KNOBS 4
#define NUM_LEDS 5
#define NUM_BUTTONS 6
#define NUM_SAMPLES_TOTAL 5 // total number of samples including those only triggered by chance
#define NUM_SAMPLES_DEFINED 5 // number of samples defined in the preset drumbeats (kick, hat, snare, rim, tom)
#define NUM_DELAY_RECORDS 64
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
byte sampleVolumes[NUM_SAMPLES_TOTAL] = {255,255,255,255,255}; // temp
byte midiNotes[NUM_SAMPLES_TOTAL] = {36,42,38,37,43};
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
byte paramSlop;
byte paramSwing;
unsigned int paramDelayTime;
byte paramBeat;
byte paramTimeSignature;
byte paramDrift;
byte paramDrop;
byte oscilGain1;
byte oscilGain2;
byte paramDroneRoot;
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
//#define PARAM_DELAY_MIX 11
#define PARAM_DROP 11
#define PARAM_BEAT 12
#define PARAM_TEMPO 13
#define PARAM_TIME_SIGNATURE 14
#define PARAM_DRIFT 15
#define PARAM_DRONE_ROOT 16
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

// define root notes
float rootNotes[13] = {
  277.182631f,
  184.9972114f,
  246.9416506f,
  164.8137785f,
  220.0f,
  146.832384f,
  195.997718f,
  261.6255653f,
  174.6141157f,
  233.0818808f,
  155.5634919f,
  207.6523488f,
  138.5913155f,
};

// dropRef determines whether each sample is played or muted
byte dropRef[NUM_SAMPLES_DEFINED] = {
  B11110000, // kick
  B00111111, // hat
  B01111100, // snare
  B00011110, // rim
  B00011000, // tom
};

float nextPulseTime = 0;
int pulseNum = 0;
byte stepNum = 0;
byte tempSlop[NUM_SAMPLES_DEFINED];
byte delayRecords[NUM_DELAY_RECORDS][3]; // 1st byte = time (in pulses), 2nd byte = sample, 3rd byte = velocity (could compress this to 2 bytes if struggling for space)
byte currentDelayIndex = 0;

int driftOffset0;
int driftOffset1;
int driftOffset2;

// glitch stuff
const byte maxFreezeSize = 50;
byte freezeSize = maxFreezeSize;
int freezeRepeatNum = 5;
int freeze[maxFreezeSize];
byte freezePos = 0;
byte freezeRepeats = 0;

void setup() {
  startMozzi(CONTROL_RATE);
  randSeed();

  kick.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  closedhat.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  snare.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  rim.setFreq((float) rim_SAMPLERATE / (float) rim_NUM_CELLS);
  tom.setFreq((float) tom_SAMPLERATE / (float) tom_NUM_CELLS);
  
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

  droneOscillator1.setFreq(110);

  // add more default values here
  storedValues[PARAM_CHANCE] = 0;
  storedValues[PARAM_ZOOM] = 150;
  storedValues[PARAM_MIDPOINT] = 128;
  storedValues[PARAM_RANGE] = 255;
  storedValues[PARAM_CRUSH] = 255;
  storedValues[PARAM_PITCH] = 160;
  storedValues[PARAM_BEAT] = 8;
  storedValues[PARAM_TIME_SIGNATURE] = 0; // this means 4/4, it's confusing...
  storedValues[PARAM_TEMPO] = 100 - MIN_TEMPO; // not BPM!
  storedValues[PARAM_DROP] = 128;
  storedValues[PARAM_DRONE] = 128;
  storedValues[PARAM_DRONE_PITCH] = 128;
  storedValues[PARAM_DRONE_MOD] = 128;
  storedValues[PARAM_CROP] = 255;

  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    updateParameters(i);
  }

  tapTempo.setMinBPM((float) MIN_TEMPO);
  tapTempo.setMaxBPM((float) MAX_TEMPO);

  startupLedSequence();
}

bool syncReceived = false;
byte thisMidiByte;
bool testToggle = false;

void loop() {
  audioHook();
  while(Serial.available()) {
    thisMidiByte = Serial.read();
    if(thisMidiByte==0xF8) {
      syncReceived = true;
      if(sequencePlaying) {
        doPulse();
      }
    }
  }
}

void toggleSequence() {
  if(sequencePlaying) stopSequence();
  else startSequence();
}

void startSequence() {
  #if !DEBUGGING
  Serial.write(0xFA); // MIDI clock start
  #endif
  sequencePlaying = true;
  nextPulseTime = millis();
  pulseNum = 0;
  stepNum = 0;
}

void stopSequence() {
  #if !DEBUGGING
  Serial.write(0xFC); // MIDI clock stop
  #endif
  sequencePlaying = false;
}

byte noteDown = B00000000;
void updateControl() {
  byte i;
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
      if(readyToChooseLoadSlot) loadParams(5);
      else if(readyToChooseSaveSlot) saveParams(5);
      else toggleSequence();
    }

    // messing everything up to make a glitchy effect
    if(paramGlitch>0) {
      freezeRepeatNum = rand(1,(byte) paramGlitch);
      freezeSize = rand(1,(byte) paramGlitch/6);
      //freezeSize = paramGlitch / 5;
    } else {
      freezeSize = 1;
      freezeRepeatNum = 1;
    }
    
  }
  
  if(sequencePlaying&&!syncReceived) {
    float msPerPulse = tapTempo.getBeatLength() / 24.0;
    if(millis()>=nextPulseTime) {
      Serial.println(pulseNum);
      doPulse();
      nextPulseTime = nextPulseTime + msPerPulse;
    }
  }

  for(i=0;i<NUM_KNOBS;i++) {
    if(firstLoop) {
      byte dummyReading = mozziAnalogRead(breadboardAnalogPins[i]);
    } else {
      if(BREADBOARD) {
        analogValues[i] = mozziAnalogRead(breadboardAnalogPins[i])>>2;
      } else {
        analogValues[i] = 255 - (mozziAnalogRead(pcbAnalogPins[i])>>2);
      }
    }
  }
  if(controlSetChanged||secondLoop) {
    // "lock" all knobs when control set changes
    // also do this on second loop (mozziAnalogRead doesn't work properly on first loop)
    for(byte i=0;i<NUM_KNOBS;i++) {
      knobLocked[i] = true;
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
  if(!newStateLoaded) {
    for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
      updateParameters(i);
    }
  }
  
  if(firstLoop) {
    firstLoop = false;
    secondLoop = true;
  } else {
    secondLoop = false;
  }
}

void doPulse() {
  byte i;
  #if !DEBUGGING
  Serial.write(0xF8); // MIDI clock continue
  #endif

  cancelMidiNotes();

  int range0 = paramDrift;
  int driftMax0 = constrain(range0/16,1,255);
  int thisDrift0 = rand(-driftMax0, driftMax0+1);
  driftOffset0 += thisDrift0;
  driftOffset0 = constrain(driftOffset0,-range0,range0);

  int range1 = constrain(2*(paramDrift-127),0,255);
  int driftMax1 = constrain(range1/16,1,255);
  int thisDrift1 = rand(-driftMax1, driftMax1+1);
  driftOffset1 += thisDrift1;
  driftOffset1 = constrain(driftOffset1,-range1,range1);

  int range2 = constrain(4*(paramDrift-191),0,255);
  int driftMax2 = constrain(range2/16,1,255);
  int thisDrift2 = rand(-driftMax2, driftMax2+1);
  driftOffset2 += thisDrift2;
  driftOffset2 = constrain(driftOffset2,-range2,range2);
  
  if(pulseNum%24==0) {
    if(stepNum==0) {
      numSteps = paramTimeSignature * 8;
      if(numSteps == 32) numSteps = 64;
    }
    digitalWrite(ledPins[constrain((stepNum/8)%paramTimeSignature,0,NUM_LEDS-1)],HIGH);
  }
  else if(pulseNum%24==2) digitalWrite(ledPins[constrain((stepNum/8)%paramTimeSignature,0,NUM_LEDS-1)],LOW);
  for(i=0;i<NUM_DELAY_RECORDS;i++) {
    if(delayRecords[i][0]>1) {
      delayRecords[i][0] --;
    } else if(delayRecords[i][0]==1) {
      delayRecords[i][0] = 0;
      triggerNote(delayRecords[i][1], delayRecords[i][2]);
    }
  }
  for(i=0;i<NUM_SAMPLES_DEFINED;i++) {
    if(pulseNum%3==tempSlop[i] && bitRead(dropRef[i],paramDrop)) {
      bool useBeat = pulseNum%6==0;
      calculateNote(i);
    }
  }
  if(pulseNum%3==2) {
    stepNum ++;
    if(stepNum >= numSteps) stepNum = 0;
    for(i=0;i<NUM_SAMPLES_DEFINED;i++) {
      if(stepNum/2%2==0) {
        tempSlop[i] = rand(0,paramSlop+1);
      } else {
        tempSlop[i] = constrain(rand(paramSwing-paramSlop/2, paramSwing+paramSlop/2+1),0,3);
      }
    }
  }
  pulseNum = (pulseNum + 1) % 24;
}

void updateParameters(byte thisControlSet) {
  switch(thisControlSet) {
    case 0:
    paramChance = driftValue(PARAM_CHANCE,0);
    paramMidpoint = driftValue(PARAM_MIDPOINT,0);
    paramRange = driftValue(PARAM_RANGE,0);
    paramZoom = driftValue(PARAM_ZOOM,0);
    break;
    
    case 1:
    {
      // pitch alteration (could do this in a more efficient way to reduce RAM usage)
      float thisPitch = fabs((float) driftValue(PARAM_PITCH,2) * (8.0f/255.0f) - 4.0f);
      float newKickFreq = thisPitch * (float) kick_SAMPLERATE / (float) kick_NUM_CELLS;
      float newHatFreq = thisPitch * (float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS;
      float newSnareFreq = thisPitch * (float) snare_SAMPLERATE / (float) snare_NUM_CELLS;
      float newRimFreq = thisPitch * (float) rim_SAMPLERATE / (float) rim_NUM_CELLS;
      float newtomFreq = thisPitch * (float) tom_SAMPLERATE / (float) tom_NUM_CELLS;
      kick.setFreq(newKickFreq);
      closedhat.setFreq(newHatFreq);
      snare.setFreq(newSnareFreq);
      rim.setFreq(newRimFreq);
      tom.setFreq(newtomFreq);
      bool thisDirection = driftValue(PARAM_PITCH,2) >= 128;
      kick.setDirection(thisDirection);
      closedhat.setDirection(thisDirection);
      snare.setDirection(thisDirection);
      rim.setDirection(thisDirection);
      tom.setDirection(thisDirection);
      
      // bit crush! high value = clean (8 bits), low value = dirty (1 bit?)
      paramCrush = 7-(driftValue(PARAM_CRUSH,1)>>5);
      crushCompensation = paramCrush;
      if(paramCrush >= 6) crushCompensation --;
      if(paramCrush >= 7) crushCompensation --;

      // crop - a basic effect to chop off the end of each sample for a more staccato feel
      paramCrop = driftValue(PARAM_CROP,1);
      kick.setEnd(thisDirection ? map(paramCrop,0,255,100,kick_NUM_CELLS) : kick_NUM_CELLS);
      closedhat.setEnd(thisDirection ? map(paramCrop,0,255,100,closedhat_NUM_CELLS) : closedhat_NUM_CELLS);
      snare.setEnd(thisDirection ? map(paramCrop,0,255,100,snare_NUM_CELLS) : snare_NUM_CELLS);
      rim.setEnd(thisDirection ? map(paramCrop,0,255,100,rim_NUM_CELLS) : rim_NUM_CELLS);
      tom.setEnd(thisDirection ? map(paramCrop,0,255,100,tom_NUM_CELLS) : tom_NUM_CELLS);
      kick.setStart(!thisDirection ? map(paramCrop,255,0,100,kick_NUM_CELLS) : 0);
      closedhat.setStart(!thisDirection ? map(paramCrop,255,0,100,closedhat_NUM_CELLS) : 0);
      snare.setStart(!thisDirection ? map(paramCrop,255,0,100,snare_NUM_CELLS) : 0);
      rim.setStart(!thisDirection ? map(paramCrop,255,0,100,rim_NUM_CELLS) : 0);
      tom.setStart(!thisDirection ? map(paramCrop,255,0,100,tom_NUM_CELLS) : 0);
      
      // experimental glitch effect
      paramGlitch = driftValue(PARAM_GLITCH,2);
    }
    break;
    
    case 2:
    paramSlop = storedValues[PARAM_SLOP] / 86; // 255/43 => between 0 and 5 pulses
    paramSwing = storedValues[PARAM_SWING] / 86; // 255/43 => between 0 and 5 pulses
    //paramDelayMix = map(storedValues[PARAM_DELAY_MIX],0,256,0,4);
    paramDelayTime = map(storedValues[PARAM_DELAY_TIME],0,255,0,24);
    break;

    case 3:
    paramBeat = (NUM_BEATS * (int) storedValues[PARAM_BEAT]) / 256;
    tapTempo.setBPM((float) MIN_TEMPO + ((float) storedValues[PARAM_TEMPO]));
    //Serial.println(tapTempo.getBPM());
    //Serial.println(tapTempo.getBeatLength());
    paramTimeSignature = map(storedValues[PARAM_TIME_SIGNATURE],0,256,4,8);
    //Serial.print("TIME SIG: ");
    //Serial.println(paramTimeSignature);
    //numSteps = paramTimeSignature * 8;
    //if(numSteps == 32) numSteps = 64;
    //Serial.print("STEPS: ");
    //Serial.println(numSteps);
    paramDrift = storedValues[PARAM_DRIFT];
    break;
    
    case 4:
    paramDrop = driftValue(PARAM_DROP,2) >> 5; // range of 0 to 7
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
      paramDroneMod = constrain(240-2*driftValue(PARAM_DRONE_MOD,2), 0, 255);
    } else {
      droneMod2Active = true;
      paramDroneMod = constrain(2*driftValue(PARAM_DRONE_MOD,2)-270, 0, 255);;
    }
    paramDroneRoot = map(driftValue(PARAM_DRONE_ROOT,2),0,255,0,12);
    paramDronePitch = rootNotes[paramDroneRoot] * (0.5f + (float) storedValues[PARAM_DRONE_PITCH]/170.0f);// * 0.768f + 65.41f;
    droneOscillator1.setFreq(paramDronePitch);
    droneOscillator2.setFreq(paramDronePitch*1.5f);
    break;
  }
}

byte driftValue(byte storedValueNum, byte driftGroup) {
  switch(driftGroup) {
    case 0:
    return constrain(storedValues[storedValueNum]+driftOffset0,0,255);
    break;
    case 1:
    return constrain(storedValues[storedValueNum]+driftOffset1,0,255);
    break;
    case 2:
    return constrain(storedValues[storedValueNum]+driftOffset2,0,255);
    break;
  }
}

byte zoomValues[] = {32,16,8,4,2,1};
void calculateNote(byte sampleNum) {
  byte zoomValueIndex = paramZoom / 51; // gives value from 0 to 5
  byte zoomVelocity = paramZoom % 51; // 
  if(zoomValueIndex == 5) {
    zoomValueIndex = 4;
    zoomVelocity = 51;
  }
  byte lowerZoomValue = zoomValues[zoomValueIndex]; // e.g. 8 for a quarter note
  byte upperZoomValue = zoomValues[zoomValueIndex+1]; // e.g. 16 for a quarter note
  long thisVelocity = 0;
  if(stepNum%2==0) {
    // beats only defined down to 16th notes not 32nd, hence %2
    byte beatByte = pgm_read_byte(&beats[paramBeat][sampleNum][stepNum/16]);
    if(bitRead(beatByte,7-((stepNum/2)%8))) thisVelocity = 255;
  }
  if(thisVelocity==0) {
    // for steps not defined in beat, use algorithm to determine velocity
    if(stepNum%upperZoomValue==0) {
      // if step length is longer(?) than is affected by zoom
      byte yesNoRand = rand(0,255);
      if(yesNoRand < paramChance) {
        //int velocityRand = rand(0,255);
        //velocityRand = 255;
        //thisVelocity = velocityRand;
        //thisVelocity = paramMidpoint - paramRange/2 + ((velocityRand * paramRange)>>8);
        int lowerBound = paramMidpoint - paramRange/2;
        int upperBound = paramMidpoint + paramRange/2;
        thisVelocity = rand(lowerBound, upperBound);
        thisVelocity = constrain(thisVelocity,0,255);
        if(stepNum%lowerZoomValue!=0) {
          Serial.println(thisVelocity);
          thisVelocity = thisVelocity * 5 * zoomVelocity / 255;
          Serial.println(thisVelocity);
          Serial.println("");
        }
      }
    }
  }

  // add delay records
  byte i;
  if(paramDelayTime>0) {
    for(i=1;i<8;i++) {
      byte delayVelocity = thisVelocity;
      delayVelocity = delayVelocity >> i;
      if(delayVelocity>8) {
        delayRecords[currentDelayIndex][0] = paramDelayTime*i;
        delayRecords[currentDelayIndex][1] = sampleNum;
        delayRecords[currentDelayIndex][2] = delayVelocity;
        currentDelayIndex = (currentDelayIndex + 1) % NUM_DELAY_RECORDS;
      }
    }
  }
    
  triggerNote(sampleNum, thisVelocity);
}

void triggerNote(byte sampleNum, byte velocity) {
  if(velocity>8) { // don't bother with very quiet notes
    switch(sampleNum) {
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
      tom.start();
      break;
    }
    sampleVolumes[sampleNum] = velocity;
    bitWrite(noteDown,sampleNum,true);
    playMidiNote(midiNotes[sampleNum], velocity);
  }
}

const byte atten = 9;
char d1Next, d2Next;
int updateAudio() {
  d1Next = droneOscillator1.next();
  d2Next = droneOscillator2.next();
  char droneSig = ((oscilGain1*d1Next)>>10)+((oscilGain2*d2Next)>>10);
  char droneModSig = (d1Next>>2)+(droneMod2Active?(d2Next>>2):0);
  char asig = ((sampleVolumes[0]*kick.next())>>atten)+
    ((sampleVolumes[1]*closedhat.next())>>atten)+
    ((sampleVolumes[2]*snare.next())>>atten)+
    ((sampleVolumes[3]*rim.next())>>atten)+
    ((sampleVolumes[4]*tom.next())>>atten)+
    (droneSig>>2);
  asig = (asig * ((255-paramDroneMod)+((paramDroneMod*droneModSig)>>6)))>>8;
  asig = (asig>>paramCrush)<<crushCompensation;
  if(freezeRepeats == 0) {
    freeze[freezePos] = (int) asig;
  }
  byte thisFreezePos = freezePos;
  freezePos ++;
  if(freezePos >= freezeSize) {
    freezePos = 0;
    freezeRepeats ++;
    if(freezeRepeats >= freezeRepeatNum) freezeRepeats = 0;
  }
  return freeze[freezePos];
  //return (int) asig;
}

void startupLedSequence() {
  byte seq[15] = {0,1,2,3,4,3,2,1,0,1,2,3,4,3,2};
  for(byte i=0; i<15; i++) {
    digitalWrite(ledPins[seq[i]], HIGH);
    delay(30);
    digitalWrite(ledPins[seq[i]], LOW);
  }
}

void cancelMidiNotes() {
  byte i;
  for(i=0; i<NUM_SAMPLES_TOTAL; i++) {
    #if !DEBUGGING
    if(bitRead(noteDown,i)) {
      Serial.write(0x90);
      Serial.write(midiNotes[i]);
      Serial.write(0x00);
      bitWrite(noteDown,i,false);
    }
    #endif
  }
}

void playMidiNote(byte noteNum, byte velocity) {
  #if !DEBUGGING
  Serial.write(0x90);
  Serial.write(noteNum);
  Serial.write(velocity>>1);
  #endif
}

void saveParams(byte slotNum) {
  // 1024 possible byte locations
  // currently need 20 bytes per saved state
  // currently planning choice of 6 sllots for saved states (1 per button)
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

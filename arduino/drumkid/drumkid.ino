#define IS_BREADBOARD true // switch to false if compiling code for PCB

// include mozzi library files - currently requires particular version of mozzi for compatibility:
// https://github.com/sensorium/Mozzi/archive/e1eba4410200842157763f1471dca34bf4867138.zip
#include <MozziGuts.h>
#include <Sample.h>
#include <EventDelay.h>
#include <mozzi_rand.h>

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
#include "click.h"

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
#define NUM_SAMPLES 4
#define SAVED_STATE_SLOT_BYTES 32

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
byte sampleVolumes[NUM_SAMPLES] = {255,255,255,255};
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
byte crushCompensation = 0;
int paramSlop = 0;
float paramTempo = 120.0;
byte paramTimeSignature = 4;
#define PARAM_CHANCE 0
#define PARAM_ZOOM 1
#define PARAM_MIDPOINT 2
#define PARAM_RANGE 3
#define PARAM_PITCH 4
#define PARAM_CRUSH 5
#define PARAM_6 6
#define PARAM_7 7
#define PARAM_SLOP 8
#define PARAM_BLEND 9
#define PARAM_10 10
#define PARAM_11 11
#define PARAM_TEMPO 12
#define PARAM_TIME_SIGNATURE 13
#define PARAM_14 14
#define PARAM_15 15
#define PARAM_16 16
#define PARAM_17 17
#define PARAM_18 18
#define PARAM_19 19
byte storedValues[NUM_PARAM_GROUPS*NUM_KNOBS];

// define samples
Sample <kick_NUM_CELLS, AUDIO_RATE> kick(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare(snare_DATA);
Sample <click_NUM_CELLS, AUDIO_RATE> click(click_DATA);

// could just use bools instead of bytes to save space
const byte beat1[NUM_SAMPLES][MAX_BEAT_STEPS] PROGMEM = {  {255,255,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,255,0,0,0,0,0,0,255,0,0,0,0,0,0,0,},
                                  {255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,},
                                  {0,0,0,0,255,0,0,0,0,0,0,0,255,128,64,32,0,0,0,0,255,0,0,0,0,0,0,0,255,128,64,32,},
                                  {0,128,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,},};
              

void setup() {
  startMozzi(CONTROL_RATE);
  randSeed();
  kick.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  closedhat.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  snare.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  click.setFreq((float) click_SAMPLERATE / (float) click_NUM_CELLS);
  kick.setEnd(7000);
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
    click.next();
  }

  // add more default values here
  storedValues[PARAM_CRUSH] = 255;
  storedValues[PARAM_PITCH] = 128;
  storedValues[PARAM_TIME_SIGNATURE] = 120; // this means 4/4, it's confusing...
  storedValues[PARAM_TEMPO] = 100; // not BPM!

  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    updateParameters(i);
  }
  
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
    for(byte i=0;i<NUM_SAMPLES;i++) {
      int slopRand = rand(0,paramSlop) - paramSlop / 2;
      thisBeatVelocity = pgm_read_byte(&beat1[i][currentStep/4]);
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
        storedValues[PARAM_TEMPO] = paramTempo - 40;
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
  }

  byte i;
  if(sequencePlaying) {
    if(schedulerEventDelay.ready()) scheduler();
    for(i=0;i<numEventDelays;i++) {
      if(eventDelays[i].ready() && delayInUse[i]) {
        delayInUse[i] = false;
        if(delayVelocity[i] > 8) { // don't bother with very quiet notes
          if(delayChannel[i]==0) {
            kick.start();
          } else if(delayChannel[i]==1) {
            closedhat.start();
          } else if(delayChannel[i]==2) {
            snare.start();
          } else if(delayChannel[i]==3) {
            click.start();
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
      //float newKickFreq = ((float) storedValues[PARAM_PITCH] / 63.0f) * (float) kick_SAMPLERATE / (float) kick_NUM_CELLS;
      float newHatFreq = ((float) storedValues[PARAM_PITCH] / 63.0f) * (float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS;
      float newSnareFreq = ((float) storedValues[PARAM_PITCH] / 63.0f) * (float) snare_SAMPLERATE / (float) snare_NUM_CELLS;
      float newClickFreq = ((float) storedValues[PARAM_PITCH] / 63.0f) * (float) click_SAMPLERATE / (float) click_NUM_CELLS;
      //kick.setFreq(newKickFreq);
      closedhat.setFreq(newHatFreq);
      snare.setFreq(newSnareFreq);
      click.setFreq(newClickFreq);
      // high value = clean (8 bits), low value = dirty (
      paramCrush = 7-(storedValues[PARAM_CRUSH]>>5);
      crushCompensation = paramCrush;
      if(paramCrush >= 6) crushCompensation --;
      if(paramCrush >= 7) crushCompensation --;
    }
    break;

    case 2:
    paramSlop = storedValues[PARAM_SLOP]; // between 0ms and 255ms?
    break;

    case 3:
    paramTempo = 40.0 + ((float) storedValues[PARAM_TEMPO]);
    paramTimeSignature = (storedValues[PARAM_TIME_SIGNATURE]>>5)+1;
    numSteps = paramTimeSignature * 16;
    break;
  }
}

const byte atten = 9;
int updateAudio() {
  char asig = ((sampleVolumes[0]*kick.next())>>atten)+((sampleVolumes[1]*closedhat.next())>>atten)+((sampleVolumes[2]*snare.next())>>atten)+((sampleVolumes[3]*click.next())>>atten);
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

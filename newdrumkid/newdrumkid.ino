#define IS_BREADBOARD true // switch to false if compiling code for PCB

// include mozzi library files
#include <MozziGuts.h>
#include <Sample.h>
#include <EventDelay.h>
#include <mozzi_rand.h>

// include debouncing library
#include <Bounce2.h>

// include audio data files
#include "kick.h"
#include "closedhat.h"
#include "snare.h"

// define buttons
Bounce buttonA = Bounce();
Bounce buttonB = Bounce();
Bounce buttonC = Bounce();
Bounce buttonX = Bounce();
Bounce buttonY = Bounce();
Bounce buttonZ = Bounce();

#define CONTROL_RATE 256 // tweak this value if performance is bad, must be power of 2 (64, 128, etc)
#define MAX_BEAT_STEPS 32

bool sequencePlaying = false; // false when stopped, true when playing
byte currentStep;
byte numSteps = 4 * 16; // 64 steps = 4/4 time signature, 48 = 3/4 or 6/8, 80 = 5/4, 112 = 7/8 etc
float nextNoteTime;
float scheduleAheadTime = 100; // ms
float lookahead = 25; // ms
float tempo = 140.0;
const byte numEventDelays = 20;
EventDelay schedulerEventDelay;
EventDelay eventDelays[numEventDelays];
bool delayInUse[numEventDelays];
byte delayChannel[numEventDelays];
byte eventDelayIndex = 0;
bool beatLedsActive = true;
byte controlSet = 0;

// variables relating to knob values
bool knobLocked[4] = {true,true,true,true};
int analogValues[4] = {0,0,0,0};
int initValues[4] = {0,0,0,0};
int storedValues[5][4] = {  {512,512,512,512},    // A
                            {512,512,512,512},    // B
                            {512,512,512,512},    // C
                            {512,512,512,512},    // X
                            {512,512,512,512},};  // Y

// define samples
Sample <kick_NUM_CELLS, AUDIO_RATE> kick1(kick_DATA);
Sample <kick_NUM_CELLS, AUDIO_RATE> kick2(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat1(closedhat_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat2(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare1(snare_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare2(snare_DATA);

byte beat1[][MAX_BEAT_STEPS] = {  {255,255,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,255,0,0,0,0,0,0,255,0,0,0,0,0,0,0,},
                      {255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,},
                      {0,0,0,0,255,0,0,0,0,0,0,0,255,128,64,32,0,0,0,0,255,0,0,0,0,0,0,0,255,128,64,32,},};

// define pins
byte breadboardLedPins[5] = {2,3,4,5,13};
byte breadboardButtonPins[6] = {7,8,10,11,12,6};
byte pcbLedPins[5] = {6,5,4,3,2};
byte pcbButtonPins[6] = {13,12,11,10,8,7};
byte (&ledPins)[5] = IS_BREADBOARD ? breadboardLedPins : pcbLedPins;
byte (&buttonPins)[6] = IS_BREADBOARD ? breadboardButtonPins : pcbButtonPins;

void setup() {
  startMozzi(CONTROL_RATE);
  randSeed();
  kick1.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  kick2.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  closedhat1.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  closedhat2.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  snare1.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  snare2.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  kick1.setEnd(7000);
  kick2.setEnd(7000);
  Serial.begin(9600);
  for(byte i=0;i<5;i++) {
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

  startupLedSequence();
}

void loop() {
  audioHook();
}

void nextNote() {
  float msPerBeat = (60.0 / tempo) * 1000.0;
  nextNoteTime += 0.25 * msPerBeat / 4;
  currentStep ++;
  currentStep = currentStep % numSteps;
}

void scheduleNote(byte channelNumber, int beatNumber, float delayTime) {
  // schedule a drum hit to occur after [delayTime] milliseconds
  eventDelays[eventDelayIndex].set(delayTime);
  eventDelays[eventDelayIndex].start();
  delayInUse[eventDelayIndex] = true;
  delayChannel[eventDelayIndex] = channelNumber;
  eventDelayIndex = (eventDelayIndex + 1) % numEventDelays; // replace with "find next free slot" function
}

void scheduler() {
  while(nextNoteTime < (float) millis() + scheduleAheadTime) {
    byte thisStep = currentStep/16;
    byte stepLED = constrain(thisStep, 0, 4);
    if(beatLedsActive) {
      if(currentStep%16==0) digitalWrite(ledPins[stepLED], HIGH);
      else if(currentStep%4==0) digitalWrite(ledPins[stepLED], LOW);
    }
    for(byte i=0;i<3;i++) {
      if(currentStep%4==0&&beat1[i][currentStep/4]>0) scheduleNote(i, currentStep, nextNoteTime - (float) millis());
      //else if(i==2&&currentStep%2==0&&rand(0,10)==0) scheduleNote(i, currentStep, nextNoteTime - (float) millis());
      //else if(i==2&&rand(0,40)==0) scheduleNote(i, currentStep, nextNoteTime - (float) millis());
    }
    nextNote();
  }
  schedulerEventDelay.set(lookahead);
  schedulerEventDelay.start();
}

void play() {
  sequencePlaying = !sequencePlaying;

  if(sequencePlaying) {
    // start
    currentStep = 0;
    nextNoteTime = millis();
    scheduler();
  } else {
    // stop
    for(byte i=0; i<5; i++) {
      digitalWrite(ledPins[i], LOW);
    }
    // cancel "timeout" somehow..?
  }
}

void updateControl() {
  
  buttonA.update();
  buttonB.update();
  buttonC.update();
  buttonX.update();
  buttonY.update();
  buttonZ.update();

  // switch active set of control knobs if button pressed
  byte prevControlSet = controlSet;
  if(buttonA.fell()) controlSet = 0;
  else if(buttonB.fell()) controlSet = 1;
  else if(buttonC.fell()) controlSet = 2;
  else if(buttonX.fell()) controlSet = 3;
  else if(buttonY.fell()) controlSet = 4;
  bool controlSetChanged = (prevControlSet != controlSet);
  
  if(buttonZ.fell()) play();
  if(sequencePlaying) {
    if(schedulerEventDelay.ready()) scheduler();
    for(byte i=0;i<numEventDelays;i++) {
      if(eventDelays[i].ready() && delayInUse[i]) {
        delayInUse[i] = false;
        if(delayChannel[i]==0) kick1.start();
        else if(delayChannel[i]==1) closedhat1.start();
        else if(delayChannel[i]==2) snare1.start();
      }
    }
  }

  for(int i=0;i<4;i++) {
    analogValues[i] = mozziAnalogRead(i); // read all analog values
  }
  if(controlSetChanged) {
    // "lock" all knobs when control set changes
    for(byte i=0;i<4;i++) {
      knobLocked[i] = true;
      initValues[i] = analogValues[i];
    }
  } else {
    // unlock knobs if passing through stored value position
    for(byte i=0;i<4;i++) {
      if(knobLocked[i]) {
        if(initValues[i]<storedValues[controlSet][i]) {
          if(analogValues[i]>=storedValues[controlSet][i]) {
            knobLocked[i] = false;
          }
        } else {
          if(analogValues[i]<=storedValues[controlSet][i]) {
            knobLocked[i] = false;
          }
        }
      }
      if(!knobLocked[i]) {
        storedValues[controlSet][i] = analogValues[i]; // store new analog value if knob active
      }
    }
  }
}

int updateAudio() {
  char asig = (closedhat1.next() + kick1.next() + snare1.next()) >> 1;
  return (int) asig;
}

void startupLedSequence() {
  byte seq[15] = {0,4,3,1,4,2,0,3,1,0,4,3,0,4,2};
  for(byte i=0; i<15; i++) {
    digitalWrite(ledPins[seq[i]], HIGH);
    delay(50);
    digitalWrite(ledPins[seq[i]], LOW);
  }
}

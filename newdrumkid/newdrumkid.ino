#include <MozziGuts.h>
#include <Sample.h>
#include <EventDelay.h>
#include <mozzi_rand.h>
#include "kick.h"
#include "closedhat.h"
#include "snare.h"

#define CONTROL_RATE 256
bool sequencePlaying = false;
int currentStep;
float nextNoteTime;
float scheduleAheadTime = 100; // ms
float lookahead = 25; // ms
float tempo = 120.0;
const int numEventDelays = 20;
EventDelay schedulerEventDelay;
EventDelay eventDelays[numEventDelays];
bool delayInUse[numEventDelays];
byte delayChannel[numEventDelays];
int eventDelayIndex = 0;
Sample <kick_NUM_CELLS, AUDIO_RATE> kick1(kick_DATA);
Sample <kick_NUM_CELLS, AUDIO_RATE> kick2(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat1(closedhat_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat2(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare1(snare_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare2(snare_DATA);
byte beat1[][16] = {  {255,255,0,0,0,0,0,0,255,0,0,0,0,0,0,0,},
                      {255,128,255,128,255,128,255,128,255,128,255,128,255,128,255,128,},
                      {0,0,0,0,255,0,0,0,0,0,0,0,255,128,64,32,},};

// temp
int ledPins[] = {2,3,4,5,6};
int buttonPins[] = {7,8,10,11,12,13};

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
  play();
  Serial.begin(9600);
  for(int i=0;i<5;i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(buttonPins[5], INPUT);
}

void loop() {
  audioHook();
}

void nextNote() {
  float msPerBeat = (60.0 / tempo) * 1000.0;
  nextNoteTime += 0.25 * msPerBeat / 4;
  currentStep ++;
  currentStep = currentStep % 64;
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
    for(int i=0;i<3;i++) {
      if(currentStep%4==0&&beat1[i][currentStep/4]>0) scheduleNote(i, currentStep, nextNoteTime - (float) millis());
      //else if(i==1&&rand(0,4)==0) scheduleNote(i, currentStep, nextNoteTime - (float) millis());
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
    // cancel "timeout" somehow
  }
}

void updateControl() {
  bool masterButton = digitalRead(buttonPins[5]);
  for(int i=0;i<5;i++) {
    digitalWrite(ledPins[i], (!digitalRead(buttonPins[i])||masterButton));
  }
  if(schedulerEventDelay.ready()) scheduler();
  for(int i=0;i<numEventDelays;i++) {
    if(eventDelays[i].ready() && delayInUse[i]) {
      delayInUse[i] = false;
      if(delayChannel[i]==0) kick1.start();
      else if(delayChannel[i]==1) closedhat1.start();
      else if(delayChannel[i]==2) snare1.start();
    }
  }
}

int updateAudio() {
  return closedhat1.next() + kick1.next() + snare1.next();
}

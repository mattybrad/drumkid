#include <MozziGuts.h>
#include <Sample.h>
#include <EventDelay.h>
#include <mozzi_rand.h>
#include "closedhat.h"

#define CONTROL_RATE 128
bool metronomePlaying = false;
int currentStep;
float nextNoteTime;
float scheduleAheadTime = 100; // ms
float lookahead = 25; // ms
float tempo = 120.0;
const int numEventDelays = 20;
EventDelay schedulerEventDelay;
EventDelay eventDelays[numEventDelays];
bool delayInUse[numEventDelays];
int eventDelayIndex = 0;
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat(closedhat_DATA);

void setup() {
  startMozzi(CONTROL_RATE);
  closedhat.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  play();
  randSeed();
  Serial.begin(9600);
}

void loop() {
  audioHook();
}

void nextNote() {
  float msPerBeat = (60.0 / tempo) * 1000.0;
  nextNoteTime += 0.25 * msPerBeat;
  currentStep ++;
  currentStep = currentStep % 16;
}

void scheduleNote(int beatNumber, float delayTime) {
  // schedule a drum hit to occur after [delayTime] milliseconds
  eventDelays[eventDelayIndex].set(delayTime);
  eventDelays[eventDelayIndex].start();
  delayInUse[eventDelayIndex] = true;
  eventDelayIndex = (eventDelayIndex + 1) % numEventDelays;
  //Serial.println(eventDelayIndex);
}

void scheduler() {
  while(nextNoteTime < (float) millis() + scheduleAheadTime) {
    //Serial.println(nextNoteTime - millis());
    float sloppy = (float) (rand(0,16) - 8);
    scheduleNote(currentStep, nextNoteTime - (float) millis() + sloppy);
    nextNote();
  }
  schedulerEventDelay.set(lookahead);
  schedulerEventDelay.start();
}

void play() {
  metronomePlaying = !metronomePlaying;

  if(metronomePlaying) {
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
  if(schedulerEventDelay.ready()) scheduler();
  for(int i=0;i<numEventDelays;i++) {
    if(eventDelays[i].ready() && delayInUse[i]) {
      delayInUse[i] = false;
      //Serial.println("START");
      //Serial.println(i);
      closedhat.start();
    }
  }
}

int updateAudio() {
  return closedhat.next();
}


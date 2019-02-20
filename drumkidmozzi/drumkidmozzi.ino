// include relevant mozzi libraries
#include <MozziGuts.h>
#include <Sample.h>
#include <LowPassFilter.h>
#include <EventDelay.h>
#include <mozzi_rand.h>

// include custom sample files
#include "kick.h"
#include "closedhat.h"
#include "snare.h"

#define CONTROL_RATE 64

// define samples
Sample <kick_NUM_CELLS, AUDIO_RATE> aSample(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> bSample(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> cSample(snare_DATA);

// define other mozzi things
LowPassFilter lpf;
EventDelay kTriggerDelay;

byte bitCrushLevel; // between 0 and 7
byte bitCrushCompensation;
int beatTime = 150;

void setup(){
  startMozzi(CONTROL_RATE);
  aSample.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  bSample.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  cSample.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  aSample.setEnd(7000); // was getting a funny click at the end of the kick sample
  lpf.setResonance(200);
  lpf.setCutoffFreq(255);
  kTriggerDelay.set(beatTime);
}


void updateControl(){
  if(kTriggerDelay.ready()){
    byte r = rand(3);
    if(r==0) aSample.start();
    else if(r==1) cSample.start();
    else bSample.start();
    kTriggerDelay.start(beatTime);
  }
  if(true) lpf.setCutoffFreq(mozziAnalogRead(0)>>2);
  if(false) {
    bitCrushLevel = 7-(mozziAnalogRead(0)>>7);
    bitCrushCompensation = bitCrushLevel;
    if(bitCrushLevel >= 6) bitCrushCompensation --;
    if(bitCrushLevel >= 7) bitCrushCompensation --;
  }
  if(false) beatTime = 20 + mozziAnalogRead(0); // temp
}


int updateAudio(){
  char asig = lpf.next((aSample.next()>>1)+(bSample.next()>>1)+(cSample.next()>>1));
  asig = (asig>>bitCrushLevel)<<bitCrushCompensation;
  return (int) asig;
}


void loop(){
  audioHook();
}




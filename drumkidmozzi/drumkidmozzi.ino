#include <MozziGuts.h>
#include <Sample.h> // Sample template
#include "kick.h"
#include "closedhat.h"
#include "snare.h"
#include <EventDelay.h>
#include <mozzi_rand.h>

#define CONTROL_RATE 64

// use: Sample <table_size, update_rate> SampleName (wavetable)
Sample <kick_NUM_CELLS, AUDIO_RATE> aSample(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> bSample(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> cSample(snare_DATA);

// for scheduling sample start
EventDelay kTriggerDelay;

void setup(){
  startMozzi(CONTROL_RATE);
  aSample.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  bSample.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  cSample.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  aSample.setEnd(7000);
  kTriggerDelay.set(200);
}


void updateControl(){
  if(kTriggerDelay.ready()){
    byte r = rand(3);
    if(r==0) {
      aSample.start();
    }
    else if(r==1) bSample.start();
    else cSample.start();
    kTriggerDelay.start();
  }
}


int updateAudio(){
  return (int) aSample.next()+bSample.next()+cSample.next();
}


void loop(){
  audioHook();
}




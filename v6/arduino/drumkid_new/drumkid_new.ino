/*  This code is for the version 6 (V6) DrumKid PCB and breadboard designs.
 *  It might work with other versions with a bit of tweaking   
 */

#include <MozziGuts.h>
#include <Sample.h>
#include <mozzi_rand.h>

#include "kick.h"
#include "closedhat.h"
#include "snare.h"

Sample <kick_NUM_CELLS, AUDIO_RATE> kick(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare(snare_DATA);

// include debouncing library
#include <Bounce2.h>

Bounce buttonA = Bounce();
Bounce buttonB = Bounce();

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // Hz, powers of 2 are most reliable

void setup(){
  startMozzi(CONTROL_RATE);
  kick.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  closedhat.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  snare.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  buttonA.interval(25);
  buttonB.interval(25);
  buttonA.attach(4, INPUT_PULLUP);
  buttonB.attach(5, INPUT_PULLUP);
}

float nextPulseTime = 0.0;
float msPerPulse = 25.0;
void updateControl(){
  /*buttonA.update();
  buttonB.update();
  if(buttonA.fell()) kick.start();
  if(buttonB.fell()) snare.start();*/
  if(millis()>=nextPulseTime) {
    doPulse();
    nextPulseTime = nextPulseTime + msPerPulse;
  }
}

void doPulse() {
  if(rand(0,10)==0) kick.start();
  if(rand(0,2)==0) closedhat.start();
  if(rand(0,10)==0) snare.start();
}

int updateAudio(){
  char asig = (kick.next() >> 2) + (closedhat.next() >> 2) + (snare.next() >> 2);
  return asig; // return an int signal centred around 0
}


void loop(){
  audioHook(); // required here
}

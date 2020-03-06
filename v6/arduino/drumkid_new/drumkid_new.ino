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

float nextPulseTime = 0.0;
float msPerPulse = 20.8333; // 120bpm
byte pulseNum = 0; // 0 to 23 (24ppqn, pulses per quarter note)
byte beatNum = 0; // 0 to 7 (max two bars of 8 beats)

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

void updateControl(){
  /*buttonA.update();
  buttonB.update();
  if(buttonA.fell()) kick.start();
  if(buttonB.fell()) snare.start();*/
  if(millis()>=nextPulseTime) {
    playPulseHits();
    incrementPulse();
    nextPulseTime = nextPulseTime + msPerPulse;
  }
}

void playPulseHits() {
  if(pulseNum % 12 == 0) closedhat.start();
  if(pulseNum % 24 == 0) {
    if(beatNum % 2 == 0) kick.start();
    else snare.start();
  }
}

void incrementPulse() {
  pulseNum ++;
  if(pulseNum == 24) {
    pulseNum = 0;
    beatNum ++;
    if(beatNum == 4) {
      beatNum = 0;
    }
  }
}

int updateAudio(){
  char asig = (kick.next() >> 2) + (closedhat.next() >> 2) + (snare.next() >> 2);
  return asig; // return an int signal centred around 0
}


void loop(){
  audioHook(); // required here
}

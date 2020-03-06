/*  This code is for the version 6 (V6) DrumKid PCB and breadboard designs.
 *  It might work with other versions with a bit of tweaking   
 */

#include <MozziGuts.h>
#include <Sample.h>
#include <mozzi_rand.h>

#include "kick.h"
#include "closedhat.h"
#include "snare.h"
#include "rim.h"
#include "tom.h"

Sample <kick_NUM_CELLS, AUDIO_RATE> kick(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> closedhat(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> snare(snare_DATA);
Sample <rim_NUM_CELLS, AUDIO_RATE> rim(rim_DATA);
Sample <tom_NUM_CELLS, AUDIO_RATE> tom(tom_DATA);

// include debouncing library
#include <Bounce2.h>

Bounce buttonA = Bounce();
Bounce buttonB = Bounce();

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // Hz, powers of 2 are most reliable

float nextPulseTime = 0.0;
float msPerPulse = 20.8333; // 120bpm
byte pulseNum = 0; // 0 to 23 (24ppqn, pulses per quarter note)
byte stepNum = 0; // 0 to 32 (max two bars of 8 beats, aka 32 16th-notes)

// temp beat definition
byte beats[1][5][4] = {
  { // videotape bonnaroo
    {B10000000,B10000010,B10000000,B10000010,},
    {B10101010,B10111010,B10101010,B10111010,},
    {B00001000,B00001000,B00001000,B00001000,},
    {B00000000,B00001000,B00000000,B00001000,},
    {B00000000,B00000100,B00000000,B00000100,},
  },
};

void setup(){
  startMozzi(CONTROL_RATE);
  kick.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  closedhat.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  snare.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  rim.setFreq((float) rim_SAMPLERATE / (float) rim_NUM_CELLS);
  tom.setFreq((float) tom_SAMPLERATE / (float) tom_NUM_CELLS);
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
  if(pulseNum % 6 == 0) {
    for(byte i=0; i<5; i++) {
      if(bitRead(beats[0][i][stepNum/8],7-(stepNum%8))) {
        switch(i) {
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
      }
    }
  }
}

void incrementPulse() {
  pulseNum ++;
  if(pulseNum == 24) {
    pulseNum = 0;
  }
  if(pulseNum % 6 == 0) {
    stepNum ++;
    if(stepNum == 32) {
      stepNum = 0;
    }
  }
}

int updateAudio(){
  char asig = (kick.next() >> 2) + (closedhat.next() >> 3) + (snare.next() >> 2) + (rim.next() >> 1) + (tom.next() >> 2);
  return asig; // return an int signal centred around 0
}


void loop(){
  audioHook(); // required here
}

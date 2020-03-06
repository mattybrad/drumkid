/*  This code is for the version 6 (V6) DrumKid PCB and breadboard designs.
 *  It might work with other versions with a bit of tweaking   
 */

#include <MozziGuts.h>
#include <Sample.h>
//#include <mozzi_rand.h>

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

#include "ArduinoTapTempoDK/src/ArduinoTapTempo.cpp"
ArduinoTapTempo tapTempo;

Bounce buttonA = Bounce();
Bounce buttonB = Bounce();
Bounce buttonC = Bounce();
Bounce buttonX = Bounce();
Bounce buttonY = Bounce();
Bounce buttonZ = Bounce();

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 256 // Hz, aiming for 256 to keep up with high tempos

float nextPulseTime = 0.0;
float msPerPulse = 20.8333; // 120bpm
byte pulseNum = 0; // 0 to 23 (24ppqn, pulses per quarter note)
byte stepNum = 0; // 0 to 32 (max two bars of 8 beats, aka 32 16th-notes)
bool beatPlaying = false;

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
  buttonC.interval(25);
  buttonX.interval(25);
  buttonY.interval(25);
  buttonZ.interval(25);
  buttonA.attach(4, INPUT_PULLUP);
  buttonB.attach(5, INPUT_PULLUP);
  buttonC.attach(6, INPUT_PULLUP);
  buttonX.attach(7, INPUT_PULLUP);
  buttonY.attach(8, INPUT_PULLUP);
  buttonZ.attach(10, INPUT_PULLUP);
  tapTempo.setMinBPM((float) 40);
  tapTempo.setMaxBPM((float) 240);
  tapTempo.setBPM(120.0);
}

float testBPM = 120.0;
void updateControl(){
  buttonA.update();
  buttonB.update();
  buttonC.update();
  buttonX.update();
  buttonY.update();
  buttonZ.update();
  tapTempo.update(!buttonA.read());
  msPerPulse = tapTempo.getBeatLength() / 24.0;
  if(buttonZ.fell()) doStartStop();
  while(beatPlaying && millis()>=nextPulseTime) {
    playPulseHits();
    incrementPulse();
    nextPulseTime = nextPulseTime + msPerPulse;
  }
}

void doStartStop() {
  beatPlaying = !beatPlaying;
  if(beatPlaying) {
    pulseNum = 0;
    stepNum = 0;
    nextPulseTime = millis();
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

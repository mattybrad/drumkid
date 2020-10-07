#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <ADSR.h>
#include <mozzi_rand.h>

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);
#define CONTROL_RATE 128

byte gain[4];
byte channel = 0;
unsigned long int nextNoteTime = 0;

ADSR <CONTROL_RATE, CONTROL_RATE> env[4];
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin4(SIN2048_DATA);

void setup(){
  startMozzi(CONTROL_RATE); // :)
  aSin1.setFreq(110);
  aSin2.setFreq(220);
  aSin3.setFreq(330);
  aSin4.setFreq(440);
  for(byte i=0;i<4;i++) {
    env[i].setADLevels(255,128);
    env[i].setTimes(3,30,100,300);
  }
}


void updateControl(){
  byte quadRand = rand(0,16);
  if(millis()>=nextNoteTime) {
    for(byte i=0;i<4;i++) {
      if(bitRead(quadRand, i)) env[i].noteOn();
    }
    nextNoteTime += 100;
  }
  for(byte i=0;i<4;i++) {
    env[i].update();
    gain[i] = env[i].next();
  }
}


int updateAudio(){
  char out = (aSin1.next() * gain[0])>>10;
  out += (aSin2.next() * gain[1])>>10;
  out += (aSin3.next() * gain[2])>>10;
  out += (aSin4.next() * gain[3])>>10;
  return out;
}


void loop(){
  audioHook();
}

#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/square_analogue512_int8.h>
#include <ADSR.h>
#include <mozzi_rand.h>

#define CONTROL_RATE 256

byte gain[4];
byte channel = 0;
unsigned long int nextNoteTime = 0;

ADSR <CONTROL_RATE, CONTROL_RATE> env[4];
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSin1(SQUARE_ANALOGUE512_DATA);
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSin2(SQUARE_ANALOGUE512_DATA);
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSin3(SQUARE_ANALOGUE512_DATA);
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSin4(SQUARE_ANALOGUE512_DATA);

void setup(){
  startMozzi(CONTROL_RATE);
  aSin1.setFreq(110);
  aSin2.setFreq(220);
  aSin3.setFreq(330);
  aSin4.setFreq(440);
  for(byte i=0;i<4;i++) {
    env[i].setADLevels(255,128);
    env[i].setTimes(3,30,100,700);
  }
}


void updateControl(){
  byte quadRand = rand(0,16);
  if(millis()>=nextNoteTime) {
    for(byte i=0;i<4;i++) {
      if(bitRead(quadRand, i)) {
        int randFreq = rand(60,500);
        if(i==0) aSin1.setFreq(randFreq);
        else if(i==1) aSin2.setFreq(randFreq);
        else if(i==2) aSin3.setFreq(randFreq);
        else if(i==3) aSin4.setFreq(randFreq);
        env[i].noteOn();
      }
    }
    nextNoteTime += 50;
  }
  for(byte i=0;i<4;i++) {
    env[i].update();
    gain[i] = env[i].next();
  }
}


int updateAudio(){
  long int out = (aSin1.next() * gain[0]);
  out += (aSin2.next() * gain[1]);
  out += (aSin3.next() * gain[2]);
  out += (aSin4.next() * gain[3]);
  return (out>>10);
}


void loop(){
  audioHook();
}

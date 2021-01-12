#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/square_analogue512_int8.h>
#include <ADSR.h>
#include <mozzi_rand.h>

#define CONTROL_RATE 256

// define how many pins are used for DrumKid's LEDs, buttons, and potentiometers
#define NUM_LED_PINS 5
#define NUM_BUTTON_PINS 6
#define NUM_ANALOG_PINS 4

// define which pins are used
const byte ledPins[NUM_LED_PINS] = {2,3,11,12,13};
const byte buttonPins[NUM_BUTTON_PINS] = {4,5,6,7,8,10};
const byte analogPins[NUM_ANALOG_PINS] = {A0,A1,A2,A3};

byte gain[4];
byte channel = 0;
unsigned long int nextNoteTime = 0;

const float noteFreqs[88] = {
  27.50, 29.14, 30.87, 32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.25,
  49.00, 51.91, 55.00, 58.27, 61.74, 65.41, 69.30, 73.42, 77.78, 82.41,
  87.31, 92.50, 98.00, 103.83, 110.00, 116.54, 123.47, 130.81, 138.59,
  146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08,
  246.94, 261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00,
  415.30, 440.00, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 659.26,
  698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77, 1046.50, 1108.73,
  1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22, 1760.00,
  1864.66, 1975.53, 2093.00, 2217.46, 2349.32, 2489.02, 2637.02, 2793.83,
  2959.96, 3135.96, 3322.44, 3520.00, 3729.31, 3951.07, 4186.01
};
const byte majorScale[7] = {0,2,4,5,7,9,11};

const byte chords[6][3] = {
  {0,4,7},
  {2,5,9},
  {4,7,11},
  {5,9,0},
  {7,11,2},
  {9,0,4}
};

ADSR <CONTROL_RATE, CONTROL_RATE> env[4];
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSin1(SQUARE_ANALOGUE512_DATA);
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSin2(SQUARE_ANALOGUE512_DATA);
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSin3(SQUARE_ANALOGUE512_DATA);
Oscil <SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aSin4(SQUARE_ANALOGUE512_DATA);

void setup(){
  for(byte i=0; i<NUM_BUTTON_PINS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  for(byte i=0; i<NUM_LED_PINS; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
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

byte currentChord = 0;
void updateControl(){
  
  byte chance = mozziAnalogRead(A0);
  byte range = map(mozziAnalogRead(A2),0,1023,0,44);
  byte midpoint = map(mozziAnalogRead(A3),0,1023,0,88);

  for(byte i=0; i<NUM_BUTTON_PINS; i++) {
    if(!digitalRead(buttonPins[i])) currentChord = i;
  }
  
  if(millis()>=nextNoteTime) {
    for(byte i=0;i<4;i++) {
      byte onOffRand = rand(0,1024);
      if(onOffRand<chance) {
        //byte randNote = rand(max(0,midpoint-range),min(40,midpoint+range));
        //float randFreq = noteFreqs[chords[0][randNote%3]+12*(randNote/3)];
        byte randNote = rand(max(0,midpoint-range),min(88,midpoint+range));
        float randFreq = noteFreqs[findClosestNoteInChord(randNote)];
        if(i==0) aSin1.setFreq(randFreq);
        else if(i==1) aSin2.setFreq(randFreq);
        else if(i==2) aSin3.setFreq(randFreq);
        else if(i==3) aSin4.setFreq(randFreq);
        env[i].noteOn();
      }
    }
    nextNoteTime += 200;
  }
  for(byte i=0;i<4;i++) {
    env[i].update();
    gain[i] = env[i].next();
  }
}

int findClosestNoteInChord(int rawNote) {
  int closestNoteDistance = 255;
  byte closestNote = 0;
  int thisDistance;
  for(byte i=0; i<3; i++) {
    thisDistance = abs(chords[currentChord][i] - (rawNote%12));
    if(thisDistance < closestNoteDistance) {
      closestNote = i;
      closestNoteDistance = thisDistance;
    }
  }
  return 12*(rawNote/12) + chords[currentChord][closestNote];
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

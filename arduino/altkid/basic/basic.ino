#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator

#define NUM_LED_PINS 5
#define NUM_BUTTON_PINS 6
#define NUM_ANALOG_PINS 4

const byte ledPins[NUM_LED_PINS] = {2,3,11,12,13};
const byte buttonPins[NUM_BUTTON_PINS] = {4,5,6,7,8,10};
const byte analogPins[NUM_ANALOG_PINS] = {A0,A1,A2,A3};

const float noteFrequencies[NUM_BUTTON_PINS] = {220.0,246.94,277.18,311.13,349.23,392.00};

#define CONTROL_RATE 64

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);

void setup(){
  for(byte i=0; i<NUM_BUTTON_PINS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  startMozzi(CONTROL_RATE);
  aSin.setFreq(noteFrequencies[0]);
}


void updateControl(){
  for(byte i=0; i<NUM_BUTTON_PINS; i++) {
    if(!digitalRead(buttonPins[i])) aSin.setFreq(noteFrequencies[i]);
  }
}


int updateAudio(){
  return aSin.next();
}


void loop(){
  audioHook();
}

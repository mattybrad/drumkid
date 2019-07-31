#include <MozziGuts.h>
#include <Sample.h>
#include <EventDelay.h>
#include <mozzi_rand.h>

#define CONTROL_RATE 256
// temp
int ledPins[] = {2,3,4,5,6};
int buttonPins[] = {7,8,10,11,12,13};
//int ledPins[] = {2,3,4,5,13};
//int buttonPins[] = {7,8,10,11,12,6};

void setup() {
  startMozzi(CONTROL_RATE);
  randSeed();
  for(int i=0;i<5;i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void loop() {
  audioHook();
}

void updateControl() {
  for(int i=0;i<5;i++) {
    digitalWrite(ledPins[i], !digitalRead(buttonPins[i]));
  }
}

int updateAudio() {
  return 0;
}

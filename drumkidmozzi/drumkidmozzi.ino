// include relevant mozzi libraries
#include <MozziGuts.h>
#include <Sample.h>
#include <LowPassFilter.h>
#include <EventDelay.h>
#include <mozzi_rand.h>

// include other stuff
#include <Bounce2.h>

// include custom sample files
#include "kick.h"
#include "closedhat.h"
#include "snare.h"

// define some numbers etc
#define CONTROL_RATE 64
#define START_STOP_PIN 2
#define SHIFT_PIN 4
#define BUTTON_A_PIN 7
#define BUTTON_B_PIN 8
#define BUTTON_C_PIN 12

#define LED_1 3
#define LED_2 5
#define LED_3 6
#define LED_4 10
#define LED_5 11

// define buttons
Bounce buttonA = Bounce();
Bounce buttonB = Bounce();
Bounce buttonC = Bounce();
Bounce startStopButton = Bounce();
Bounce shiftButton = Bounce();

// define samples
Sample <kick_NUM_CELLS, AUDIO_RATE> aSample(kick_DATA);
Sample <closedhat_NUM_CELLS, AUDIO_RATE> bSample(closedhat_DATA);
Sample <snare_NUM_CELLS, AUDIO_RATE> cSample(snare_DATA);

// define other mozzi things
LowPassFilter lpf;
EventDelay kTriggerDelay;

byte bitCrushLevel; // between 0 and 7
byte bitCrushCompensation;
int beatTime = 150;
byte beatIndex = 0;
float tempo = 120;
float blend = 0.5;
byte controlSet = 0;
byte ledPins[] = {LED_1,LED_2,LED_3,LED_4,LED_5};

int storedValues[6][3] = {  {0,0,0},
                            {0,0,0},
                            {0,0,0},
                            {0,0,0},
                            {0,0,0},
                            {0,0,0},};

// define beats (temporarily here, probably move to a separate file later?)
byte beat1[][16] = {  {255,0,0,0,0,0,0,0,255,0,255,0,64,128,212,255,},
                      {64,0,64,0,64,0,64,64,0,64,64,0,64,0,64,32,},
                      {0,0,0,0,255,0,0,0,0,0,0,0,255,0,64,32,},};

byte beat2[][16] = {  {255,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,},
                      {255,0,128,0,255,0,128,0,255,0,128,0,255,0,128,0,},
                      {0,0,0,0,255,0,0,0,0,0,0,0,255,0,0,0,},};

byte blendedBeat[3][16];
byte hyper = 0;


void setup(){
  pinMode(LED_1,OUTPUT);
  pinMode(LED_2,OUTPUT);
  pinMode(LED_3,OUTPUT);
  pinMode(LED_4,OUTPUT);
  pinMode(LED_5,OUTPUT);
  digitalWrite(LED_1,HIGH);
  delay(200);
  digitalWrite(LED_1,LOW);
  digitalWrite(LED_2,HIGH);
  delay(200);
  digitalWrite(LED_2,LOW);
  digitalWrite(LED_3,HIGH);
  delay(200);
  digitalWrite(LED_3,LOW);
  digitalWrite(LED_4,HIGH);
  delay(200);
  digitalWrite(LED_4,LOW);
  digitalWrite(LED_5,HIGH);
  delay(200);
  digitalWrite(LED_5,LOW);
  startStopButton.attach(START_STOP_PIN, INPUT_PULLUP);
  shiftButton.attach(SHIFT_PIN, INPUT_PULLUP);
  buttonA.attach(BUTTON_A_PIN, INPUT_PULLUP);
  buttonB.attach(BUTTON_B_PIN, INPUT_PULLUP);
  buttonC.attach(BUTTON_C_PIN, INPUT_PULLUP);
  startStopButton.interval(25);
  shiftButton.interval(25);
  buttonA.interval(25);
  buttonB.interval(25);
  buttonC.interval(25);
  startMozzi(CONTROL_RATE);
  aSample.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  bSample.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  cSample.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  aSample.setEnd(7000); // was getting a funny click at the end of the kick sample
  lpf.setResonance(200);
  lpf.setCutoffFreq(255);
  kTriggerDelay.set(beatTime);
  randSeed();
}

bool started = false;
byte aVol;
byte bVol;
byte cVol;
void updateControl(){
  bool startNow = false;
  buttonA.update();
  buttonB.update();
  buttonC.update();
  startStopButton.update();
  shiftButton.update();

  if(buttonA.fell()) controlSet = 0 + (!shiftButton.read()?3:0);
  else if(buttonB.fell()) controlSet = 1 + (!shiftButton.read()?3:0);
  else if(buttonC.fell()) controlSet = 2 + (!shiftButton.read()?3:0);
  
  // attempt blending beat
  for(byte i=0;i<3;i++) {
    for(byte j=0;j<16;j++) {
      blendedBeat[i][j] = constrain(blend * beat1[i][j] + (1-blend) * beat2[i][j] + rand(0, hyper), 0, 255);
      //blendedBeat[i][j] = rand(0,255);
    }
  }
  
  if((kTriggerDelay.ready() && started) || startNow){
    if(blendedBeat[0][beatIndex]>0) aSample.start();
    if(blendedBeat[1][beatIndex]>0) bSample.start();
    if(blendedBeat[2][beatIndex]>0) cSample.start();
    if(blendedBeat[0][beatIndex]>0) aVol = blendedBeat[0][beatIndex];
    if(blendedBeat[1][beatIndex]>0) bVol = blendedBeat[1][beatIndex];
    if(blendedBeat[2][beatIndex]>0) cVol = blendedBeat[2][beatIndex];
    for(int i=0;i<5;i++) digitalWrite(ledPins[i], LOW);
    if(beatIndex%4==0) digitalWrite(ledPins[beatIndex/4], HIGH);
    beatIndex ++;
    beatIndex = beatIndex % 16;
    kTriggerDelay.start(beatTime);
  }
  if(startStopButton.fell()) {
    started = !started;
    if(started) {
      beatIndex = 0;
      startNow = true;
    }
  }
  switch(controlSet) {
    case 0:
    hyper = mozziAnalogRead(0)>>2;
    //ceiling = mozziAnalogRead(1)>>2;
    //zoom = mozziAnalogRead(2)>>2;
    break;
    case 1:
    aSample.setFreq(((float) mozziAnalogRead(0) / 255.0f) * (float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
    bSample.setFreq(((float) mozziAnalogRead(0) / 255.0f) * (float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
    cSample.setFreq(((float) mozziAnalogRead(0) / 255.0f) * (float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
    lpf.setCutoffFreq(mozziAnalogRead(1)>>2);
    bitCrushLevel = 7-(mozziAnalogRead(2)>>7);
    bitCrushCompensation = bitCrushLevel;
    if(bitCrushLevel >= 6) bitCrushCompensation --;
    if(bitCrushLevel >= 7) bitCrushCompensation --;
    break;
    case 2:
    // could probably optimise this maths
    tempo = 40.0 + mozziAnalogRead(0) / 5.0;
    beatTime = 15.0 / (tempo/1000.0);
    break;
    case 3:

    break;
    case 4:

    break;
    case 5:
    blend = mozziAnalogRead(0)/1023.0;
    break;
  }
}

int updateAudio(){
  //char asig = lpf.next((aSample.next()>>1)+(bSample.next()>>1)+(cSample.next()>>1));
  //char asig = lpf.next(((aVol*aSample.next())>>9));
  char asig = lpf.next(((aVol*aSample.next())>>9)+((bVol*bSample.next())>>9)+((cVol*cSample.next())>>9));
  asig = (asig>>bitCrushLevel)<<bitCrushCompensation;
  return (int) asig;
}


void loop(){
  audioHook();
}




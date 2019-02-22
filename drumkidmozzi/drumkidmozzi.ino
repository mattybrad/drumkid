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
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define BUTTON_C_PIN 7
#define BUTTON_D_PIN 8

// define buttons
Bounce startStopButton = Bounce();
Bounce buttonA = Bounce();
Bounce buttonB = Bounce();
Bounce buttonC = Bounce();
Bounce buttonD = Bounce();

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

// define beats (temporarily here, probably move to a separate file later?)
bool beat1[][16] = {  {1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,1,},
                      {1,0,1,0,1,0,1,1,0,1,1,0,1,0,1,0,},
                      {0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,},};

void setup(){
  for(int i=2;i<=8;i++) {
    pinMode(i,INPUT_PULLUP);
  }
  startStopButton.attach(START_STOP_PIN, INPUT_PULLUP);
  buttonA.attach(BUTTON_A_PIN, INPUT_PULLUP);
  buttonB.attach(BUTTON_B_PIN, INPUT_PULLUP);
  buttonC.attach(BUTTON_C_PIN, INPUT_PULLUP);
  buttonD.attach(BUTTON_D_PIN, INPUT_PULLUP);
  startStopButton.interval(25);
  buttonA.interval(25);
  buttonB.interval(25);
  buttonC.interval(25);
  buttonD.interval(25);
  startMozzi(CONTROL_RATE);
  aSample.setFreq((float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
  bSample.setFreq((float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
  cSample.setFreq((float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  aSample.setEnd(7000); // was getting a funny click at the end of the kick sample
  lpf.setResonance(200);
  lpf.setCutoffFreq(255);
  kTriggerDelay.set(beatTime);
}

bool started = false;
void updateControl(){
  bool startNow = false;
  startStopButton.update();
  if(startStopButton.fell()) {
    started = !started;
    if(started) {
      beatIndex = 0;
      startNow = true;
    }
  }
  buttonA.update();
  buttonB.update();
  buttonC.update();
  buttonD.update();
  if(startStopButton.fell()) {
    // do something
  }
  if((kTriggerDelay.ready() && started) || startNow){
    //byte r = rand(3);
    if(beat1[0][beatIndex]) aSample.start();
    if(beat1[1][beatIndex]) bSample.start();
    if(beat1[2][beatIndex]) cSample.start();
    beatIndex ++;
    beatIndex = beatIndex % 16;
    kTriggerDelay.start(beatTime);
  }
  if(!buttonA.read()) lpf.setCutoffFreq(mozziAnalogRead(0)>>2);
  if(!buttonB.read()) {
    bitCrushLevel = 7-(mozziAnalogRead(0)>>7);
    bitCrushCompensation = bitCrushLevel;
    if(bitCrushLevel >= 6) bitCrushCompensation --;
    if(bitCrushLevel >= 7) bitCrushCompensation --;
  }
  if(!buttonC.read()) beatTime = 20 + mozziAnalogRead(0); // temp
  if(!buttonD.read()) {
    aSample.setFreq(((float) mozziAnalogRead(0) / 255.0f) * (float) kick_SAMPLERATE / (float) kick_NUM_CELLS);
    bSample.setFreq(((float) mozziAnalogRead(0) / 255.0f) * (float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS);
    cSample.setFreq(((float) mozziAnalogRead(0) / 255.0f) * (float) snare_SAMPLERATE / (float) snare_NUM_CELLS);
  }
}


int updateAudio(){
  char asig = lpf.next((aSample.next()>>1)+(bSample.next()>>1)+(cSample.next()>>1));
  asig = (asig>>bitCrushLevel)<<bitCrushCompensation;
  return (int) asig;
}


void loop(){
  audioHook();
}




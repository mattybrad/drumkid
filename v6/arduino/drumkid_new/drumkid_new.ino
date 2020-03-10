/*  This code is for the version 6 (V6) DrumKid PCB and breadboard designs.
 *  It might work with other versions with a bit of tweaking   
 */

#include <MozziGuts.h>
#include <Sample.h>
#include <mozzi_rand.h>

#include "beats.h"

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

#define CONTROL_RATE 256 // Hz, aiming for 256 to keep up with high tempos
#define NUM_KNOBS 4
#define NUM_LEDS 5
#define NUM_BUTTONS 6
#define NUM_SAMPLES_TOTAL 5 // total number of samples

const byte ledPins[5] = {2,3,11,12,13};
const byte buttonPins[6] = {4,5,6,7,8,10};
const byte analogPins[4] = {0,1,2,3};

float nextPulseTime = 0.0;
float msPerPulse = 20.8333; // 120bpm
byte pulseNum = 0; // 0 to 23 (24ppqn, pulses per quarter note)
byte stepNum = 0; // 0 to 32 (max two bars of 8 beats, aka 32 16th-notes)
bool beatPlaying = false;
byte noteDown = B00000000;
bool syncReceived = false;

const byte midiNotes[NUM_SAMPLES_TOTAL] = {36,42,38,37,43};
const byte zoomValues[] = {32,16,8,4,2,1}; // could be compressed?
byte sampleVolumes[NUM_SAMPLES_TOTAL] = {255,255,255,255,255}; // temp

void setup(){
  byte i;
  for(i=0;i<NUM_LEDS;i++) {
    pinMode(ledPins[i],OUTPUT);
  }
  for(i=0;i<NUM_BUTTONS;i++) {
    pinMode(buttonPins[i],INPUT_PULLUP);
  }
  startMozzi(CONTROL_RATE);
  randSeed();
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
  Serial.begin(31250);
  flashLeds(); // remove if low on space later
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
  while(!syncReceived && beatPlaying && millis()>=nextPulseTime) {
    Serial.write(0xF8); // MIDI clock continue
    cancelMidiNotes();
    updateLeds();
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
    Serial.write(0xFA); // MIDI clock start
  } else {
    Serial.write(0xFC); // MIDI clock stop
    updateLeds();
  }
}

void playPulseHits() {
  if(pulseNum % 6 == 0) {
    for(byte i=0; i<5; i++) {
      calculateNote(i);
    }
  }
}

byte paramZoom = 200;
byte paramChance = 64;
byte paramMidpoint = 127;
byte paramRange = 127;
byte paramBeat = 2;
void calculateNote(byte sampleNum) {
  byte zoomValueIndex = paramZoom / 51; // gives value from 0 to 5
  byte zoomVelocity = paramZoom % 51; // 
  if(zoomValueIndex == 5) {
    zoomValueIndex = 4;
    zoomVelocity = 51;
  }
  byte lowerZoomValue = zoomValues[zoomValueIndex]; // e.g. 8 for a quarter note
  byte upperZoomValue = zoomValues[zoomValueIndex+1]; // e.g. 16 for a quarter note
  long thisVelocity = 0;
  if(stepNum%2==0) {
    // beats only defined down to 16th notes not 32nd, hence %2
    byte beatByte = pgm_read_byte(&beats[paramBeat][sampleNum][stepNum/16]);
    if(bitRead(beatByte,7-((stepNum/2)%8))) thisVelocity = 255;
  }
  if(thisVelocity==0) {
    // for steps not defined in beat, use algorithm to determine velocity
    if(stepNum%upperZoomValue==0) {
      // if step length is longer(?) than is affected by zoom
      byte yesNoRand = rand(0,255);
      if(yesNoRand < paramChance) {
        //int velocityRand = rand(0,255);
        //velocityRand = 255;
        //thisVelocity = velocityRand;
        //thisVelocity = paramMidpoint - paramRange/2 + ((velocityRand * paramRange)>>8);
        int lowerBound = paramMidpoint - paramRange/2;
        int upperBound = paramMidpoint + paramRange/2;
        thisVelocity = rand(lowerBound, upperBound);
        thisVelocity = constrain(thisVelocity,0,255);
        if(stepNum%lowerZoomValue!=0) {
          //Serial.println(thisVelocity);
          thisVelocity = thisVelocity * 5 * zoomVelocity / 255;
          //Serial.println(thisVelocity);
          //Serial.println("");
        }
      }
    }
  } 
  triggerNote(sampleNum, thisVelocity);
}

void triggerNote(byte sampleNum, byte velocity) {
  if(velocity>8) { // don't bother with very quiet notes
    switch(sampleNum) {
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
    sampleVolumes[sampleNum] = velocity;
    bitWrite(noteDown,sampleNum,true);
    playMidiNote(midiNotes[sampleNum], velocity);
  }
}

void playMidiNote(byte noteNum, byte velocity) {
  Serial.write(0x90);
  Serial.write(noteNum);
  Serial.write(velocity>>1);
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

void cancelMidiNotes() {
  byte i;
  for(i=0; i<NUM_SAMPLES_TOTAL; i++) {
    if(bitRead(noteDown,i)) {
      Serial.write(0x90);
      Serial.write(midiNotes[i]);
      Serial.write(0x00);
      bitWrite(noteDown,i,false);
    }
  }
}

const byte atten = 9;
int updateAudio(){
  //char asig = (kick.next() >> 2) + (closedhat.next() >> 3) + (snare.next() >> 2) + (rim.next() >> 1) + (tom.next() >> 2);
  //return asig; // return an int signal centred around 0

  char asig = ((sampleVolumes[0]*kick.next())>>atten)+
    ((sampleVolumes[1]*closedhat.next())>>atten)+
    ((sampleVolumes[2]*snare.next())>>atten)+
    ((sampleVolumes[3]*rim.next())>>atten)+
    ((sampleVolumes[4]*tom.next())>>atten);
  return asig;
}

byte thisMidiByte;
void loop(){
  audioHook(); // main Mozzi function, calls updateAudio and updateControl
  while(Serial.available()) {
    thisMidiByte = Serial.read();
    if(thisMidiByte==0xF8) {
      syncReceived = true;
      if(beatPlaying) {
        cancelMidiNotes();
        playPulseHits();
        incrementPulse();
      }
    }
  }
}

void flashLeds() {
  byte i, j;
  for(i=0;i<7;i++) {
    for(j=0;j<NUM_LEDS;j++) {
      digitalWrite(ledPins[j],(i+j)%2&&i<6);
    }
    delay(100);
  }
}

void updateLeds() {
  byte i;
  for(i=0;i<NUM_LEDS;i++) {
    if(beatPlaying) {
      switch(pulseNum%24) {
        case 0:
        if(i==stepNum/4) digitalWrite(ledPins[i], HIGH);
        break;
        case 12:
        if(i==stepNum/4) digitalWrite(ledPins[i], LOW);
        break;
      }
    } else {
      digitalWrite(ledPins[i], LOW);
    }
  }
}

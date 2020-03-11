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

// include EEPROM library for saving data
#include <EEPROM.h>

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
#define NUM_PARAM_GROUPS 5
#define SAVED_STATE_SLOT_BYTES 32

const byte ledPins[5] = {2,3,11,12,13};
const byte buttonPins[6] = {4,5,6,7,8,10};
const byte analogPins[4] = {3,2,1,0}; // unnecessary? remove, save four bytes?

float nextPulseTime = 0.0;
float msPerPulse = 20.8333; // 120bpm
byte pulseNum = 0; // 0 to 23 (24ppqn, pulses per quarter note)
byte stepNum = 0; // 0 to 64 (max two bars of 8 beats, aka 32 32nd-notes)
bool beatPlaying = false;
byte noteDown = B00000000;
bool syncReceived = false;

const byte midiNotes[NUM_SAMPLES_TOTAL] = {36,42,38,37,43};
const byte zoomValues[] = {32,16,8,4,2,1}; // could be compressed?
byte sampleVolumes[NUM_SAMPLES_TOTAL] = {255,255,255,255,255}; // temp

byte storedValues[NUM_PARAM_GROUPS*NUM_KNOBS]; // analog knob values
byte firstLoop = true;
byte secondLoop = false;
byte controlSet = 0;
bool knobLocked[NUM_KNOBS] = {true,true,true,true}; // bad use of space, use single byte instead
byte analogValues[NUM_KNOBS] = {0,0,0,0};
byte initValues[NUM_KNOBS] = {0,0,0,0};

bool readyToSave = true;
bool readyToChooseSaveSlot = false;
bool readyToLoad = true;
bool readyToChooseLoadSlot = false;
bool newStateLoaded = false;

// parameters, 0-255 unless otherwise noted
byte paramChance = 64;
byte paramZoom = 200;
byte paramMidpoint = 127;
byte paramRange = 127;

byte paramPitch = 127;
byte paramCrush = 0;
byte paramCrop = 0;
byte paramGlitch = 0;

byte paramBeat = 2; // 0 to 23
byte paramTimeSignature = 4;
// N.B. no such variable as paramTempo - handled by TapTempo library
byte paramDrift = 0; // to be scrapped?

byte crushCompensation;

#define MIN_TEMPO 40
#define MAX_TEMPO 295

#if MIN_TEMPO + 255 != MAX_TEMPO
#error "Tempo must have a range of exactly 255 otherwise I'll have to write more code"
#endif

// define which knob controls which parameter
// e.g. 0 is group 1 knob 1, 6 is group 2 knob 3
#define CHANCE 0
#define ZOOM 1
#define MIDPOINT 2
#define RANGE 3

#define PITCH 4
#define CRUSH 5
#define CROP 6
#define GLITCH 7

#define BEAT 12
#define TEMPO 13
#define TIME_SIGNATURE 14
#define DRIFT 15

void setup(){
  byte i;
  for(i=0;i<NUM_LEDS;i++) {
    pinMode(ledPins[i],OUTPUT);
  }
  for(i=0;i<NUM_BUTTONS;i++) {
    pinMode(buttonPins[i],INPUT_PULLUP);
  }
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
  
  storedValues[CHANCE] = 0;
  storedValues[ZOOM] = 0;
  storedValues[RANGE] = 0;
  storedValues[MIDPOINT] = 0;
  
  storedValues[PITCH] = 130;
  storedValues[CRUSH] = 0;
  storedValues[CROP] = 0;
  storedValues[GLITCH] = 0;
  
  tapTempo.setMinBPM((float) MIN_TEMPO);
  tapTempo.setMaxBPM((float) MAX_TEMPO);
  tapTempo.setBPM(120.0);

  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    updateParameters(i);
  }
  
  Serial.begin(31250);
  flashLeds(); // remove if low on space later
}

float testBPM = 120.0;
void updateControl(){
  byte i;
  bool controlSetChanged = false;
  newStateLoaded = false;
  
  buttonA.update();
  buttonB.update();
  buttonC.update();
  buttonX.update();
  buttonY.update();
  buttonZ.update();

  if(!buttonB.read() && !buttonC.read()) {
    readyToLoad = false;
    readyToSave = false;
    readyToChooseLoadSlot = true;
    readyToChooseSaveSlot = false;
    tapTempo.update(false);
  } else if(!buttonX.read() && !buttonY.read()) {
    readyToSave = false;
    readyToLoad = false;
    readyToChooseSaveSlot = true;
    readyToChooseLoadSlot = false;
    tapTempo.update(false);
  } else {
    readyToSave = true;
    readyToLoad = true;
    if(!readyToChooseLoadSlot&&!readyToChooseSaveSlot) {
      tapTempo.update(!buttonA.read());
    } else {
      tapTempo.update(false);
    }

    // handle button presses
    byte prevControlSet = controlSet;
    if(buttonA.fell()) {
      if(readyToChooseLoadSlot) loadParams(0);
      else if(readyToChooseSaveSlot) saveParams(0);
      else {
        controlSet = 0;
        storedValues[TEMPO] = tapTempo.getBPM() - MIN_TEMPO;
      }
    } else if(buttonB.fell()) {
      if(readyToChooseLoadSlot) loadParams(1);
      else if(readyToChooseSaveSlot) saveParams(1);
      else controlSet = 1;
    } else if(buttonC.fell()) {
      if(readyToChooseLoadSlot) loadParams(2);
      else if(readyToChooseSaveSlot) saveParams(2);
      else controlSet = 2;
    } else if(buttonX.fell()) {
      if(readyToChooseLoadSlot) loadParams(3);
      else if(readyToChooseSaveSlot) saveParams(3);
      else controlSet = 3;
    } else if(buttonY.fell()) {
      if(readyToChooseLoadSlot) loadParams(4);
      else if(readyToChooseSaveSlot) saveParams(4);
      else controlSet = 4;
    }
    controlSetChanged = (prevControlSet != controlSet);
    
    if(buttonZ.fell()) {
      if(readyToChooseLoadSlot) loadParams(5);
      else if(readyToChooseSaveSlot) saveParams(5);
      else doStartStop();
    }
    
  }

  //if(buttonZ.fell()) doStartStop();
  //tapTempo.update(!buttonA.read());
  
  msPerPulse = tapTempo.getBeatLength() / 24.0;
  while(!syncReceived && beatPlaying && millis()>=nextPulseTime) {
    Serial.write(0xF8); // MIDI clock continue
    cancelMidiNotes();
    updateLeds();
    playPulseHits();
    incrementPulse();
    nextPulseTime = nextPulseTime + msPerPulse;
  }

  for(i=0;i<NUM_KNOBS;i++) {
    if(firstLoop) {
      byte dummyReading = mozziAnalogRead(analogPins[i]);
    } else {
      analogValues[i] = mozziAnalogRead(analogPins[i])>>2;
    }
  }
  if(controlSetChanged||secondLoop||newStateLoaded) {
    for(i=0;i<NUM_KNOBS;i++) {
      knobLocked[i] = true;
      initValues[i] = analogValues[i];
    }
  } else {
    for(i=0;i<NUM_KNOBS;i++) {
      if(knobLocked[i]) {
        int diff = initValues[i] - analogValues[i];
        if(diff<-5||diff>5) knobLocked[i] = false;
      }
      if(!knobLocked[i]) {
        storedValues[NUM_KNOBS*controlSet+i] = analogValues[i];
      }
    }
  }
  if(secondLoop) {
    for(i=0;i<NUM_PARAM_GROUPS;i++) {
      updateParameters(i);
    }
  } else {
    updateParameters(controlSet);
  }

  if(firstLoop) {
    firstLoop = false;
    secondLoop = true;
  } else {
    secondLoop = false;
  }
}

void updateParameters(byte thisControlSet) {
  switch(thisControlSet) {
    case 0:
    paramChance = storedValues[CHANCE];
    paramZoom = storedValues[ZOOM];
    paramRange = storedValues[RANGE];
    paramMidpoint = storedValues[MIDPOINT];
    break;

    case 1:
    {
      // pitch alteration (could do this in a more efficient way to reduce RAM usage)
      float thisPitch = fabs((float) storedValues[PITCH] * (8.0f/255.0f) - 4.0f);
      float newKickFreq = thisPitch * (float) kick_SAMPLERATE / (float) kick_NUM_CELLS;
      float newHatFreq = thisPitch * (float) closedhat_SAMPLERATE / (float) closedhat_NUM_CELLS;
      float newSnareFreq = thisPitch * (float) snare_SAMPLERATE / (float) snare_NUM_CELLS;
      float newRimFreq = thisPitch * (float) rim_SAMPLERATE / (float) rim_NUM_CELLS;
      float newTomFreq = thisPitch * (float) tom_SAMPLERATE / (float) tom_NUM_CELLS;
      kick.setFreq(newKickFreq);
      closedhat.setFreq(newHatFreq);
      snare.setFreq(newSnareFreq);
      rim.setFreq(newRimFreq);
      tom.setFreq(newTomFreq);
      /*bool thisDirection = driftValue(PARAM_PITCH,2) >= 128;
      kick.setDirection(thisDirection);
      closedhat.setDirection(thisDirection);
      snare.setDirection(thisDirection);
      rim.setDirection(thisDirection);
      tom.setDirection(thisDirection);*/
      
      // bit crush! high value = clean (8 bits), low value = dirty (1 bit?)
      paramCrush = 7-(storedValues[CRUSH]>>5);
      crushCompensation = paramCrush;
      if(paramCrush >= 6) crushCompensation --;
      if(paramCrush >= 7) crushCompensation --;

      // crop - a basic effect to chop off the end of each sample for a more staccato feel
      paramCrop = storedValues[CRUSH];
      /*kick.setEnd(thisDirection ? map(paramCrop,0,255,100,kick_NUM_CELLS) : kick_NUM_CELLS);
      closedhat.setEnd(thisDirection ? map(paramCrop,0,255,100,closedhat_NUM_CELLS) : closedhat_NUM_CELLS);
      snare.setEnd(thisDirection ? map(paramCrop,0,255,100,snare_NUM_CELLS) : snare_NUM_CELLS);
      rim.setEnd(thisDirection ? map(paramCrop,0,255,100,rim_NUM_CELLS) : rim_NUM_CELLS);
      tom.setEnd(thisDirection ? map(paramCrop,0,255,100,tom_NUM_CELLS) : tom_NUM_CELLS);
      kick.setStart(!thisDirection ? map(paramCrop,255,0,100,kick_NUM_CELLS) : 0);
      closedhat.setStart(!thisDirection ? map(paramCrop,255,0,100,closedhat_NUM_CELLS) : 0);
      snare.setStart(!thisDirection ? map(paramCrop,255,0,100,snare_NUM_CELLS) : 0);
      rim.setStart(!thisDirection ? map(paramCrop,255,0,100,rim_NUM_CELLS) : 0);
      tom.setStart(!thisDirection ? map(paramCrop,255,0,100,tom_NUM_CELLS) : 0);*/
      
      // experimental glitch effect
      //paramGlitch = driftValue(PARAM_GLITCH,2);
    }
    break;

    case 2:
    // slop, swing, etc
    break;

    case 3:
    paramBeat = (NUM_BEATS * (int) storedValues[BEAT]) / 256;
    tapTempo.setBPM((float) MIN_TEMPO + ((float) storedValues[TEMPO]));
    paramTimeSignature = map(storedValues[TIME_SIGNATURE],0,256,4,8);
    paramDrift = storedValues[DRIFT]; // to be scrapped?
    break;
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
  if(pulseNum % 3 == 0) {
    for(byte i=0; i<5; i++) {
      calculateNote(i);
    }
  }
}

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
  if(pulseNum % 3 == 0) {
    stepNum ++;
    if(stepNum == 64) {
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
  asig = (asig>>paramCrush)<<crushCompensation;
  return asig;
}

byte thisMidiByte;
void loop(){
  audioHook(); // main Mozzi function, calls updateAudio and updateControl
  while(Serial.available()) {
    thisMidiByte = Serial.read();
    if(thisMidiByte==0xFA) {
      // start beat (redo this later, bit hacky, separate start/stop funcs)
      beatPlaying = false;
      doStartStop();
    } else if(thisMidiByte==0xFC) {
      // stop beat (also hacky)
      //beatPlaying = true;
      //doStartStop();
    } else if(thisMidiByte==0xF8) {
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
        if(i==stepNum/8) digitalWrite(ledPins[i], HIGH);
        break;
        case 12:
        if(i==stepNum/8) digitalWrite(ledPins[i], LOW);
        break;
      }
    } else {
      digitalWrite(ledPins[i], LOW);
    }
  }
}

void loadParams(byte slotNum) {
  readyToChooseLoadSlot = false;
  newStateLoaded = true;
  for(byte i=0; i<NUM_PARAM_GROUPS*NUM_KNOBS; i++) {
    byte thisValue = EEPROM.read(slotNum*SAVED_STATE_SLOT_BYTES+i);
    storedValues[i] = thisValue;
  }
  for(byte i=0;i<NUM_PARAM_GROUPS;i++) {
    updateParameters(i);
  }
}

void saveParams(byte slotNum) {
  // 1024 possible byte locations
  // currently need 20 bytes per saved state
  // currently planning choice of 6 slots for saved states (1 per button)
  // need to leave space for some possible extra saved state data
  // allot 32 bytes per saved state (nice round number), taking up total of 192 bytes
  readyToChooseSaveSlot = false;
  for(byte i=0; i<NUM_PARAM_GROUPS*NUM_KNOBS; i++) {
    EEPROM.write(slotNum*SAVED_STATE_SLOT_BYTES+i, storedValues[i]);
  }
}

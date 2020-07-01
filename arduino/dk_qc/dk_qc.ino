// DrumKid quality control testing sketch (V6/V7)
// This is probably not the sketch you're looking for!
// I use it to test each DrumKid unit before shipping it
// It also wipes the EEPROM (memory) so be careful ;)

// testing sketch goes through a number of phases:
// 0) flash all LEDs one by one
// 1) press buttons one by one, LEDs flash to show success (once for button 1, twice for button 2, etc)
// 2) test knobs one by one, with LEDs showing approx position for broad feedback
//    a) turn to zero (knob should register ~0)
//    b) turn to full (knob should register ~1023)
//    c) turn back to to zero (knob should register ~0)
//    d) flash appropriate LED to confirm all good
// 3) play constant sound until button is pressed
// 4) send repeated MIDI note sequence until button is pressed
// 5) play notes when MIDI input received until button is pressed

#define CLEAR_MEMORY 1

// include EEPROM library for saving data
#include <EEPROM.h>

// include debouncing library to make buttons work reliably
#include <Bounce2.h>

#include <MIDI.h>
#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <mozzi_midi.h>
#include <ADSR.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 128 // Hz, powers of 2 are most reliable

// audio sinewave oscillator
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);

// envelope generator
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

Bounce buttons[6];

int testPhase = 0;

// define pin numbers
const byte ledPins[5] = {2,3,11,12,13};
const byte buttonPins[6] = {4,5,6,7,8,10};
const byte analogPins[4] = {A0,A1,A2,A3};

// define knobs to help initialising EEPROM
#define CHANCE 0
#define ZOOM 1
#define RANGE 2
#define MIDPOINT 3
#define PITCH 4
#define CRUSH 5
#define CROP 6
#define DROP 7
#define DRONE 8
#define DRONE_MOD 9
#define DRONE_PITCH 10
#define DRONE_ROOT 11
#define BEAT 12
#define TIME_SIGNATURE 13
#define SWING 14
#define TEMPO 15

#define NUM_KNOBS 4
#define NUM_PARAM_GROUPS 4
#define SAVED_STATE_SLOT_BYTES 24
byte storedValues[NUM_PARAM_GROUPS*NUM_KNOBS];

void HandleNoteOn(byte channel, byte note, byte velocity) {
  if(testPhase==5) {
    aSin.setFreq(mtof(float(note)));
    envelope.noteOn();
    digitalWrite(ledPins[0],HIGH);
  }
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  if(testPhase==5) {
    envelope.noteOff();
    digitalWrite(ledPins[0],LOW);
  }
}

void setup() {
  if(CLEAR_MEMORY == 1) resetAllEepromSlots();
  
  // initialise pins
  for(int i=0; i<5; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  for(int i=0; i<6; i++) {
    buttons[i].interval(25);
    buttons[i].attach(buttonPins[i], INPUT_PULLUP);
    buttons[i].update();
  }

  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  // Initiate MIDI communications, listen to all channels (not needed with Teensy usbMIDI)
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();

  envelope.setADLevels(255,64);
  envelope.setTimes(50,200,10000,200); // 10000 is so the note will sustain 10 seconds unless a noteOff comes

  aSin.setFreq(440); // default frequency

  if(testPhase>=3) startMozzi(CONTROL_RATE);
}

bool buttonsTested[6];
int currentKnob = 0;
int knobPhases[4];
unsigned long nextMidiNoteTime;
void loop() {
  if(testPhase == 0) {
    // check LEDs work
    flashLeds(-1, 5);
    delay(500);
    for(int i=0; i<5; i++) {
      flashLeds(i, 5);
      delay(250);
    }
    testPhase = 1;
  } else if(testPhase == 1) {
    // check buttons work
    bool anyButtonsNotTested = false;
    for(int i=0; i<6; i++) {
      buttons[i].update();
      if(buttons[i].fell()) {
        buttonsTested[i] = true;
        if(i<5) {
          flashLeds(i, 5);
        } else {
          flashLeds(4, 2);
          flashLeds(3, 2);
          flashLeds(2, 2);
          flashLeds(1, 2);
          flashLeds(0, 2);
        }
      }
      if(!buttonsTested[i]) anyButtonsNotTested = true;
    }
    if(!anyButtonsNotTested) {
      flashLeds(-1, 5);
      testPhase = 2;
    }
  } else if(testPhase == 2) {
    int knobReading = analogRead(currentKnob);
    int ledNum = map(knobReading, 0, 1024, 0, 5);
    for(int i=0; i<5; i++) {
      digitalWrite(ledPins[i], i==ledNum);
    }
    if(knobPhases[currentKnob]==0) {
      if(knobReading == 0) knobPhases[currentKnob] = 1;
    } else if(knobPhases[currentKnob]==1) {
      if(knobReading == 1023) knobPhases[currentKnob] = 2;
    } else if(knobPhases[currentKnob]==2) {
      if(knobReading == 0) {
        knobPhases[currentKnob] = 3;
        currentKnob ++;
        if(currentKnob == 4) {
          testPhase = 3;
          startMozzi(CONTROL_RATE);
          flashLeds(-1, 10);
        } else {
          flashLeds(-1, 3);
        }
      }
    }
  } else if(testPhase >= 3) {
    audioHook();
  }
}

bool noteIsDown = false;
void updateControl(){
  if(testPhase==3) {
    buttons[0].update();
    if(buttons[0].fell()) {
      testPhase = 4;
      nextMidiNoteTime = millis();
    }
  } else if(testPhase==4) {
    if(millis() > nextMidiNoteTime) {
      if(noteIsDown) MIDI.sendNoteOff(50, 0, 1);
      else MIDI.sendNoteOn(50, 127, 1);
      noteIsDown = !noteIsDown;
      nextMidiNoteTime += 250;
    }
    buttons[0].update();
    if(buttons[0].fell()) {
      if(noteIsDown) MIDI.sendNoteOff(50, 0, 1);
      testPhase = 5;
    }
  } else if(testPhase==5) {
    MIDI.read();
    envelope.update();
  }
}


int updateAudio(){
  char asig = 0;
  if(testPhase == 3) asig = aSin.next()>>2;
  else if(testPhase == 5) asig = (envelope.next() * aSin.next())>>8;
  return asig;
}

void flashLeds(int ledNum, int numFlashes) {
  bool flashState = true;
  for(int i=0; i<numFlashes*2; i++) {
    if(i>0) delay(50);
    for(int j=0; j<5; j++) {
      if(j==ledNum || ledNum==-1) {
        digitalWrite(ledPins[j], flashState);
      }
    }
    flashState = !flashState;
  }
}

void resetAllEepromSlots() {
  // write semi-random (curated random?!) data to the EEPROM
  randomSeed(analogRead(5)); // A5 should be floating hence random
  for(byte i=0; i<6; i++) {
    for(byte j=0; j<6; j++) {
      storedValues[CHANCE] = random(64,192);
      storedValues[ZOOM] = random(100,190);
      storedValues[RANGE] = random(0,128);
      storedValues[MIDPOINT] = random(80,190);
      
      storedValues[PITCH] = random(135,256);
      if(random(0,6)==0) storedValues[PITCH] = random(0,120);
      storedValues[CRUSH] = random(160,256);
      storedValues[CROP] = random(128,256);
      storedValues[DROP] = 128;
      if(random(0,3)==0) storedValues[DROP] = random(29,226);

      storedValues[DRONE_MOD] = 127;
      storedValues[DRONE] = 127;
      storedValues[DRONE_ROOT] = random(0,256);
      storedValues[DRONE_PITCH] = 127;
      if(random(0,6)==0) storedValues[DRONE_MOD] = random(0,256);
      if(random(0,4)==0) storedValues[DRONE] = random(64,192);
    
      storedValues[BEAT] = random(0,240);
      storedValues[TIME_SIGNATURE] = 64; // equates to 4/4
      if(random(0,3)==0) storedValues[TIME_SIGNATURE] = random(0,256);
      storedValues[SWING] = random(0,256);
      storedValues[TEMPO] = random(70,160);
      if(random(0,4)==0) storedValues[TEMPO] = random(0,256);
      saveEepromParams(j, i);
    }
  }
}

void saveEepromParams(byte slotNum, byte bankNum) {
  // 1024 possible byte locations in EEPROM
  // currently need 16 bytes per saved state (4 groups of 4 params, 1 byte per param)
  for(byte i=0; i<NUM_PARAM_GROUPS*NUM_KNOBS; i++) {
    EEPROM.write((slotNum+6*bankNum)*SAVED_STATE_SLOT_BYTES+i, storedValues[i]);
  }
}

// DrumKid quality control testing sketch (V6/V7)
// This is probably not the sketch you're looking for!

// STILL NEED TO ADD TEST FOR ANALOG INPUTS

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

int testPhase = 0;

// define pin numbers
const byte ledPins[5] = {2,3,11,12,13};
const byte buttonPins[6] = {4,5,6,7,8,10};
const byte analogPins[4] = {A0,A1,A2,A3};

void HandleNoteOn(byte channel, byte note, byte velocity) {
  if(testPhase==1) {
    aSin.setFreq(mtof(float(note)));
    envelope.noteOn();
    digitalWrite(ledPins[0],HIGH);
    MIDI.sendNoteOn(note+4, 127, 1);
  }
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  if(testPhase==1) {
    envelope.noteOff();
    digitalWrite(ledPins[0],LOW);
    MIDI.sendNoteOff(note+4 , 0, 1);
  }
}

void setup() {
  // initialise pins
  for(int i=0; i<5; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  for(int i=0; i<6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  for(int i=0; i<5; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(500);
    digitalWrite(ledPins[i], LOW);
  }

  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  // Initiate MIDI communications, listen to all channels (not needed with Teensy usbMIDI)
  MIDI.begin(MIDI_CHANNEL_OMNI);

  envelope.setADLevels(255,64);
  envelope.setTimes(50,200,10000,200); // 10000 is so the note will sustain 10 seconds unless a noteOff comes

  aSin.setFreq(440); // default frequency
}

void loop() {
  if(testPhase == 0) {
    for(int i=0; i<5; i++) {
      digitalWrite(ledPins[i],!digitalRead(buttonPins[i])); 
    }
    if(!digitalRead(buttonPins[5])) {
      for(int j=0; j<5; j++) {
        digitalWrite(ledPins[j],HIGH);
        delay(500);
        digitalWrite(ledPins[j],LOW);
      }
      testPhase = 1;
      startMozzi(CONTROL_RATE);
    }
  } else if(testPhase == 1) {
    audioHook();
  }
}

void updateControl(){
  MIDI.read();
  envelope.update();
}


int updateAudio(){
  return (int) (envelope.next() * aSin.next())>>8;
}

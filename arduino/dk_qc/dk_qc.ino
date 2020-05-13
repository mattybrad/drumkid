// DrumKid quality control testing sketch (V6/V7)
// This is probably not the sketch you're looking for!

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

// ALTKID: Basic

// This is an alternative firmware sketch for DrumKid
// This code is simply intended to show how to write your own code for DrumKid
// A sine wave oscillator is controlled by DrumKid's buttons
// The tuning can be controlled using the left potentiometer
// The LEDs will light up when buttons are pressed
// Make sure to install the Mozzi library before starting: https://sensorium.github.io/Mozzi/

// need to include the main Mozzi library, the oscillator template and the sine wavetable data
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

// define the rate at which Mozzi performs non-audio updates (should be a power of 2)
#define CONTROL_RATE 64

// define how many pins are used for DrumKid's LEDs, buttons, and potentiometers
#define NUM_LED_PINS 5
#define NUM_BUTTON_PINS 6
#define NUM_ANALOG_PINS 4

// define which pins are used
const byte ledPins[NUM_LED_PINS] = {2,3,11,12,13};
const byte buttonPins[NUM_BUTTON_PINS] = {4,5,6,7,8,10};
const byte analogPins[NUM_ANALOG_PINS] = {A0,A1,A2,A3};

// define the frequencies of a musical scale (whole tone scale in this example)
const float noteFrequencies[NUM_BUTTON_PINS] = {220.0,246.94,277.18,311.13,349.23,392.00};

// declare some global variables
float baseFrequency;
float tuningMultiplier;

// create a sine oscillator instance
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> sineOscillator(SIN2048_DATA);

void setup(){
  // set the buttons as inputs (with pull-up resistors) and the LEDs as outputs
  for(byte i=0; i<NUM_BUTTON_PINS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  for(byte i=0; i<NUM_LED_PINS; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  // set the starting base frequency
  baseFrequency = noteFrequencies[0];

  // start the Mozzi audio engine
  startMozzi(CONTROL_RATE);
}

// this function is called automatically by Mozzi
// it handles functionality not directly related to audio output (e.g. buttons)
void updateControl(){
  // check each of DrumKid's buttons in turn
  for(byte i=0; i<NUM_BUTTON_PINS; i++) {
    boolean buttonIsPressed = !digitalRead(buttonPins[i]);
    if(buttonIsPressed) {
      baseFrequency = noteFrequencies[i];
    }
    if(i<NUM_LED_PINS) {
      // light up an LED if button is pressed
      digitalWrite(ledPins[i], buttonIsPressed);
    }
  }

  // you must use "mozziAnalogRead" instead of "analogRead" when using Mozzi
  tuningMultiplier = mozziAnalogRead(analogPins[0])/200.0 + 0.5;
  float newFrequency = baseFrequency * tuningMultiplier;

  // update the oscillator's frequency
  sineOscillator.setFreq(newFrequency);
}

// this function is called automatically by Mozzi, and handles audio updates
int updateAudio(){
  // return the new value of the sine oscillator
  return sineOscillator.next();
}


void loop(){
  // unlike normal Arduino sketches, you don't want to put anything else in "loop"
  audioHook();
}

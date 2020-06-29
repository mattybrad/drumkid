// include the ArduinoTapTempo library
#include <ArduinoTapTempo.h>

// define the pin you want to attach your tap button to
const int BUTTON_PIN = 5;

// make an ArduinoTapTempo object
ArduinoTapTempo tapTempo;


void setup() {
  // begin serial so we can see the state of the tempo object through the serial monitor
  Serial.begin(9600);

  // setup your button as required by your project
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH);
}

void loop() {
  // get the state of the button
  boolean buttonDown = digitalRead(BUTTON_PIN) == LOW;
  
  // update ArduinoTapTempo
  tapTempo.update(buttonDown);

  Serial.print("bpm: ");
  Serial.println(tapTempo.getBPM());

  // uncomment the block below to demo many of ArduinoTapTempo's methods
  // note that Serial.print() is not a fast operation, and using it decreases the accuracy of the the tap timing
  
  /*
  Serial.print("len:");
  Serial.print(tapTempo.getBeatLength());
  Serial.print(",\tbpm: ");
  Serial.print(tapTempo.getBPM());
  Serial.print(",\tchain active: ");
  Serial.print(tapTempo.isChainActive() ? "yes" : "no ");
  Serial.print(",\tlasttap: ");
  Serial.print(tapTempo.getLastTapTime());
  Serial.print(",\tprogress: ");
  Serial.print(tapTempo.beatProgress());
  Serial.print(",\tbeat: ");
  Serial.println(tapTempo.onBeat() ? "beat" : "    ");
  */
}

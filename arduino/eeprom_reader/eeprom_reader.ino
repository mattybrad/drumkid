// this sketch helps you back up your drumkid beats/settings from the internal memory (EEPROM)
// run the sketch, open the serial monitor, and save the resulting output somewhere safe!

#include <EEPROM.h>

// start reading from the first byte (address 0) of the EEPROM
int address = 0;
byte value;
bool readDone = false;

void setup() {
  // initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("drumkid eeprom data to paste into loader sketch:");
  while(!readDone) {
    
    // read a byte from the current address of the EEPROM
    value = EEPROM.read(address);
  
    if(address%16==0) Serial.print("\n\t");
    Serial.print(value, DEC);
    Serial.print(",");
  
    address = address + 1;
    if (address == EEPROM.length()) {
      Serial.println("");
      Serial.println("end of data");
      Serial.println("");
      readDone = true;      
    }
  }
}

void loop() {
  // nothing
}

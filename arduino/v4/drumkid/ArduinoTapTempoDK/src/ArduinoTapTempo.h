/*
 * ArduinoTapTempo.h
 * An Arduino library that times consecutive button presses to calculate Beats Per Minute. Corrects for missed beats and can reset phase with single taps. *
 * Copyright (c) 2016 Damien Clarke
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.  
 */
 
#ifndef ARDUINO_TAP_TEMPO_H
#define ARDUINO_TAP_TEMPO_H

#include <Arduino.h>

class ArduinoTapTempo
{
  public:
    static const int MAX_TAP_VALUES = 10;

    bool onBeat(); // returns true if a beat has occured since the last update(), not accurate enough for timing in music but useful for LEDs or other indicators
    // Note that this may return true in rapid succession while tapping and setting the tempo to a slightly slower rate than the current tempo,
    // as the beat may occur and shortly afterward a new tap triggers a new beat
    bool isChainActive(); // returns true if the current tap chain is still accepting new taps to fine tune the tempo
    bool isChainActive(unsigned long ms); // returns true if the current tap chain is still accepting new taps to fine tune the tempo
    float getBPM(); // returns the number of beats per minute
    void setBPM(float bpm); // set the number of beats per minute
    float beatProgress(); // returns a float from 0.0 to 1.0 indicating the percent through the current beat
    void resetTapChain(); // resets the current chain of taps and sets the start of the bar to the current time
    void resetTapChain(unsigned long ms); // resets the current chain of taps and sets the start of the bar to the current time

    inline unsigned long getBeatLength() { return beatLengthMS; } // returns the length of the beat in milliseconds
    inline unsigned long getLastTapTime() { return lastTapMS; } // returns the time of the last tap in milliseconds since the program started

    void update(bool buttonDown); // call this each time you read your button state, accepts a boolean indicating if the button is down

    // getters and setters

    // skipped taps are detected when a tap duration is close to double the last tap duration.
    inline void enableSkippedTapDetection() { skippedTapDetection = true; }
    inline void disableSkippedTapDetection() { skippedTapDetection = false; }
    void setSkippedTapThresholdLow(float threshold); // This sets the lower threshold, accepts a float from 1.0 to 2.0.
    void setSkippedTapThresholdHigh(float threshold); // This sets the upper threshold, accepts a float from 2.0 to 4.0.
    void setBeatsUntilChainReset(int beats); // The current chain of taps will finish this many beats after the most recent tap, accepts an int greater than 1 
    void setTotalTapValues(int total); // Sets the maximum number of most recent taps that will be averaged out to calculate the tempo, accepts int from 2 to MAX_TAP_VALUES
    // increasing this allows the tempo to be more accurate compared to your tapping, but slower to respond to gradual changes in tapping speed.
    inline void setMaxBeatLengthMS(unsigned long ms) { maxBeatLengthMS = ms; } // Sets the maximum beat length permissible.
    // If a tap attempts to set the beat length to anything greater than this value, the new tap will start a new chain and the tempo will remain unchanged.
    inline void setMinBeatLengthMS(unsigned long ms) { minBeatLengthMS = ms; } // Sets the minimum beat length permissible.
    // If the average tap length is less than this value, then this value will be used instead.
    inline void setMaxBPM(float bpm) { minBeatLengthMS = 60000.0 / bpm; } // Sets the minimum beats per minute permissible.
    // This is another way of setting the minimum beat length.
    inline void setMinBPM(float bpm) { maxBeatLengthMS = 60000.0 / bpm; } // Sets the maximum beats per minute permissible.
    // This is another way of setting the maximum beat length.


  private:
    // config
    unsigned long maxBeatLengthMS = 2000; // 30.0bpm
    unsigned long minBeatLengthMS = 250; // 240.0bpm
    int beatsUntilChainReset = 3;
    int totalTapValues = 8;
    float skippedTapThresholdLow = 1.75;
    float skippedTapThresholdHigh = 2.75;

    // button state
    bool buttonDownOld = false;

    // timing
    unsigned long millisSinceReset = 0;
    unsigned long millisSinceResetOld = 0;
    unsigned long beatLengthMS = 500;
    unsigned long lastResetMS = millis();
    
    // taps
    unsigned long lastTapMS = 0;
    unsigned long tapDurations[ArduinoTapTempo::MAX_TAP_VALUES];
    int tapDurationIndex = 0;
    int tapsInChain = 0;
    bool skippedTapDetection = true;
    bool lastTapSkipped = false;

    // private methods
    void tap(unsigned long ms);
    void addTapToChain(unsigned long ms);
    unsigned long getAverageTapDuration();
};

#endif

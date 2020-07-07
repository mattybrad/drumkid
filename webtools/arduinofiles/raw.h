#ifndef testing_H_
#define testing_H_
 
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "src/MozziDK/src/mozzi_pgmspace.h"
 
#define testing_NUM_CELLS 37
#define testing_SAMPLERATE 16384
 
CONSTTABLE_STORAGE(int8_t) testing_DATA [] = {0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0,
1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0,
};

#endif /* testing_H_ */

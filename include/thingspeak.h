#ifndef TS_FUNCTIONS_H
#define TS_FUNCTIONS_H


#include "setup.h"


// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
extern const char* TS_WRITE_KEY;
extern const char* TS_READ_KEY;
extern const char* TS_CHANNEL;
extern const char* TS_TALKBACK_ID; 
extern const char* TS_TALKBACK_KEY;


bool sendThingSpeak( float m, float t, float ec, float ph, int n, int p, int k );
bool getSettings( uint8_t &threshold, uint32_t &duration );

#endif
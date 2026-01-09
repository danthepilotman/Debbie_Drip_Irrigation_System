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


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
bool sendThingSpeak( float m, float t, float ec, float ph, int n, int p, int k, time_t status_time_ESP32 );
bool getSettings( uint8_t &threshold, uint32_t &duration, bool &rain_expected, bool &watering_needed, time_t &status_time_TS );
String urlEncode(const String &s);
void solenoid_state_Update();


#endif
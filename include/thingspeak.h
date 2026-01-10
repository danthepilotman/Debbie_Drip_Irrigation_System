#ifndef TS_FUNCTIONS_H
#define TS_FUNCTIONS_H


#include "setup.h"
#include "RS485.h"


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
String iso8601_Timestamp();
bool sendThingSpeak( float t, float ec, float ph, int n, int p, int k );
bool getSettings();
String urlEncode(const String &s);
void solenoid_state_Update();
bool get_new_readings();


#endif
#ifndef TS_FUNCTIONS_H
#define TS_FUNCTIONS_H


#include "setup.h"
#include "RS485.h"


constexpr uint8_t MAX_TRIES = 5;
constexpr uint8_t TB_DELAY = 10;  // Time to wait for ThingSpeak to process TalkBack
constexpr uint8_t TB_MAX_DELAY = 10;  // Maximum time between when TB was updated and present time

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
bool sendThingSpeak( float t, float ec, float ph, int n, int p, int k );
bool getSettings();
bool get_new_readings();
String urlEncode(const String &input);
String Timestamp();
void solenoid_state_Update();
long secondsSincePosition1(JsonArray arr);
time_t iso8601ToEpochUsingGmtime(const char* ts);


#endif
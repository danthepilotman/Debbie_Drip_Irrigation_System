#ifndef TS_FUNCTIONS_H
#define TS_FUNCTIONS_H


#include "setup.h"
#include "RS485.h"
#include "helper.h"


constexpr uint8_t MAX_TRIES = 5;
constexpr int16_t TB_DELAY = 180;  // Time to wait for ThingSpeak to process TalkBack
constexpr int16_t TB_MAX_DELAY = 180;  // Maximum time between when TB was updated and present time

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
void get_new_readings();



#endif
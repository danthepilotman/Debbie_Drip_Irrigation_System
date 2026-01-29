#ifndef TS_FUNCTIONS_H
#define TS_FUNCTIONS_H


#include "setup.h"
#include "RS485.h"
#include "helper.h"


constexpr uint8_t MAX_TRIES = 5;  // Max tries x TB_DELAY = Max time allowed for TB update via REACT on ThingSpeak
constexpr uint32_t TB_DELAY = 60000UL;  // Time to wait for ThingSpeak to process TalkBack [ms]
constexpr uint32_t TS_DELAY = 15000UL;  // Time to wait for ThingSpeak to process update [ms]

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
void getSettings();
void get_new_readings();



#endif
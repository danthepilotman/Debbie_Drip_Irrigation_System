#ifndef TS_FUNCTIONS_H
#define TS_FUNCTIONS_H


#include "setup.h"  // project configuration
#include "RS485.h"  // RS485 interface
#include "helper.h"  // helper utilities


constexpr uint8_t MAX_TRIES = 5;  // retry attempts
constexpr uint32_t TB_DELAY = 60000UL;  // TalkBack processing delay [ms]
constexpr uint32_t TS_DELAY = 15000UL;  // ThingSpeak update delay [ms]

// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
extern const char* TS_WRITE_KEY;  // ThingSpeak write API key
extern const char* TS_READ_KEY;  // ThingSpeak read API key
extern const char* TS_CHANNEL;  // ThingSpeak channel ID
extern const char* TS_TALKBACK_ID;  // TalkBack ID
extern const char* TS_TALKBACK_KEY;  // TalkBack key


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void sendThingSpeak( float t, float ec, float ph, int n, int p, int k );  // upload readings
void getSettings();  // fetch TalkBack settings
void get_new_readings();  // read sensors and upload
void ping_ThingSpeak();  // Transmit status message to ThingSpeak Channel
void send_RSSI();  // Send Wifi RSSI to ThingSpeak channel

#endif
#ifndef TS_FUNCTIONS_H
#define TS_FUNCTIONS_H


#include "setup.h"  // project configuration
#include "RS485.h"  // RS485 interface
#include "helper.h"  // helper utilities
#include "update_OLED.h"  // OLED update functions
#include "ThingSpeakClient.h"


constexpr uint8_t MAX_TRIES = 5;  // retry attempts
constexpr uint32_t TS_PROCESS_DELAY = 20000UL;  // ThingSpeak update delay [ms]


// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
extern const char* TS_CHANNEL;  // ThingSpeak channel id
extern const char* TS_WRITE_KEY;  // ThingSpeak write key
extern const char* TS_READ_KEY;  // ThingSpeak read key

extern const char* TS_WATERING_ID;  // Watering channel ID used for watering parameters
extern const char* TS_WATERING_WRITE_KEY;  // Watering channel write key used for watering parameters

// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void thingSpeak_Update();  // upload readings and fetch TalkBack settings
void sendThingSpeak();  // upload readings
void getSettings();  // fetch TalkBack settings
void ping_ThingSpeak();  // Transmit status message to ThingSpeak Channel
void send_RSSI();  // Send Wifi RSSI to ThingSpeak channel

#endif
#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ModbusMaster.h>
#include <time.h>


// ==================================================
// ================== BUILD OPTIONS =================
// ==================================================
#define DEBUG_ENABLED
//#define TEST_MODE
//#define DISABLE_RELAY   // comment out when ready for real watering

#ifdef TEST_MODE
const unsigned long UPDATE_INTERVAL = 2UL * 60UL * 1000UL; // 10 minutes
#else
#define UPDATE_INTERVAL 6ULL * 60ULL * 60ULL * 1000000ULL  // 6 hours
#endif


extern const unsigned long SERIAL_BAUD_RATE;

// ==================================================
// ================= WIFI SETTINGS ==================
// ==================================================
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;

// ==================================================
// ================= HARDWARE =======================
// ==================================================
#define RS485_RX   16
#define RS485_TX   17
#define RELAY_PIN  21

extern ModbusMaster node;

// ==================================================
// ================= DEBUG MACROS ===================
// ==================================================
#ifdef DEBUG_ENABLED
  #define DBG(x) Serial.println(x)
  #define DBGf(...) Serial.printf(__VA_ARGS__)
#else
  #define DBG(x)
  #define DBGf(...)
#endif

// ==================================================
// ================= NTP ===================
// ==================================================
extern const char *timeZone;  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
extern const char *ntpServer_1;
extern const char *ntpServer_2;
extern const char *ntpServer_3;


void setup_Serial();
void setup_Digital();
void setup_RS485();
void connect_WiFi();
void setup_NTP();

#endif
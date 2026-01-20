#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <LittleFS.h>


// ==================================================
// ================= LOGIC DEFINITION ================
// ==================================================
#define ON true
#define OFF false

#define YES true
#define NO false

// ==================================================
// ================== BUILD OPTIONS =================
// ==================================================
#define DEBUG_ENABLED
#define DEBBIE_HOUSE
//#define SOIL_SENSOR


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
// ================= HARDWARE =======================
// ==================================================

const int RELAY_PIN = 21;  // Pin used to drive solenoid valve via MOSFET driver circuit

extern HardwareSerial RS485Serial;  // Declare RS485Serial as external so other .cpp files can see it

extern const unsigned long SERIAL_BAUD_RATE;  // Set UI serial baud rate

// ==================================================
// ================= WIFI SETTINGS ==================
// ==================================================
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;

extern bool wifi_connectivity;

// ==================================================
// ================= NTP ===================
// ==================================================
extern const char *timeZone;  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
extern const char *ntpServer_1;
extern const char *ntpServer_2;
extern const char *ntpServer_3;

// ==================================================
// =============== GLOBAL VARIABLES =================
// ==================================================

typedef struct TimeSet
{
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} ScheduleTime;

struct Settings {
  float threshold;  // Soil moisture threshold to trigger watering
  uint32_t duration;  // Watering time in seconds
  TimeSet times[4];  // Up to 4 watering times per day
};

extern Settings settings;

constexpr uint8_t SCHEDULE_COUNT = sizeof(settings.times) / sizeof(settings.times[0]);

extern bool watering_needed_ESP32;  // Watering needed (yes or no)

extern bool solenoid_state;  // Store solenoid open/close state

extern float moisture;

extern bool rain_expected_TS;

extern bool watering_needed_TS;




// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void setup_Serial();
void setup_Digital();
void setup_RS485();
void connect_WiFi();
void setup_NTP();


#endif
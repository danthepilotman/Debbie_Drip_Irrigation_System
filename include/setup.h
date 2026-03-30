#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H

#include <Arduino.h>  // Arduino core
#include <WiFi.h>  // WiFi library
#include <HTTPClient.h>  // HTTP client helper
#include <time.h>  // time functions
#include <HardwareSerial.h>  // hardware serial support
#include <ArduinoJson.h>  // ArduinoJson library
#include <LittleFS.h>  // LittleFS filesystem


// ==================================================
// ================= LOGIC DEFINITION ================
// ==================================================
#define ON true  // logical on
#define OFF false  // logical off

#define YES true  // yes/true alias
#define NO false  // no/false alias

// ==================================================
// ================== BUILD OPTIONS =================
// ==================================================
#define DEBUG_ENABLED  // enable debug logging
#define DEBBIE_HOUSE  // define for Debbie's house config
#define SOIL_SENSOR  // include soil sensor support


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
extern const char* WIFI_SSID;  // WiFi SSID
extern const char* WIFI_PASS;  // WiFi password

extern bool wifi_connectivity;  // WiFi connection status

// ==================================================
// ================= NTP ===================
// ==================================================
extern const char *timeZone;  // timezone setting
extern const char *ntpServer_1;  // primary NTP server
extern const char *ntpServer_2;  // secondary NTP server
extern const char *ntpServer_3;  // tertiary NTP server

// ==================================================
// =============== GLOBAL VARIABLES =================
// ==================================================

typedef struct TimeSet
{
    uint8_t hour;  // hour component
    uint8_t min;   // minute component
    uint8_t sec;   // second component
} ScheduleTime;

struct Settings {
  float threshold;  // Soil moisture threshold to trigger watering
  uint32_t duration;  // Watering time in seconds
  TimeSet times[4];  // Up to 4 watering times per day
};

extern Settings settings;  // global settings struct

constexpr uint8_t SCHEDULE_COUNT = sizeof( settings.times ) / sizeof( settings.times[0] );  // number of schedule slots

extern bool watering_needed_ESP32;  // watering state from ESP32

extern bool solenoid_state;  // solenoid open/close state

extern float moisture;  // current moisture reading

extern bool rain_expected_TS;  // rain flag from ThingSpeak

extern bool watering_needed_TS;  // watering needed flag from ThingSpeak

extern JsonDocument doc;  // JSON document instance (shared)


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void setup_Serial(); // Initialize Serial and debug output
void setup_Discretes(); // Configure GPIOs, relays, and discrete I/O
void setup_RS485(); // Initialize RS485 hardware and serial settings
void connect_WiFi(); // Connect to WiFi network (blocking until success)
void setup_NTP(); // Configure NTP and synchronize system time

#endif
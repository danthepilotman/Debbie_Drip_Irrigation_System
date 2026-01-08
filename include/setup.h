#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <HardwareSerial.h>


// ==================================================
// ================== BUILD OPTIONS =================
// ==================================================
#define DEBUG_ENABLED

// ==================================================
// ================= WIFI SETTINGS ==================
// ==================================================
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;

// ==================================================
// ================= HARDWARE =======================
// ==================================================

const int RELAY_PIN = 21;  // Pin used to drive solenoid valve via MOSFET driver circuit

extern HardwareSerial RS485Serial;  // Declare RS485Serial as external so other .cpp files can see it

extern const unsigned long SERIAL_BAUD_RATE;  // Set UI serial baud rate

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




extern uint32_t duration;  // Watering time in seconds
   
extern bool watering_needed_ESP32;  // Watering needed (yes or no)

extern bool solenoid_closed;  // Store solenoid open/close state



// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void setup_Serial();
void setup_Digital();
void setup_RS485();
void connect_WiFi();
void setup_NTP();

#endif
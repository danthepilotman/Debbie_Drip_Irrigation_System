#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H

#include <Arduino.h>  // Arduino core
#include <WiFi.h>  // WiFi library
#include <HTTPClient.h>  // HTTP client helper
#include <time.h>  // time functions
#include <HardwareSerial.h>  // hardware serial support
#include <ArduinoJson.h>  // ArduinoJson library
#include <LittleFS.h>  // LittleFS filesystem
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
//#define DEBBIE_HOUSE  // define for Debbie's house config
//#define SOIL_SENSOR  // include soil sensor support
#define DEBUG_ENABLED  // enable debug logging

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

const int RELAY_PIN = LED_BUILTIN;  // Pin used to drive solenoid valve via MOSFET driver circuit

extern HardwareSerial RS485Serial;  // Declare RS485Serial as external so other .cpp files can see it

extern const unsigned long SERIAL_BAUD_RATE;  // Set UI serial baud rate

// ==================================================
// ================= OLED =======================
// ==================================================

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


extern Adafruit_SSD1306 display;  // Declare display as external so other .cpp files can see it


enum Page {
    PAGE_STATUS,
    PAGE_SOIL,
    PAGE_SETTINGS,
    PAGE_WIFI,
    NUM_OF_PAGES
};

extern volatile Page currentPage;  // Track current OLED page for button navigation

extern volatile bool buttonPressed;  // Flag to indicate button press for page navigation

const int BUTTON_PIN = GPIO_NUM_33;  // GPIO pin for page navigation button (must be a pin that supports interrupts)

// ==================================================
// ================= WIFI SETTINGS ==================
// ==================================================
extern const char* WIFI_SSID;  // WiFi SSID
extern const char* WIFI_PASS;  // WiFi password


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
  uint32_t rain_min_Prob;  // Minimum probability of rain to set rain_expected flag
  TimeSet times[4];  // Up to 4 watering times per day
};

extern Settings settings;


constexpr uint8_t SCHEDULE_COUNT = sizeof( settings.times ) / sizeof( settings.times[0] );  // number of schedule slots


extern struct Status {
  bool rain_expected;  // whether rain is expected based on weather forecast
  bool watering_needed;  // whether watering is needed based on current moisture and threshold
  bool solenoid_state;  // current state of solenoid valve (ON/OFF)
  bool wifi_connectivity;  // WiFi connection status
  int wifi_rssi;  // WiFi signal strength in dBm
  String status_str;  // Status string to display on OLED, updated with ThingSpeak upload status and timestamp
} status; 


// struct to hold soil sensor readings
extern struct Soil {
  float moisture;  // current moisture reading
  float temp;  // current temperature reading
  float ec;  // current EC reading
  float pH; // current pH reading scaled by 10
  int N;  // current N register reading
  int P;  // current P register reading
  int K;  // current K register reading
} soil;  


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void setup_Serial(); // Initialize Serial and debug output
void setup_Discretes(); // Configure GPIOs, relays, and discrete I/O
void setup_RS485(); // Initialize RS485 hardware and serial settings
void connect_WiFi(); // Connect to WiFi network (blocking until success)
void setup_NTP(); // Configure NTP and synchronize system time
void setup_OLED(); // Initialize OLED display

#endif
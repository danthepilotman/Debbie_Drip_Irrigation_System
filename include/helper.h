#ifndef HELPER_FUNCTIONS_H
#define HELPEER_FUNCTIONS_H


#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include "thingspeak.h"  // thingspeak interface
#include "sleep_timer.h"  // sleep timer interface
#include "ThingSpeakTypes.h"  // ThingSpeak response struct
#include "esp_ota_ops.h"


extern const char *MANIFEST_URL;
extern const char *FIRMWARE_VERSION;

String urlEncode(const String &input);  // URL-encode helper
String Timestamp();  // formatted timestamp
void solenoid_state_Update();  // report solenoid state
long secondsSincePosition1(JsonArray arr);  // compute seconds since position 1
time_t iso8601ToEpochUsingGmtime(const char* ts);  // parse ISO8601 timestamp
void update_Schedule ( String cmdStr, uint8_t position );  // update schedule
bool check_new_settings();  // detect config changes
bool initFlashFS();  // initialize LittleFS
bool loadSettings();  // load settings from FS
bool saveSettings();  // save settings to FS
void printSettings();  // print current settings
void get_new_readings();  // read sensors and store values
void check_button_press(); // check for button press and update currentPage for OLED navigation if button pressed
bool getFirmwareInfo(String &latestVersion, String &firmwareUrl);  // get latest firmware info from ThingSpeak
bool isNewer(String latest);  // compare firmware versions
void performOTA(String url);  // perform OTA update from URL
void check_ota_state();  // check OTA state and update flags accordingly


#endif
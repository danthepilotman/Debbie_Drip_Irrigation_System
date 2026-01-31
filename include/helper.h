#ifndef HELPER_FUNCTIONS_H
#define HELPEER_FUNCTIONS_H


#include "thingspeak.h"  // thingspeak interface
#include "sleep_timer.h"  // sleep timer interface

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


#endif
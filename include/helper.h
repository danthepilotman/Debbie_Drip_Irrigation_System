#ifndef HELPER_FUNCTIONS_H
#define HELPEER_FUNCTIONS_H


#include "thingspeak.h"
#include "sleep_timer.h"

String urlEncode(const String &input);
String Timestamp();
void solenoid_state_Update();
long secondsSincePosition1(JsonArray arr);
time_t iso8601ToEpochUsingGmtime(const char* ts);
void update_Schedule ( String cmdStr, uint8_t position );
bool check_new_settings();
bool initFlashFS();
bool loadSettings();
bool saveSettings();
void printSettings();


#endif
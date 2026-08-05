#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <ArduinoJson.h>  // Default Arduino header file

constexpr uint8_t MAX_TRIES = 5;  // retry attempts

#ifdef THINGSPEAK_ENABLE

    bool check_new_settings();  // detect config changes
    bool saveSettings();  // save settings to FS
    void update_Schedule ( String cmdStr, uint8_t position );  // update schedule

#endif

bool getForecastTimes( const char *startTime, time_t &forecast_time, time_t &current_hour ); // Convert NWS startTime to epoch time and determine the beginning of the current local hour

bool initFlashFS();  // initialize LittleFS
void printSettings();  // print current setting

String urlEncode(const String &input);  // URL-encode helper
String Timestamp(const char* format = "%a %b %d, %Y %I:%M:%S %p");  // formatted timestamp

void check_button_press(); // check for button press and update currentPage for OLED navigation if button pressed
void handle_sample_state();  // handle behavior in sample state (read sensors, update ThingSpeak, compute watering parameters)


#endif
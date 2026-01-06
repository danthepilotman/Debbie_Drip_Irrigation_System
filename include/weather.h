#ifndef WEATHER_FUNCTIONS_H
#define WEATHER_FUNCTIONS_H


#include "setup.h"
#include "ArduinoJson.h"


// ==================================================
// ================= OPENWEATHER ====================
// ==================================================
extern const char* WEATHER_API_KEY;
extern const char* LAT;
extern const char* LON;

bool rainExpectedSoon();

#endif
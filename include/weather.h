#ifndef WEATHER_FUNCTIONS_H
#define WEATHER_FUNCTIONS_H


#include "setup.h"


// ==================================================
// ================= OPENWEATHER ====================
// ==================================================
extern const char* WEATHER_API_KEY;
extern const char* LAT;
extern const char* LON;

// ==================================================
// ========= Prototype Functions ===========
// ==================================================
bool rainExpectedSoon();

#endif
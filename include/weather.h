#ifndef WEATHER_FUNCTIONS_H
#define WEATHER_FUNCTIONS_H


#include "setup.h"


// ==================================================
// ================= OPENWEATHER ====================
// ==================================================
extern const char* WEATHER_API_KEY;
extern const char* LAT;
extern const char* LON;

const float rain_prob_min = 0.40;  // Minimum precipitation probability to determine rain expected


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
bool rainExpectedSoon();

#endif
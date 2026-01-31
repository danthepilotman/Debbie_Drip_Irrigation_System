#ifndef WEATHER_FUNCTIONS_H
#define WEATHER_FUNCTIONS_H


#include "setup.h"  // project config


// ==================================================
// ================= OPENWEATHER ====================
// ==================================================
extern const char* WEATHER_API_KEY;  // OpenWeather API key
extern const char* LAT;  // latitude for forecast
extern const char* LON;  // longitude for forecast

const float rain_prob_min = 0.40;  // Minimum precipitation probability to determine rain expected


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
bool rainExpectedSoon();  // check upcoming hourly forecast for rain

#endif
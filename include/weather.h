#ifndef WEATHER_FUNCTIONS_H
#define WEATHER_FUNCTIONS_H


#include "setup.h"  // project config


const int RAIN_PROB_MIN = 50;  // Minimum precipitation probability percentage [%] to determine rain expected


extern int precip_prob[24]; // Store hourly PoP values

constexpr uint8_t MAX_FORECAST_HOURS = sizeof( precip_prob ) / sizeof( precip_prob[0] );

extern float avg_precip_prob;  // Store average PoP

extern uint8_t valid_hourly_PoP_count;  // Count how many valid hourly forecast periods are found

// ==================================================
// ========= Prototype Functions ===========
// ==================================================


bool getNWSForecast( JsonDocument &doc );
float calculateAveragePoP( uint8_t count );
bool rainExpectedSoon();  // check upcoming hourly forecast for rain
void processForecastPeriods( JsonArray filteredPeriods, time_t next_target, int &total, uint8_t &valid_count );

#endif
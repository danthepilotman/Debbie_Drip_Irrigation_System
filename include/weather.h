#ifndef WEATHER_FUNCTIONS_H
#define WEATHER_FUNCTIONS_H


#include "setup.h"  // project config


const int RAIN_PROB_MIN = 50;  // Minimum precipitation probability percentage [%] to determine rain expected


extern volatile int precip_prob[24]; 

extern float avg_precip_prob;  // Store average PoP

extern uint8_t valid_hourly_PoP_count;  // Count how many valid hourly forecast periods are found


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
bool rainExpectedSoon();  // check upcoming hourly forecast for rain

#endif
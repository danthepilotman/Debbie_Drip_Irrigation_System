#ifndef WEATHER_FUNCTIONS_H
#define WEATHER_FUNCTIONS_H


#include "setup.h"  // project config


const int RAIN_PROB_MIN = 50;  // Minimum precipitation probability percentage [%] to determine rain expected


extern volatile int precip_prob[6];  // 




// ==================================================
// ========= Prototype Functions ===========
// ==================================================
bool rainExpectedSoon();  // check upcoming hourly forecast for rain

#endif
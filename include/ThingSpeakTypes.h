#ifdef THINGSPEAK_ENABLE

#pragma once
#include <Arduino.h>

struct ThingSpeakResponse
{
    int httpCode;
    String body;
};

#endif  // THINGSPEAK_ENABLE
#pragma once
#include <Arduino.h>

struct ThingSpeakResponse
{
    int httpCode;
    String body;
};
#pragma once

#include <WiFiClientSecure.h>
#include <Arduino.h>
#include "ThingSpeakTypes.h"
#include "thingspeak.h"

class ThingSpeakClient
{
public:
    
    ThingSpeakClient();

    void begin(bool insecure = true);

    ThingSpeakResponse get(const char* url);
    
    ThingSpeakResponse post(const char* url, const String& body);

    ThingSpeakResponse getWithRetry(const char* url, uint8_t maxTries = 5, uint32_t delayMs = 20000UL);

    ThingSpeakResponse postWithRetry(const char* url, const String& body, uint8_t maxTries = 5, uint32_t delayMs = 20000UL);

private:
    WiFiClientSecure client;
};

// 👇 global instance declaration
extern ThingSpeakClient tsClient;
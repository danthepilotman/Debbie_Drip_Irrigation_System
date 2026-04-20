#include "ThingSpeakClient.h"
#include <HTTPClient.h>

ThingSpeakClient::ThingSpeakClient() {}

// 👇 actual SINGLE instance lives here
ThingSpeakClient tsClient;

void ThingSpeakClient::begin(bool insecure)
{
    if (insecure)
        client.setInsecure();
}


ThingSpeakResponse ThingSpeakClient::get(const char* url)
{
    HTTPClient http;
    ThingSpeakResponse resp;

    http.begin(client, url);

    resp.httpCode = http.GET();
    resp.body = http.getString();

    http.end();

    return resp;
}


ThingSpeakResponse ThingSpeakClient::post(const char* url, const String& body)
{
    HTTPClient http;
    ThingSpeakResponse resp;

    http.begin(client, url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    resp.httpCode = http.POST(body);
    resp.body = http.getString();

    http.end();

    return resp;
}


ThingSpeakResponse ThingSpeakClient::getWithRetry(const char* url,
                                                  uint8_t maxTries,
                                                  uint32_t delayMs)
{
    ThingSpeakResponse resp;

    for (uint8_t i = 0; i < maxTries; i++)
    {
        resp = get(url);

        if (resp.httpCode == HTTP_CODE_OK)
            break;

        delay(delayMs);
    }

    return resp;
}


ThingSpeakResponse ThingSpeakClient::postWithRetry(const char* url,
                                                   const String& body,
                                                   uint8_t maxTries,
                                                   uint32_t delayMs)
{
    ThingSpeakResponse resp;

    for (uint8_t i = 0; i < maxTries; i++)
    {
        resp = post(url, body);

        if (resp.httpCode == HTTP_CODE_OK && resp.body != "0")
            break;

        delay(delayMs);
    }

    return resp;
}
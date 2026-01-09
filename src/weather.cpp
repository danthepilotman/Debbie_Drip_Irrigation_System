#include "weather.h"


const char* WEATHER_API_KEY = "1f237060a56d83d3827815039317d2a9";
const char* LAT = "28.027";
const char* LON = "-80.631";


// ==================================================
// ================= WEATHER ========================
// ==================================================
bool rainExpectedSoon()
{
    DBG( F( "[WEATHER] Checking forecast" ) );

    HTTPClient http;

    String url = "https://api.openweathermap.org/data/2.5/forecast?lat=" +
                 String(LAT) + "&lon=" + String(LON) +
                 "&appid=" + WEATHER_API_KEY;

    http.begin( url );

    int code = http.GET();

    DBGf( "[WEATHER] HTTP code: %d\n", code );

    if ( code != 200 )  // Check for errors
    {
        http.end();
        return false;
    }

    JsonDocument doc;  // Create JSON document instance

    DeserializationError err = deserializeJson( doc, http.getString() );  // Process JSON document

    http.end();

    if ( err )
    {
        DBG( F( "[WEATHER][ERROR] JSON parse failed" ) );
        return false;
    }

    uint8_t checks = 0;

    for ( JsonObject item : doc["list"].as<JsonArray>() )  // Loop through the forecast JSON doc
    {
        if ( checks++ > 4 )
            break; // next 12 hours

        String main = item["weather"][0]["main"];  // Retrieve main weather forecast

        DBGf( "[WEATHER] Forecast: %s\n", main.c_str() );  // Print main forecast

        if ( main == "Rain" || main == "Drizzle" || main == "Thunderstorm" )  // Check for "rain" events
            return true;
    }
    
    return false;  // If you made it past the for loop without finding any precip then no rain is expected

}

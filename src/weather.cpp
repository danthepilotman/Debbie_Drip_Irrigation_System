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

    String url = "https://api.openweathermap.org/data/3.0/onecall?lat=" +
                 String(LAT) + "&lon=" + String(LON) + "&exclude=current,daily,minutely,alerts" +
                 "&appid=" + WEATHER_API_KEY;

    http.begin( url );

    int code = http.GET();

    DBGf( "[WEATHER] HTTP code: %d\r\n", code );

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

    for ( JsonObject item : doc["hourly"].as<JsonArray>() )  // Loop through the forecast JSON doc
    {
        if ( checks++ > 5 )  // next 6 hours since forecast are hourly
            break; 

        String main = item["weather"][0]["main"];  // Retrieve main weather forecast

        String pop = item["weather"]["pop"];  // Retrieve probability of precipitation

        float precip_prob = pop.toFloat();  // Convert to floating point number
       
        DBGf( "[WEATHER] Forecast: %s\tPop: %.2f\r\n", main.c_str(),  precip_prob );  // Print main forecast

        if ( ( main == "Rain" || main == "Drizzle" || main == "Thunderstorm" ) && precip_prob > rain_prob_min )  // Check for "rain" events
            return true;
        
    }
    
    return false;  // If you made it past the for loop without finding any precip then no rain is expected

}
